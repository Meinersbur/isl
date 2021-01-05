#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import pathlib
import shlex
import tempfile
import copy
import tool.invoke as invoke
from tool.support import *
from pathlib import Path
from typing import Dict,List
from concurrent import futures
import random


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
        #self.definition=None
        self.deflvl=deflvl
        self.islocal=(deflvl is not None)
        #self.hasuse=False

    def use(self,lvl):
        #self.hasuse=True
        if lvl and lvl!=self.deflvl:
            self.islocal=False

    def asArg(self):
        return self.name


class IslArg:
    IsVal = False
    IsPtr = False
    IsCb=False
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

    def clone(self):
        return copy.copy(self)

class IslCbArg(IslArg):
    IsCb = True
    def __init__(self,name,ty,callbacker):
        super().__init__(name,ty)
        assert callbacker
        self.callbacker=callbacker



class Stmt:
    IsCall = False 
    IsPredef = False


class IslCall(Stmt):
    IsCall=True
    def __init__(self,funcname,hasret,retty,isspecial=None):
        self.funcname=funcname
        self.args =[]
        self.isspecial = isspecial
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
        self.hasret=True
        self.code=code
        self.retobj=obj


class Level:
    def __init__(self):
        self.calls = []

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
        self.allcalls = []
        #self.callbackers=[]


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
    assert False
    return '???'


def get_getctx_func(ty:str) -> str:
    if ty.endswith('*'):
        # is a pointer
        pointee = ty[:-1].strip()
        return f'{pointee}_get_ctx'
    return None

def get_readfromstr_func(ty:str) -> str:
    if ty.endswith('*'):
        # is a pointer
        pointee = ty[:-1].strip()
        if pointee =='isl_space' or pointee=='isl_constraint':
            return None
        return f'{pointee}_read_from_str'
    return None


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
    callstack[0].allcalls.append(predef)


def make_arg(ty,name=None,val=None,ptr=None,cb=None,uselvl=None):
    global callbackers
    assert val==None or ptr==None

    if cb != None:
        cbptr=  int(cb,0)
        callbacker = getCallbacker(cbptr)
        arg = IslCbArg(name=name,ty=ty,callbacker=callbacker)
    elif val != None:
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

        def parse_arg(name,type,val=None,ptr=None,cb=None):
            arg= make_arg(ty=type,name=name,val=val,ptr=ptr,cb=cb)
            call.addArg(arg)

        parse_arg(**popts)

    if fname=="isl_val_get_abs_num_chunks":
        call.args[2] = IslValArg(name="chunks",ty="void*",val="/*don't write to memory*/NULL")

    getToplevel().addCall(call)
    callstack[0].allcalls.append(call)


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
    #callstack[0].callbackers.append(backer)


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

    cblevel.retarg = make_arg(ty=retty,val=retval,ptr=retptr,uselvl=cblevel)

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


def gen_arg(arg, call: Stmt, level: Level, mutations):
    origarg=arg
    while True:
        if not arg.IsPtr:
            break
        obj = arg.obj
        if obj not in mutations.objs_replaced:
            break
        arg = mutations.objs_replaced[obj]
    if origarg!=arg:
        arg=arg.clone()
        arg.name=origarg.name

    if arg.IsCb:
        callbacker = arg.callbacker
        return callbacker.name

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

def get_repl_obj(obj,mutations):
    while obj in mutations.objs_replaced:
        obj = mutations.objs_replaced[obj].obj
    return obj

def gen_call(call: Stmt, level: Level, mutations, reachability):
    if call.hasret and call.retobj and call.retobj.IsObj  and call.retobj.name == 'umap28':
                c=4
    if call in mutations.calls_removed:
        return
    if call in mutations.calls_replaced:
        replcall = mutations.calls_replaced[call]
        yield from gen_call(call=replcall,level=level,mutations=mutations,reachability=reachability)
        return

    if call.IsCall:
        funcname = call.funcname
        
        argstr = []
        for arg in call.args:
            argstr.append(gen_arg(arg,call,level,mutations=mutations))
        if call.isspecial!=None:
            argstr[call.isspecial] += ')'

        callstr = f"{funcname}({', '.join(argstr)});"

        if call.hasret:
            retobj = call.retobj # get_repl_obj(call.retobj,mutations=mutations)
            descstr = f" // {call.desc}" if call.desc else ""
            if retobj!=None and not retobj.IsNull and retobj in reachability.reachable_objs:
                if  retobj not in reachability.globobjs: #retobj.islocal:
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


def gen_version(mainlevel):
    yield "const char *dynIslVersion = isl_version();" # for some reason isl_version has a trailing newline
    yield f"const char *genIslVersion = \"{mainlevel.gentime}\";"
    yield "printf(\"### ISL version  : %s### at generation: %s\\n\",  dynIslVersion, genIslVersion);"


def gen_globvars(mainlevel,reachability):
    global memory,allobjs
    for obj in allobjs:
        if obj not in reachability.reachable_objs:
            continue
        #if not obj.hasuse:
        #    continue
        if obj not in reachability.globobjs: #.islocal:
            continue
        yield f"{obj.ty} {obj.name};"


def gen_callbacker(callbacker:Callbacker,genbody:bool,mutations,reachability):
    if callbacker not in reachability.reachable_callbackers:
        return

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
            if argobj not in reachability.reachable_objs:
               continue
            if argobj not in reachability.globobjs : # argobj.islocal:
               continue
            yield f"      {argobj.name} = param{i};"

        for call in invok.calls:
            for line in gen_call(call,level=invok,mutations=mutations,reachability=reachability):
                yield f"      {line}"
        if callbacker.hasret:
            retstr = gen_arg(invok.retarg, None, level=invok, mutations=mutations)
            yield f"      return {retstr};"
        yield "    } break;"
    yield "    default:"
    yield "      // trace only recorded the calls above"
    yield "      abort();"
    yield "  };"
    yield "}"


def gen_callbacks(mainlevel,mutations,reachability):
    global callbackers

    # Forward declarations
    for cb in callbackers.values():
        yield from gen_callbacker(callbacker=cb,genbody=False,mutations=mutations,reachability=reachability)
    yield ""

    # Definitions
    for cb in callbackers.values():
        yield from gen_callbacker(callbacker=cb,genbody=True,mutations=mutations,reachability=reachability)
        yield ""


def gen_main(mainlevel,mutations,reachability):      
    for call in mainlevel.calls:
        yield from gen_call(call,level=mainlevel,mutations=mutations,reachability=reachability)



def writeoutput(out,mainlevel,mutations,reachability=None):
    if not reachability:
        reachability=update_uses(mainlevel,mutations=mutations)

    print(prologue,file=out)
    print("// variables used accross callbacks",file=out)
    for line in gen_globvars(mainlevel=mainlevel,reachability=reachability):
        print(line,file=out)
    print("",file=out)

    for line in gen_callbacks(mainlevel=mainlevel,mutations=mutations,reachability=reachability):
        print(line,file=out)

    print("int main() {",file=out)
    for line in gen_version(mainlevel=mainlevel):
        print(f"  {line}",file=out)
    print("",file=out)
    for line in gen_main(mainlevel=mainlevel,mutations=mutations,reachability=reachability):
        print(f"  {line}",file=out)
    print("  return EXIT_SUCCESS;",file=out)
    print("}",file=out)


def check_validity(mainlevel,mutations,reachability):
    for obj,replarg in mutations.objs_replaced.items():
        if not replarg.IsPtr:
            continue
        if not replarg.obj.IsObj:
            continue
        if obj in reachability.defined_objs:
            continue
        if replarg.obj in mutations.objs_replaced:
            continue
        return False
    return True

SKIP = NamedSentinel("SKIP")

def check_trace(mainlevel,mutations):
    reachability = update_uses(mainlevel,mutations=mutations) 
    #if not check_validity(mainlevel,mutations,reachability=reachability):
    #    return SKIP

    workdir = mkpath(tempfile.mkdtemp(prefix='isltrace'))
    cfile = workdir / 'isltrace.c'   

    cmakefile = workdir / 'CMakeLists.txt'
    cmakecontent="""cmake_minimum_required(VERSION 3.12)
project(isltrace C)

find_package(isl REQUIRED)
add_executable(isltrace isltrace.c)
target_link_libraries(isltrace PRIVATE isl)
"""

    with cmakefile.open(mode='w') as out:
        out.write(cmakecontent)

    with cfile.open(mode='w') as out:
        writeoutput(out=out,mainlevel=mainlevel,mutations=mutations,reachability=reachability)

    cmakeresult = invoke.execute('cmake', '-GNinja', '-S', '.', '-DCMAKE_BUILD_TYPE=Release', '-Disl_DIR=C:/Users/meinersbur/build/isl/release',
        cwd=workdir,onerror=invoke.Invoke.IGNORE,print_stdout=True,print_stderr=None,print_command=True,std_prefixed=[workdir/'output_cmake.txt'])
    if not cmakeresult.success:
        return False

    buildresult = invoke.execute('cmake', '--build', '.',
        cwd=workdir,onerror=invoke.Invoke.IGNORE,print_stdout=True,print_stderr=None,print_command=True,std_prefixed=[workdir/'output_ninja.txt'])
    if not buildresult.success:
        return False

    exefile = workdir / 'isltrace.exe'
    execresult = invoke.execute(exefile,
        cwd=workdir,onerror=invoke.Invoke.IGNORE,return_stdout=True,return_stderr=True,print_prefixed=True,print_command=True,std_prefixed=[workdir/'output_exe.txt'])
    return (execresult.exitcode, execresult.stdout, execresult.stderr)



class Reachability:
    def __init__(self,reachable_levels,reachable_callbackers,reachable_objs,defined_objs,globobjs):
        self.reachable_levels=reachable_levels
        self.reachable_callbackers=reachable_callbackers
        self.reachable_objs=reachable_objs
        self.defined_objs=defined_objs
        self.globobjs=globobjs


def update_uses(mainlevel,mutations):
    levels=set()
    callbackers=set()
    objs=set()
    defined=set()
    globobjs=set()
    def update_uses_arg(call,arg,lvl):        
        if arg.IsCb:
            cb = arg.callbacker
            callbackers.add(cb)
            for invok in cb.invocations:
                for arg in invok.args:
                    if not arg:
                        continue

                update_uses_level(invok)
                update_uses_arg(None,invok.retarg,invok)
        elif arg.IsVal:
            pass
        elif arg.IsPtr:
            obj = arg.obj
            if obj.IsObj and obj.name == 'node2':
                d=4

            if obj in mutations.objs_replaced:
                repl = mutations.objs_replaced[obj]
                update_uses_arg(call, repl,lvl)  
                return

            if obj and obj.IsObj:
                objs.add(obj)
                if obj.deflvl!=lvl:
                    globobjs.add(obj)
            #    obj.islocal=True
            #    obj.used=False
            #obj.use(lvl)
    def update_uses_level(lvl):
        for call in lvl.calls:
            if call.hasret and call.retobj and call.retobj.IsObj  and  call.retobj.name == 'umap28':
                c=3
            if False:
                abort()

            if call in mutations.calls_removed:
                continue
            while call in mutations.calls_replaced: 
                call = mutations.calls_replaced[call]

            if call.IsCall:
                for arg in call.args:
                    update_uses_arg(call,arg,lvl)
            if call.hasret:
                retobj = call.retobj
                if retobj and retobj.IsObj:
                    if retobj in mutations.objs_replaced:
                        continue
                    objs.add(retobj)
                    defined.add(retobj)
                    assert retobj.deflvl==lvl
                    

    update_uses_level(mainlevel)    
    return Reachability(reachable_levels=levels,reachable_callbackers=callbackers,reachable_objs=objs,defined_objs=defined,globobjs=globobjs)



class Mutations:
    def __init__(self):
        self.calls_removed=set()
        self.calls_replaced=dict()
        #self.args_replaced=dict()
        self.objs_replaced=dict()


    def clone(self):
        result = Mutations()
        result.calls_removed=self.calls_removed.copy()
        result.calls_replaced=self.calls_replaced.copy()
        #result.args_replaced=self.args_replaced.copy()
        result.objs_replaced=self.objs_replaced.copy()
        return result


def make_remove_call_closure(i,call):
    def mut_remove_call(mutations):
        result = mutations
        result.calls_removed.add(call)
        if call.hasret and call.retobj and call.retobj.IsObj:
            result.objs_replaced[call.retobj] = IslPtrArg(name=None,ty=call.retobj.ty,obj=nullobj)
        return result
    return mut_remove_call

def mut_remove_calls(mainlevel):    
    for i,call in enumerate(mainlevel.allcalls):        
        if call.IsCall:
            yield make_remove_call_closure(i,call)


def make_forward_arg_closure(call,arg):
    def mut_forward_arg(mutations):
        #mutations.calls_removed.add(call)
        mutations.objs_replaced[call.retobj] = arg
        return mutations
    return mut_forward_arg

def mut_forward_arg(mainlevel): 
    for i,call in enumerate(mainlevel.allcalls):
        if call.IsCall:
            if not (call.hasret and call.retobj and call.retobj.IsObj):
                continue
            retobj = call.retobj  
            for arg in call.args:
                if not arg.IsPtr:
                    continue
                obj = arg.obj
                if not obj.IsObj:
                    continue
                if retobj.ty != obj.ty:
                    continue
                yield make_forward_arg_closure(call,arg)

        

def make_replace_from_string_closure(call,read_fnname,ctx_argidx,ctx_fname,str): 
    def mut_replace_call(mutations):
        retobj = call.retobj
        if ctx_fname:
            replcall = IslCall(funcname=f'{read_fnname}({ctx_fname}',hasret=True,retty=call.retty,isspecial=0)
        else:
            replcall = IslCall(funcname=read_fnname,hasret=True,retty=call.retty)
        replcall.args.append(call.args[0])
        replcall.args.append(IslValArg(name='str',ty= 'char *', val=f'"{str}"' ))
        replcall.retobj = retobj # IslMemObj(name=retobj.name,ty=retobj.ty,ptr=retobj.ptr,deflvl=retobj.deflvl)

        mutations.calls_replaced[call] = replcall
        #mutations.objs_replaced[retobj] = IslPtrArg( name=None,ty=retobj.ty,obj=  replcall.retobj)
        return mutations
    return mut_replace_call



def mut_replace_from_string(mainlevel): 
    for i,call in enumerate(mainlevel.allcalls):
        if call.IsCall:
            if not (call.hasret and call.retobj and call.retobj.IsObj):
                continue
            if not call.desc:
                continue
            if call.funcname.endswith("_copy") or call.funcname.endswith("_read_from_str") :
                continue
            retobj = call.retobj
            callname = get_readfromstr_func(retobj.ty)
            if not callname:
                continue
            ctxargi = -1
            ctxargcallname = None
            for i, arg in enumerate(call.args):
                if arg.ty=='isl_ctx *' or arg.ty=='isl_ctx*':
                    ctxargi = i
                    break
                ctxargcallname = get_getctx_func(arg.ty)
                if ctxargcallname:
                    ctxargi = i
                    break
            if ctxargi==-1:
                continue            
            yield make_replace_from_string_closure(call,callname,ctxargi,ctxargcallname,call.desc)



def make_replace_id_alloc_closure(call): 
    def mut_replace_call(mutations):
        replcall = IslCall(funcname=call.funcname,hasret=True,retty=call.retty)
        replcall.args = call.args[:]
        replcall.args[2] = IslValArg(name=call.args[2].name,ty=call.args[2].ty,val='NULL')
        replcall.retobj = call.retobj
        return mutations
    return mut_replace_call


def mut_id_alloc(mainlevel): 
    for i,call in enumerate(mainlevel.allcalls):
        if not call.IsCall:
            continue
        if not call.funcname == 'isl_id_alloc':
            continue
        yield make_replace_id_alloc_closure(call)


def make_replace_all_id_alloc_closure(mainlevel): 
    def mut_replace_call(mutations):
        for mut in mut_id_alloc(mainlevel):
            mutations=mut(mutations)
        return mutations
    return mut_replace_call



def get_mutators(mainlevel):
    yield make_replace_all_id_alloc_closure(mainlevel)
    yield from mut_id_alloc(mainlevel)
    yield from mut_remove_calls(mainlevel)
    yield from mut_forward_arg(mainlevel)
    yield from mut_replace_from_string(mainlevel)




class TaskResult:
    def __init__(self,result,pos,testme):
        self.result=result
        self.pos=pos
        self.testme=testme

def make_task(data,avail,combinefn,testfn,l,pos):
    select = avail[pos:pos+l]
    testme = combinefn(data, select)
    result = testfn(testme)
    return TaskResult(result,pos,testme)


def bisect_concurrent(data, avail, combinefn, testfn):
    length = max(1, len(avail) // 32)
    while length > 0:
        def run_tasks(choose,startpos,l):
            if True:
                ex = futures.ThreadPoolExecutor(max_workers=os.cpu_count()-1) # os.cpu_count()
                tasks = [ex.submit(make_task, data,avail, combinefn, testfn, l, pos) for pos in range(startpos, len(avail), l)]
                fut = futures.as_completed(tasks)
                results = (f.result() for f in fut)
            else:
                ex = None
                results = (make_task(data, choose, combinefn, testfn, l, pos) for pos in  range(startpos, len(avail), l))
            for result in results:
                if not result.result:
                    # not interesting anymore: discard and wait for next
                    continue

                #for i in range(result.pos,result.pos+l):
                #    mut = choose[i]
                #    result.testme = mut(result.testme)

                # still interesting: stop all other tasks
                print(f"### Mutating {result.pos}..{result.pos}+{l} (of {len(choose)}) remains interesting", file=sys.stderr)
                print(f"### Contains {len(result.testme.calls_removed)} removed calls", file=sys.stderr)
                print(f"### Contains {len(result.testme.calls_replaced)} replaced calls", file=sys.stderr)
                print(f"### Contains {len(result.testme.objs_replaced)} replaced objects", file=sys.stderr)
                if ex:
                    ex.shutdown(wait=False,cancel_futures=True)
                return result
            return None

        def run_with_length(l,shuffle):
            if shuffle:
                print(f"### Trying shuffled size {l}", file=sys.stderr)
            else:
                print(f"### Trying sequential size {l}", file=sys.stderr)
            nonlocal avail,data

            choose=avail
            if shuffle:
                choose=choose.copy()
                random.shuffle(choose)

            changed = False
            startpos = 0
            while startpos < len(choose):
                if startpos > 0:
                    print(f"### Restarting at pos {startpos}", file=sys.stderr)

                result = run_tasks(choose=choose,startpos=startpos,l=l)
                if result == None:
                    # nothing more to explore
                    break

                # make permanent and continue at same pos (assuming all previous pos have failed)
                pos = result.pos
                del choose[pos:pos+l]
                data=result.testme
                startpos = pos
                changed = True

            if shuffle:
                # Reconstruct original order
                newavail=[]
                s={*choose}
                for d in avail:
                    if d in s:
                        newavail.append(d)
                avail=newavail

            return changed
             

        changed1 = run_with_length(length,shuffle=False)
        changed2 = run_with_length(length,shuffle=True)
        changed = changed1 or changed2

        #if changed:
        #    # Restart to ensure fixpoint
        #    l = len(avail)//32
        #    continue
        if length <= 1 and not changed:
            # Cannot go any smaller: reduction done
            break
        length = max(1, (length+1) // 2)

    print("Bisection done", file=sys.stderr)
    return data


def reduce_trace(mainlevel):
    mutators = [mut for mut in get_mutators(mainlevel=mainlevel)]
    if False:
        for i in range(0,len(mutators)):
            select = mutators[i:1]
            data= Mutations()
            for mut in select:
                data=mut(data)
            check_trace(mainlevel,mutations=data)

    mutations = Mutations()
    interesting = check_trace(mainlevel=mainlevel,mutations=mutations)
    if interesting==False or interesting==SKIP:
        print("Something went wrong: reference is not interesting/does not compile", file=sys.stderr)
        sys.exit(1)

    
    def combinefn(last,new):
        last = last.clone()
        for mut in new:
            last = mut(last)
        return last
    def testfn(trial):
        result = check_trace(mainlevel=mainlevel, mutations=trial)
        if result == False:
            print("Invalid", file=sys.stderr)
        return result == interesting
    cur = bisect_concurrent(data=mutations,avail=mutators,combinefn=combinefn,testfn=testfn)

    with outfile.with_suffix('_reduced.c').open(mode='w') as out:
        writeoutput(out,mainlevel=mainlevel,mutations=cur)

    if changes:
        print("changes apply. Retry from the beginning?",file=sys.stderr)
    else:
        print("fixpoint reached",file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('logfile')
    parser.add_argument('--outfile', '-o')
    parser.add_argument('--reduce', action='store_true')

    args = parser.parse_args()
    logfile = mkpath(args.logfile)
    outfile = mkpath(args.outfile)
    reduce = args.reduce
    if not outfile:
        outfile = logfile.with_suffix('.c')

    mainlevel = MainLevel()
    global callstack
    callstack = [mainlevel]

    with logfile.open(mode='r') as log:        
        for line in log:
            parse(line)
    assert len(callstack)==1


    if reduce:
        reduce_trace(mainlevel)
    else:
        with outfile.open(mode='w') as out:
            writeoutput(out,mainlevel=mainlevel,mutations=Mutations())





if __name__ == '__main__':
    main()
