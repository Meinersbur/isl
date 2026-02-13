#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import pathlib
import tempfile
import tool.invoke as invoke
from tool.support import *
import itertools
import math
import concurrent.futures
import threading
import time
import random
import re
import  collections



SKIP = NamedSentinel("SKIP")
MALFORMED = NamedSentinel("MALFORMED")
INTERESTING = NamedSentinel("INTERESTING")
BORING = NamedSentinel("BORING")



class ThreadPoolGeneratedQueue:
    def __init__(self,max_workers,*args,**kwargs):
        self.max_workers = max_workers
        self.stop_event = threading.Event()
        self.pool =  concurrent.futures.ThreadPoolExecutor(*args,max_workers=max_workers,**kwargs)

        # Dictionary to track active futures: {future: task_data}
        self.futures=dict()


    def submit(self,generator,workerfn):
        self.generator=generator
        self.workerfn=workerfn

        # Initial Fill: Fill the pool slots
        for _ in range(self.pool._max_workers):
            task = next(self.generator)
            self.futures[self.pool.submit(self.workerfn, *task)] = task


    def __enter__(self):
        self.pool.__enter__()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return self.pool.__exit__(exc_type,exc_val,exc_tb)

    def stop(self):
        self.stop_event.set()
        # TOOD: Ideally also  kill running gcc/isltrace processes

    def get_results(self):
        while self.futures:
            # Wait for at least one task to finish
            done, _ = concurrent.futures.wait(self.futures, return_when=concurrent.futures.FIRST_COMPLETED)

            results = []
            for fut in done:
                self.futures.pop(fut)
                results.append(fut.result())

                # Generate more tasks unless stopping
                # Can let them started before processing previous results
                if not self.stop_event.is_set():
                    try:
                        next_task = next(self.generator)
                        new_fut = self.pool.submit(self.workerfn, *next_task)
                        self.futures[new_fut] = next_task
                    except StopIteration:
                        pass # No more tasks to generate

            yield from results



_id_re = re.compile('[a-zA-Z_][a-zA-Z0-9_]+')
_stmt_call_re = re.compile(r'^\s*((?P<ltype>isl\_[a-zA-Z0-9_]+)\s*\*\s*(?P<lvar>[a-zA-Z0-9_]+)\s*\=\s*)?(?P<funcname>[a-zA-Z0-9_]+)\s*(\((?P<args>.*)\)\s*)?\;\s*$')
_dumpfile_re = re.compile(r'dump\_[a-z]+\_(?P<line>[0-9]+)\.txt')

class Tracesrc:
    class Stmt:
        def __init__(self,ltype,lvar,funcname,args):
                self.ltype = ltype
                self.lvar = lvar
                self.funcname = funcname
                self.args = args

        def __str__(self):
            if self.lvar is None:
                assert self.ltype is None
                lhs = ''
            else:
                 lhs =  f'{self.ltype} *{self.lvar} = '

            if self.args is None:
                 return f'  {lhs}{self.funcname};'
            else:
                args = ','.join(self.args)
                return f'  {lhs}{self.funcname}({args});'



    def __init__(self,lines ,occurances):
        self.lines = lines # Assumed to be the only reference / becoming the owner
        self.occurances  = occurances or dict()
        self.dumps = [None] * len(lines)


    def copy(self):
            return Tracesrc(list(self.lines), self.occurances)


    @staticmethod
    def parse_from_file(filename):
        with mkpath( filename).open('r') as f:
            lines = f.read().splitlines()
        return Tracesrc.parse(lines)

    @staticmethod
    def parse(lines):
        occurances = collections.defaultdict(lambda: 0 )
        for line in lines:
                for tok in _id_re.findall(line):
                    occurances[tok] += 1


        parsedlines = list(lines)
        for i, line in enumerate(  lines):
            m = _stmt_call_re.fullmatch(line)
            if m:
                args = m['args']
                if args is not None:
                    args =  args.split(',') if args.strip() else []
                    args = [a.strip() for a in args]
                stmt = Tracesrc.Stmt(ltype=m['ltype'],lvar = m['lvar'], funcname =m['funcname'],args=args)
                parsedlines [i] =stmt

        return Tracesrc(lines=parsedlines,occurances=occurances)




def _write_tracelines(lines, outfile, format=True ):
        with outfile.open('w+') as f:
            for line in lines.lines:
                if line is None:
                    continue
                print(line,file=f)

        if format:
            invoke.call("clang-format", '-i', '--style={BasedOnStyle: llvm, ColumnLimit: 0}', outfile, )



class CannotMutate(Exception):
     pass



def _get_mutators_unused(origlines):
    # Removed unused assignments
    for i,line in enumerate(origlines.lines):
        if not isinstance(line , Tracesrc.Stmt):
            continue
        if line.args is not None:
             continue

        if line.lvar is None:
            # no assignement, not call can be unconditionally removed
            def remove_rexpr(lines, i=i):
                    stmt = lines.lines[i]
                    if not isinstance(stmt,Tracesrc.Stmt):
                        return None
                    if not stmt.lvar is None:
                         return None
                    if  stmt.args is not None:
                         return None
                    lines.lines[i] = None
                    return lines
            yield remove_rexpr
        else:
            lvar = line.lvar
            # assignment without call: remove if not used anywhere
            if origlines.occurances.get(lvar) > 1: # One occurance is by the Tracesrc.Stmt itself
                 continue

            def remove_unused_assign(lines, i=i):
                    stmt = lines.lines[i]
                    if not isinstance(stmt,Tracesrc.Stmt):
                        return None
                    if  stmt.lvar is None:
                         return None
                    if  stmt.args is not None:
                         return None
                    lines.lines[i] = None
                    return lines
            yield remove_unused_assign



def _get_mutators_null_args(origlines):
    # replace pointer arguments with NULL
    for i,line in enumerate(origlines.lines):
            if not isinstance(line , Tracesrc.Stmt):
                continue
            if line.args is None:
                 continue
            if line.funcname == 'isl_set_read_from_str':
                 continue

            for j,arg in enumerate(line.args) :
                if arg == "NULL":
                    continue

                def delete_arg(lines,i=i,j=j):
                    stmt = lines.lines[i]
                    if not isinstance(stmt,Tracesrc.Stmt):
                        raise CannotMutate()
                    if stmt.args is None:
                        raise CannotMutate()
                    if stmt.args[j] == "NULL":
                        raise CannotMutate()
                    newargs = stmt.args.copy()

                    newargs[j] = "NULL"
                    lines.lines[i] = Tracesrc.Stmt(ltype=stmt.ltype, lvar = stmt.lvar, funcname=stmt.funcname, args=newargs )
                    return lines
                yield delete_arg


def _get_mutators_forward_assign(origlines):
    # Forward assignments to where it is used
    for i,line in enumerate(origlines.lines):
            if not isinstance(line , Tracesrc.Stmt):
                continue
            if line.args is not None:
                 continue

            def forward_assign(lines,i=i):
                    stmt = lines.lines[i]
                    if not isinstance(stmt,Tracesrc.Stmt):
                        return None
                    if stmt.args is not None:
                         raise CannotMutate()

                    lhs = stmt.lvar
                    rhs = stmt.funcname

                    for k, stmt in enumerate(lines.lines) : # FIXME: This takes a lot of time
                        if k == i:
                             continue
                        if not isinstance(stmt,Tracesrc.Stmt):
                            continue
                        if stmt.args is None:
                             if stmt.funcname == lhs:
                                  lines.lines[k] = Tracesrc.Stmt(ltype=stmt.ltype, lvar = stmt.lvar, funcname=rhs, args=None)
                        else:
                            newargs = stmt.args.copy()
                            any = False
                            for j, arg in enumerate(stmt.args):
                                if arg == lhs:
                                     newargs[j] = rhs
                                     any = True
                            if any:
                                lines.lines[k] = Tracesrc.Stmt(ltype=stmt.ltype, lvar = stmt.lvar, funcname=stmt.funcname, args=newargs)
                    lines.lines[i] = None
                    return lines
            yield forward_assign


def _get_mutators_skip_call(origlines):
    # replace call with forwaring one of its arguments
    for i,line in enumerate(origlines.lines):
        if not isinstance(line, Tracesrc.Stmt):
            continue
        if line.args is None:
            continue
        if line.funcname in ['isl_set_read_from_str', 'printf']:
            continue

        for j,arg in enumerate(line.args):
             def skip_call(lines,i=i,j=j):
                    stmt = lines.lines[i]
                    if not isinstance(stmt,Tracesrc.Stmt):
                        raise CannotMutate()
                    if stmt.args is None:
                        return None

                    arg = stmt.args[j]
                    lines.lines[i] = Tracesrc.Stmt(ltype=stmt.ltype, lvar =stmt.lvar, funcname=arg, args=None )
                    return lines
             yield skip_call



def _get_mutators_delete_lines(origlines):
    # delete lines
    for i,line in enumerate(origlines.lines):
        def delete_line(lines,i=i):
            if lines.lines[i]  is None:
                 return None
            lines.lines[i] = None
            return lines
        yield delete_line




def _get_mutators_literal_replacement(origlines):
    # replace object with a literal
    for i,line in enumerate(origlines.lines):
        thedump = origlines.dumps[i]
        if thedump is None:
              continue
        assert isinstance(line, Tracesrc.Stmt)

        def replace_with_literal(lines, i=i):
            stmt = lines.lines[i]
            if not isinstance(stmt, Tracesrc.Stmt):
                 return None
            thedump = origlines.dumps[i]

            # TODO: Escape thedump
            # TODO: Assumes there is only one isl_context and its name is ctx1
            lines.lines [i] = f'  {stmt.ltype} *{stmt.lvar} = isl_set_read_from_str(ctx1, "{thedump}");'

            return lines
        yield replace_with_literal






def _get_mutators(origlines):
    yield from _get_mutators_unused(origlines)
    yield from _get_mutators_null_args(origlines)
    yield from _get_mutators_forward_assign(origlines)
    yield from _get_mutators_skip_call(origlines)
    yield from _get_mutators_delete_lines(origlines)
    yield from _get_mutators_literal_replacement(origlines)


def _check_trace(tmpdir , lines):
    builddir = mkpath(tempfile.mkdtemp(dir=tmpdir))

    tracesrc = builddir / 'isltrace.c'
    _write_tracelines(lines, tracesrc,format=False)

    exefile  = builddir / 'isltrace'
    ccret = invoke.diag( 'gcc', '-fmax-errors=1', '-g', '-Werror=incompatible-pointer-types', '-I', '/home/meinersbur/src/isl/include', tracesrc,  '-L', '/home/meinersbur/src/isl/.libs',  '-lisl',  '-lgmp', '-o', exefile, onerror=invoke.Invoke.IGNORE, cwd=builddir)
    if not ccret.success:
        return MALFORMED, builddir

    exeret = invoke.diag(exefile, return_stdout=True, return_stderr=True,cwd=builddir)
    if not exeret.success:
        return SKIP, builddir

    if exeret.stdout != '### ISL version  : isl-0.26-GMP\n### at generation: isl-0.27-78-gfc484e00-IMath-32\n': # TOOD: Determine this from initial execution
        return SKIP, builddir

    if exeret.stderr == 'Crash expected here:\nisl_mat.c:1147: Assertion "pivot >= 0" failed\n':
        return INTERESTING, builddir

    return BORING, builddir



def _cleanup(lines):
    result = []
    for line in lines.lines:
        if line is not None:
            result.append( str(line))
    return Tracesrc.parse(result)



def _reduce(tmpdir: pathlib.Path, tracesrc: pathlib.Path,outfile: pathlib.Path):
    origlines = Tracesrc.parse_from_file(tracesrc)

    def _get_reduced_file():
        for i in itertools.count():
            reducedfilename = tmpdir / f'reduced-{i}.c'
            if not reducedfilename.exists():
                return reducedfilename


    # Initial result is original input (not applied any reduction yet)
    _write_tracelines(origlines, outfile=outfile)
    _write_tracelines(origlines, _get_reduced_file())

    originterestingness,_ =  _check_trace(tmpdir,origlines)
    if originterestingness != INTERESTING:
        print("Original program is not iteresting", file=sys.stderr)
        exit(1)




    max_workers = 16

    while True:
        def _get_next_reduced(origlines):
                # Extract string representations of variables
                dumpsrc = origlines.copy()
                for i, line in enumerate(dumpsrc.lines):
                    if not isinstance(line,Tracesrc.Stmt):
                         continue
                    if line.lvar is None:
                        continue
                    if line.ltype not in ['isl_set']:
                        continue
                    if line.funcname == "isl_set_read_from_str":
                         continue
                    dumpsrc.lines[i] = f'{line} dump_isl_set({line.lvar}, {i});'

                dumpsrc.lines[0:0] = ['''
#include <stdio.h>
#include <isl/set.h>
__isl_give isl_printer *isl_printer_set_dump(__isl_take isl_printer *p, int dump);
void dump_isl_set(isl_set *obj, int line) {
	if (!obj)
	 	return;
    char dumppath[260];
    sprintf(dumppath, "dump_set_%d.txt", line);
    FILE *f = fopen(dumppath, "w");
	isl_printer *p = isl_printer_to_file(isl_set_get_ctx(obj), f);
	p = isl_printer_set_dump(p, 1);
	p = isl_printer_print_set(p, obj);
	p = isl_printer_end_line(p);
	isl_printer_free(p);
    fclose(f);
}
''']

                dumpinterestingness,dumpdir = _check_trace(tmpdir, dumpsrc)
                assert dumpinterestingness == INTERESTING, "Even with printing it should be interesting"

                for dumpfile in dumpdir.glob("dump_*_*.txt"):
                    with dumpfile.open("r") as f:
                          dumpstr = f.read().rstrip()
                    m = _dumpfile_re.fullmatch(dumpfile.name)
                    line = int(m["line"])
                    origlines.dumps[line] = dumpstr



                def _task_generator(origlines=origlines):
                    #origlinesbck = origlines.copy()
                    mutators = list(_get_mutators(origlines))
                    n = len(mutators)

                    blocksize = 1
                    while blocksize * 2 < len(mutators):
                        blocksize *= 2

                    if False:
                        blocksize=1

                    while blocksize > 0:
                        if blocksize == 1:
                             pass
                        numblocks  =  (n+blocksize-1) //  blocksize
                        for p in range(numblocks):
                            try:
                                lines = origlines.copy()
                                start = p * blocksize
                                end = start + blocksize
                                selected_mutators = mutators[start:end]
                                changes = 0
                                for m in selected_mutators:
                                    newlines = m(lines)
                                    if newlines :
                                         lines = newlines
                                         changes += 1
                                    else:
                                        if changes == 0:
                                             pass
                                        assert changes > 0, "should not fail mutating without even a conflict"
                                assert changes
                                yield (lines,p,numblocks, len(selected_mutators))
                            except CannotMutate:
                                # Happens if mutators try to modify the same time
                                if changes == 0:
                                             pass
                                assert changes > 0, "should not fail mutating without even a conflict"
                                pass

                        blocksize = blocksize//2

                    #assert origlinesbck == origlines, "mutators must not modify original"



                q = ThreadPoolGeneratedQueue(max_workers=max_workers)
                def _worker(lines,p,numblocks,blocksize):
                        print(f"### Next job of size {blocksize} ({p+1} of {numblocks})", file=sys.stderr)
                        interestingness,builddir =  _check_trace(tmpdir,lines)
                        if interestingness == INTERESTING:
                            q.stop()
                            return lines
                        if interestingness == MALFORMED:
                            shutil.rmtree(builddir) # Avoid too much clutter
                        return None
                q.submit(generator=_task_generator(), workerfn=_worker )


                for ret in q.get_results():
                        if ret is not None:
                            q.stop()
                            return ret
                return None
                # let garbarge collector clean up the pool


        reducedlines = _get_next_reduced(origlines)
        if reducedlines is None:
            # Exhaustively tried all reductions
            break





        # (Overwrite) previous result
        _write_tracelines(reducedlines, outfile=outfile)

        # Sequence of reductions for the curious
        reducedfile = _get_reduced_file()
        _write_tracelines(reducedlines, reducedfile)
        print(f"### New reduction found: {reducedfile}" , file=sys.stderr)
        origlines = _cleanup(reducedlines)

    print("### Reduction completed" , file=sys.stderr)


def _main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('tracesrc',type=pathlib.Path)
    parser.add_argument('--outfile', '-o', type=pathlib.Path)

    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix='islreduce-') as tmpdir:
        _reduce (mkpath( tmpdir) , tracesrc=args.tracesrc,   outfile=args.outfile )



if __name__ == '__main__':
    _main()
