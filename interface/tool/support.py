#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import shlex
import platform
import itertools
import os
import tempfile
import ntpath
import pathlib
import weakref
import shutil
import sys
import stat
from pathlib import Path





shsplit = shlex.split

def shquote(arg,windows=None):
    if windows is None:
        windows = (platform.system()=='Windows')
    if windows:
        arg = str(arg)
        arg = arg.replace(r'"',r'""')
        if ' ' in arg or '^' in arg:
            return r'"' + arg + r'"'
        return arg
    return shlex.quote(str(arg))


def shjoin(args,sep=' ',windows=None):
  return sep.join([shquote(arg,windows=windows) for arg in args])



def shcombine(arg=None,args=None):
  result = []

  # pre-escaped arguments
  if arg is not None:
    result += arg

  # args needing de-escaping
  if args is not None:
    result += [arg for arg in shsplit(argline) for argline in args]

  return result

# from https://stackoverflow.com/questions/1714027/version-number-comparison-in-python
def version_cmp(v1, v2):
    def convert_int(v):
      result = []
      for p in v.split('.'):
        try: 
          i = int(p)
        except:
          # development versions have their sha1 appended
          break;
        result.append(i)
      return result
    parts1 = convert_int(v1)
    parts2 = convert_int(v2)
    parts1, parts2 = zip(*itertools.zip_longest(parts1, parts2, fillvalue=0))
    return parts1>=parts2











# For objects representing a special value
class NamedSentinel:
    def __init__(self,name,**kwargs):
        self.name = name
        for k,v in kwargs.items():
          setattr(self,k,v)
    def __repr__(self):
        return self.name





def mark_executable(path):
    assert os.path.isfile(path)
    st = os.stat(path)
    os.chmod(path, st.st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)


def remove_readonly(func, path, _):
    os.chmod(path, stat.S_IWRITE)
    func(path)



if os.name == 'nt' and sys.version_info < (3,8):
    class TemporaryDirectory(object):
        """Also delete read-only files on Windows
        See https://bugs.python.org/issue26660
        """

        def __init__(self, suffix="", prefix=tempfile.template, dir=None):
            self.name = tempfile.mkdtemp(suffix, prefix, dir)
            self._finalizer = weakref.finalize(self, self._cleanup, self.name,
                warn_message="Implicitly cleaning up {!r}".format(self))

        @classmethod
        def _cleanup(cls, name, warn_message):
            shutil.rmtree(name)
            warnings.warn(warn_message, ResourceWarning)

        def __repr__(self):
            return "<{} {!r}>".format(self.__class__.__name__, self.name)

        def __enter__(self):
            return self.name

        def __exit__(self, exc, value, tb):
            self.cleanup()

        def cleanup(self):
            if self._finalizer.detach():
                shutil.rmtree(self.name,onerror=remove_readonly)
else:
    TemporaryDirectory = tempfile.TemporaryDirectory



def replace_file(src,dst):
    try:
        os.replace(src,dst)
    except:
        os.remove(dst)
        shutil.move(src,dst)






def ntpath_to_mingwpath(path):
    if os.path is not ntpath:
        return path
    if isinstance(path,pathlib.PureWindowsPath):
      path = str(path)

    if len(path) >= 2 and path[0].isalpha() and path[1] == ':':
      return '/' + path[0].lower() + path[2:].replace('\\', '/')

    return path

    assert ntpath.isabs(path)
    drive,path = ntpath.splitdrive(path)
    parts = path.split('\\')[1:]

    if drive:
        parts =  ['/' + drive[:-1].lower()] + parts

    result = posixpath.join(*parts)
    return result



def mingwpath_to_ntpath(path):
  if os.path is not ntpath:
    if isinstance(path,Path):
      return path
    return Path(path)

  # Detect absolute MinGW paths
  if len(path) == 1 and path == '/':
    return Path('\\')
  if len(path) ==2 and path[0] =='/' and path[1].isalpha():
    return Path(path[1] + ':')
  if len(path) >= 3 and path[0]=='/' and path[1].isalpha() and path[2]=='/':
    return Path(path[1] + ':\\' +  path[3:].replace('/','\\'))

  return mkpath(path)


def mkpath(path):
  if path == None:
    return None
  if isinstance(path,pathlib.Path):
    return path
  return pathlib.Path(path)

def mkpurepath(path):
  if isinstance(path,pathlib.PurePath):
    return path
  return pathlib.PurePath(path)


def first_existing(*args):
    assert len(args) >= 1
    for arg in args[:-1]:
        if os.path.exists(arg):
            return arg
    return args[-1]

# Like (.. or .. or ..) but only considers 'None' being false-y (and no short-circuit)
# fallback= not really necessary, just to make it explicit
def first_defined(*args,fallback=None):
    for arg in args:
        if arg is not None:
            return arg
    return fallback


# Select a choice out of a set of predefined values
#
# Passing a dict allows mapping an alias to another definition. Multiple dicts/sets are applied one ofter the other.
#
# default: Default word if none matches, substitutions are also applied to it
# nomatch: Default return of no substitution matches; no substitutions are applied
#
# return predefined choice or substitution, number of matches of original word (or False if fallback to default)
def predefined(word,*substitutions,default=None):
  if word is None:
    if default is None:
      return None,False
    result,_ = predefined(default,*substitutions)
    return result,False

  rounds = 0
  for subst in substitutions:
    word_lower = word.lower()
    if word_lower in subst:
      rounds += 1
      if isinstance(subst,set) or isinstance(subst,list):
        word = word_lower
      else:
        word = subst[word_lower]
  if not rounds:
    return word,True
  return word,rounds



def predefined_fallback(word,*substitutions,fallback=None):
  result,rounds = predefined(word,*substitutions)
  if not rounds or rounds is True:
      if fallback is None:
        return None,False
      result,_ = predefined(fallback,*substitutions)
      return result, False
  return result,rounds


# Like predefined, but throws an exception when no match.
PREDEFINED_NODEFAULT=NamedSentinel('NODEFAULT')
def predefined_strict(word,*substitutions,default=PREDEFINED_NODEFAULT):
  if word is None:
    if default is None:
      return None
    if default is PREDEFINED_NODEFAULT:
      raise Exception("{word} is not one of the predefined values".format(word=word))
    result,_ = predefined(default,*substitutions)
    return result



  result,rounds = predefined(word,*substitutions,default=default)
  #assert rounds, "word must be one of the predefined values"
  if not rounds or rounds is True:
    raise Exception("{word} is not one of the predefined values".format(word=word))
  return result



def last_defined(*args,fallback=None):
    for arg in reversed(args):
        if arg is not None:
            return arg
    return fallback


def empty_none(arg):
    if arg is None:
        return []
    return arg


def min_none(*args):
    result = args[0]
    for arg in args[1:]:
        if result is None:
            result = arg
        if arg is None:
            continue
        result = min(result, arg)
    return result


def max_none(first, *args):
    result = args[0]
    for arg in args[1:]:
        if result is None:
            result = arg
        if arg is None:
            continue
        result = max(result, arg)
    return result



def createfile(filename,contents):
    if isinstance(filename,Path):
      with filename.open(mode='w+') as f:
        f.write(contents)
    with open(filename, 'w+')as f:
        f.write(contents)





def program_exists(cmd):
    if sys.platform == 'win32' and not cmd.endswith('.exe'):
        cmd += '.exe'
    for path in os.environ["PATH"].split(os.pathsep):
        if os.access(os.path.join(path, cmd), os.X_OK):
            return True
    return False


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def die(msg):
    eprint(msg)
    sys.exit(1)


def unique(list1): 
    unique_list = [] 
    for x in list1: 
        if x not in unique_list: 
            unique_list.append(x) 
    return unique_list
      

def join_natural(l, separator=', ', lastseparator = ' and '):
  l = list(l)
  n = len(l)
  result = ''
  for i,v in enumerate(l):
    if i == 0:
      result = str(v)
    elif i == n-1:
      result += str(lastseparator) + str(v)
    else:
      result +=  str(separator) + str(v)
  return result