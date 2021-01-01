#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import pathlib
import shlex
from pathlib import Path
from typing import Dict,List

def mkpath(path):
  if path == None:
    return None
  if isinstance(path,pathlib.Path):
    return path
  return pathlib.Path(path)

shsplit = shlex.split



class IslObj:
    IsNull = False
    IsObj = False
    def __init__(self):
        pass

class IslNullObj(IslObj):
    IsNull = True
    def __init__(self):
        super().__init__()

    def use(self,lvl):
        pass # ignore, NULLs have not uses

    def asArg(self):
        return "NULL"


class IslMemObj(IslObj):
    IsObj=True
    def __init__(self,name:str,ty:str,ptr:int,deflvl):
        super().__init__()
        self.name=name
        self.ty=ty
        self.ptr=ptr
        self.definition=None
        self.deflvl=deflvl
        self.islocal=(deflvl is not None)
        self.hasuse=False

    def use(self,lvl):
        self.hasuse=True
        if lvl and lvl!=self.deflvl:
            self.islocal=False

    def asArg(self):
        return self.name


class IslArg:
    IsVal = False
    IsPtr = False
    def __init__(self,name,ty):
        self.name=name
        self.ty=ty

class IslValArg(IslArg):
    IsVal = True
    def __init__(self,name,ty,val):
        super().__init__(name,ty)
        self.val = val


class IslPtrArg(IslArg):
    IsPtr = True
    def __init__(self,name,ty,obj):
        super().__init__(name,ty)
        self.obj=obj



class Stmt:
    IsCall = False
    IsPredef = False


class IslCall(Stmt):
    IsCall=True
    def __init__(self,funcname,hasret,retty):
        self.funcname=funcname
        self.args =[]
        self.hasret=hasret
        if hasret:
            #self.retarg=None
            #self.retval=None
            self.retty=retty
            self.retobj=None
            self.desc=None

    def addArg(self, arg):
        self.args.append(arg)


class Predef(Stmt):
    IsPredef=True
    def __init__(self,name,ty,ptr,code,obj):
        self.name=name
        self.ty=ty
        self.ptr=ptr
        self.code=code
        self.obj=obj


class Level:
    def __init__(self):
        self.calls =[]

    def addCall(self, call):
        self.calls.append(call)

    def getLastcall(self):
        return self.calls[-1]


class MainLevel(Level):
    IsMain = True
    IsCallback=False

    def __init__(self):
        super().__init__()
        self.gentime=None


class CallbackLevel(Level):
    IsMain = False
    IsCallback=True

    def __init__(self,callbacker):
        super().__init__()
        self.callbacker=callbacker
        self.args = [None]*len(callbacker.paramtys)
        if callbacker.hasret:
            self.retarg=None


class Callbacker:
    def __init__(self,backerptr,name,fname,pname,retty,paramtys:List[str]):
       self.backerptr=backerptr
       self.name=name
       self.fname = fname
       self.pname = pname
       self.hasret = (retty.strip() !='void')
       self.retty = retty
       self.paramtys = paramtys
       self.invocations=[] # of CallbackLevel

    def addInvocation(self,level:CallbackLevel):
        self.invocations.append(level)


shortnames = {'isl_ctx':'ctx', 'isl_printer': 'print',
              'isl_id': 'id', 'isl_space': 'space', 'isl_local_space': 'ls',
              'isl_set': 'set', 'isl_union_set': 'uset',  'isl_basic_set': 'bset', 
              'isl_map': 'map', 'isl_union_map': 'umap',  'isl_basic_map': 'bmap', 
              'isl_aff': 'aff', 'isl_pw_aff': 'paff', 'isl_pw_multi_aff': 'pma', 'isl_union_pw_multi_aff': 'upma', ' isl_multi_union_pw_aff': 'mupa',
              'isl_union_access_info': 'acceses', 'isl_union_flow': 'uflow',
              'isl_schedule': 'sched', 'isl_schedule_node': 'node', 'isl_schedule_constraints': 'contraints',
              'isl_ast_build': 'build', 'isl_ast_node': 'node', 'isl_ast_expr': 'expr',
               }
def getShortname(ty: str)->str:
    ty = ty.strip()
    if ty.endswith('*'):
        # is a pointer
        pointee = ty[:-1].strip()
        if pointee.endswith('_list'):
            return 'list'
        if sn := shortnames.get(pointee):
            return sn
        return 'ptr'
    
    return '???'


nullobj = IslNullObj()
memory:Dict[int,IslObj] = {}
allobjs:List[IslObj] = []
nextid:Dict[str,int] = {}
def lookupObj(ptr:int,ty:str,lvl:Level):
    global memory,nullobj
    if ptr== 0:
        return nullobj
    if obj := memory.get(ptr):
        obj.use(lvl=lvl)
        return obj
    
    return None
    # Create a new one on-the-fly (can we completley avoid this?) 
    adhoc = registerObj(ptr,ty,lvl=None)
    adhoc.use(lvl=lvl)
    return adhoc


def nextName(shortname:str):
    global nextid
    count = nextid.get(shortname, 1)
    name = shortname + str(count)
    nextid[shortname] = count+1
    return name


def registerObj(ptr:int,ty:str,lvl:Level,name=None):
    global memory,nullobj,allobjs
    assert isinstance(ptr,int)

    if ptr==0:
        return nullobj

    shortname = getShortname(ty)
    if name==None:
        name = nextName(shortname)
    if name =='node2':
        a=3
    obj = IslMemObj(name=name,ty=ty,ptr=ptr,deflvl=lvl)
    memory[ptr] = obj
    allobjs.append(obj)
    return obj


callstack: List[Level]=[]
def getToplevel():
    return callstack[-1]


def parse_isltrace(gentime):
    getToplevel().gentime=gentime


def parse_predef(name,ty,ptr,code):
    ptr = int(ptr,0)
    obj = registerObj(ptr=ptr,ty=ty,lvl=getToplevel(),name=name)
    predef = Predef(name=name,ty=ty,ptr=ptr,code=code,obj=obj)
    getToplevel().addCall(predef)


def make_arg(ty,name=None,val=None,ptr=None,uselvl=None):
    global callbackers
    assert val==None or ptr==None

    if val != None:
        arg = IslValArg(name=name,ty=ty,val=val)
    elif ptr!=None:
        ptr=int(ptr, 0)
        if cb := callbackers.get(ptr):
            return IslValArg(name=name,ty=ty,val=cb.name)
        if uselvl==None:
            uselvl=getToplevel()
        obj = lookupObj(ptr,ty=ty,lvl=uselvl)
        if obj != None:        
            arg = IslPtrArg(name=name,ty=ty,obj=obj)
        else:
            arg = IslValArg(name=name,ty=ty,val=f"({ty}){hex(ptr)}")
    else:
        #return None # TODO: should be defined with something
        assert False

    return arg


def parse_call(hasret,fname,rettype,numargs,**kwargs):
    numargs=int(numargs)
    call = IslCall(funcname=fname,hasret=hasret,retty=rettype)
    for i in range(numargs):
        prefix = 'parm'+str(i)
        popts = { k.removeprefix(prefix) : v for k,v in kwargs.items() if k.startswith(prefix) }

        def parse_arg(name,type,val=None,ptr=None):
            arg= make_arg(ty=type,name=name,val=val,ptr=ptr)
            call.addArg(arg)

        parse_arg(**popts)

    getToplevel().addCall(call)


def parse_return(rettype,fname,retval=None,retptr=None,desc=None):
    toplevel =getToplevel()
    lastcall = toplevel.getLastcall()
    assert lastcall.funcname==fname
    assert lastcall.hasret
    assert lastcall.retobj == None

    if retptr!=None:
        retptr=int(retptr,0)
        lastcall.retobj = registerObj(ptr=retptr,ty=rettype,lvl=toplevel)
    #lastcall.retarg = make_arg(ty=rettype,val=retval,ptr=retptr)
    lastcall.desc=desc



callbackers:Dict[int,Callbacker]={}
def getCallbacker(ptr:int):
    global callbackers
    assert ptr
    if backer := callbackers.get(ptr):
        return backer
    assert False
    


def parse_callback(callbacker,fname,pname,retty,numargs,paramtys):
    global callbackers
    backerptr=int(callbacker,0)
    assert backerptr
    assert backerptr not in callbackers
    name = nextName(fname + '_'+pname + '_cb')
    paras = [s.strip() for s in paramtys.split(',')]
    numargs = int(numargs)
    assert len(paras) == numargs
    backer = Callbacker(backerptr=backerptr,name=name,fname=fname,pname=pname,retty=retty,paramtys=paras)  
    callbackers[backerptr]=backer


def isPtr(ty:str):
    return ty.rstrip().endswith('*')

def parse_callback_enter(callbacker,numargs,**kwargs):
    global callstack
    backerptr=int(callbacker,0)
    backer=getCallbacker(backerptr)
    cb = CallbackLevel(callbacker=backer)
    backer.addInvocation(cb)
    callstack.append(cb)

    numargs = len(backer.paramtys)
    for i,aty in enumerate(backer.paramtys):
        parmname = f"arg{i}ptr"
        parmptr = kwargs.get(parmname,None)
        if isPtr(aty) and parmptr!=None:
            ptr=int(parmptr,0)
            argobj = registerObj(ptr, aty, lvl=cb)
            cb.args[i] = argobj
            


def parse_callback_exit(callbacker,retty,retval=None,retptr=None):
    global callstack
    backerptr = int(callbacker,0)  
    cblevel =  callstack[-1]
    assert cblevel.callbacker.backerptr == backerptr
    assert cblevel.callbacker.retty==retty
    assert cblevel.retarg==None

    cblevel.retarg= make_arg(ty=retty,val=retval,ptr=retptr,uselvl=cblevel)

    callstack.pop()


def parse(line:str):
    split = shsplit(line)
    cmd = split[0]
    opts = {}
    for l in split[1:]:
        k,v = l.split(sep='=',maxsplit=1)
        opts[k] = v

    if cmd == "isltrace":
        parse_isltrace(gentime=opts['gentime'])
    elif cmd == "predef":
        parse_predef(**opts)
    elif cmd in ["call_func", "call_proc"]:
        parse_call(hasret=(cmd=="call_func"),**opts)
    elif cmd == "return":
        parse_return(**opts)
    elif cmd == "callback":
        parse_callback(**opts)
    elif cmd == "callback_enter":
        parse_callback_enter(**opts)
    elif cmd == "callback_exit":
        parse_callback_exit(**opts)
    else:
        assert False, "handle this!"


prologue="""#include <stdio.h>
#include <isl/aff.h>
#include <isl/aff_type.h>
#include <isl/arg.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/ast_type.h>
#include <isl/constraint.h>
#include <isl/ctx.h>
#include <isl/fixed_box.h>
#include <isl/flow.h>
#include <isl/hash.h>
#include <isl/id.h>
#include <isl/id_to_ast_expr.h>
#include <isl/id_to_id.h>
#include <isl/id_to_pw_aff.h>
#include <isl/id_type.h>
#include <isl/ilp.h>
#include <isl/list.h>
#include <isl/local_space.h>
#include <isl/lp.h>
#include <isl/map.h>
#include <isl/map_to_basic_set.h>
#include <isl/map_type.h>
#include <isl/mat.h>
#include <isl/maybe.h>
#include <isl/maybe_ast_expr.h>
#include <isl/maybe_basic_set.h>
#include <isl/maybe_id.h>
#include <isl/maybe_pw_aff.h>
#include <isl/multi.h>
#include <isl/obj.h>
#include <isl/options.h>
#include <isl/point.h>
#include <isl/polynomial.h>
#include <isl/polynomial_type.h>
#include <isl/printer.h>
#include <isl/printer_type.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl/schedule_type.h>
#include <isl/set.h>
#include <isl/set_type.h>
#include <isl/space.h>
#include <isl/space_type.h>
#include <isl/stream.h>
#include <isl/stride_info.h>
#include <isl/union_map.h>
#include <isl/union_map_type.h>
#include <isl/union_set.h>
#include <isl/union_set_type.h>
#include <isl/val.h>
#include <isl/val_type.h>
#include <isl/vec.h>
#include <isl/version.h>
#include <isl/vertices.h>
"""


def gen_arg(arg, call: Stmt, level: Level):
    if arg.IsVal:
        return arg.val

    if arg.IsPtr:
        obj = arg.obj
        if obj.IsNull:
            return "NULL"     
        if level.IsCallback:   
          for i,paramobj in enumerate(level.args):
            if paramobj == obj:
                return f"param{i}"
        return obj.asArg()

    assert False, "implement this case" 


def gen_call(call: Stmt, level: Level):
    if call.IsCall:
        funcname = call.funcname
        
        argstr = []
        for arg in call.args:
            argstr.append(gen_arg(arg,call,level))

        callstr = f"{funcname}({', '.join(argstr)});"

        if call.hasret:
            retobj = call.retobj
            descstr = f" // {call.desc}" if call.desc else ""
            if retobj!=None and not retobj.IsNull and retobj.hasuse:
                if retobj.islocal:
                    yield f"{retobj.ty} {retobj.name} = {callstr}{descstr}"
                else:
                    yield f"{retobj.name} = {callstr}{descstr}"   
            else:
                 yield f"{callstr}{descstr}"
        else:
            yield callstr
    elif call.IsPredef:
        yield call.code
    else:
        assert False


def gen_version():
    yield "const char *dynIslVersion = isl_version();" # for some reason isl_version has a trailing newline
    yield f"const char *genIslVersion = \"{getToplevel().gentime}\";"
    yield "printf(\"### ISL version  : %s### at generation: %s\\n\",  dynIslVersion, genIslVersion);"


def gen_globvars():
    global memory,allobjs
    for obj in allobjs:
        if obj.name=='name2': 
                b =4
        if not obj.hasuse:
            continue
        if obj.islocal:
            continue
        yield f"{obj.ty} {obj.name};"


def gen_callbacker(callbacker:Callbacker,genbody:bool):
    if genbody:
        params = ', '.join(f"{argtype} param{i}" for i,argtype in enumerate(callbacker.paramtys))
    else:
        params = ', '.join(argtype for i,argtype in enumerate(callbacker.paramtys))
    decl = f"static {callbacker.retty} {callbacker.name}({params})"
    if not genbody:
        yield f"{decl};"
        return

    yield f"{decl} {{"
    yield  "  static int callidx = 0;"
    yield  "  switch (callidx++) {"
    for i,invok in enumerate(callbacker.invocations):
        yield f"    case {i}: {{"
        for i,argtype in enumerate(callbacker.paramtys):
           argobj = invok.args[i] # should be an IslArg?
           if not argobj:
               continue
           if not argobj.islocal:
               yield f"      {argobj.name} = param{i};"

        for call in invok.calls:
            for line in gen_call(call,level=invok):
                yield f"      {line}"
        if callbacker.hasret:
            retstr = gen_arg(invok.retarg, None, level=invok)
            yield f"      return {retstr};"
        yield "    } break;"
    yield "    default:"
    yield "      // trace only recorded the calls above"
    yield "      abort();"
    yield "  };"
    yield "}"


def gen_callbacks():
    # Forward declarations
    for cb in callbackers.values():
        yield from gen_callbacker(callbacker=cb,genbody=False)
    yield ""

    # Definiition
    for cb in callbackers.values():
        yield from gen_callbacker(callbacker=cb,genbody=True)
        yield ""


def gen_main():
    assert len(callstack)==1
    toplevel = callstack[0]
    for call in toplevel.calls:
        yield from gen_call(call,level=toplevel)


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('logfile')
    parser.add_argument('--outfile', '-o')

    args = parser.parse_args()
    logfile = mkpath(args.logfile)
    outfile = mkpath(args.outfile)
    if not outfile:
        outfile = logfile.with_suffix('.c')

    global callstack
    callstack = [MainLevel()]

    with logfile.open(mode='r') as log:
        for line in log:
            parse(line)
    
    with outfile.open(mode='w') as out:
        print(prologue,file=out)

        print("// variables used accross callbacks",file=out)
        for line in gen_globvars():
            print(line,file=out)
        print("",file=out)

        for line in gen_callbacks():
            print(line,file=out)

        print("int main() {",file=out)
        for line in gen_version():
            print(f"  {line}",file=out)
        print("",file=out)
        for line in gen_main():
            print(f"  {line}",file=out)
        print("  return EXIT_SUCCESS;",file=out)
        print("}",file=out)



if __name__ == '__main__':
    main()
