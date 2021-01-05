#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import subprocess
import io
import threading
import datetime
import contextlib
import pathlib
from tool.support import *


class Invoke:
    # If called program fails, throw an exception
    EXCEPTION = NamedSentinel('EXCEPTION')

    # If called program fails, exit with same error code
    ABORT = NamedSentinel('ABORT')

    # Like ABORT, but also print that the program has failed
    ABORT_EXITCODE = NamedSentinel('ABORT_EXITCODE')

    # If called program fails, ignore or return error code
    IGNORE = NamedSentinel('IGNORE')


    @staticmethod
    def hlist(arg):
        if arg is None:
            return []
        if isinstance(arg, str):  # A filename
            return [arg]
        return list(arg)


    @staticmethod
    def assemble_env(env=None, setenv=None, appendenv=None):
        if env == None and setenv == None and appendenv ==None:
            return None
        if env is None:
            env = dict(os.environ)
        if setenv is not None:
            for key, val in setenv.items():
                env[str(key)] = str(val)
        if appendenv is not None:
            for key, val in appendenv.items():
                if not val:
                    continue
                oldval = env.get(key)
                if oldval:
                    env[key] = oldval + ':' + val
                else:
                    env[key] = val
        return env


    def __init__(self, cmd, *args, cwd=None, setenv=None, appendenv=None, stdout=None, stderr=None, std_joined=None, std_prefixed=None):
        self.cmdline = [str(cmd)] + [str(a) for a in args]
        self.cwd = cwd
        self.setenv = setenv
        self.appendenv = appendenv

        self.stdout = Invoke.hlist(stdout)
        self.stderr = Invoke.hlist(stderr)
        self.std_joined = Invoke.hlist(std_joined)
        self.std_prefixed = Invoke.hlist(std_prefixed)


    def cmd(self):
        # FIXME: Windows
        shortcmd = self.cmdline[0]
        args = self.cmdline[1:]
        envs = []
        if self.setenv is not None:
            for envkey, envval in self.setenv.items():
                envs += [envkey + '=' + shquote(envval)]
        if self.appendenv is not None:
            for envkey, envval in self.appendenv.items():
                if not envval:
                    continue
                envs += [envkey + '=${' + envkey + '}:' + shquote(envval)]
        if envs:
            env = ' '.join(envs)+' '
        else:
            env = ''
        if self.cwd is None:
            result = env + shortcmd + (' ' if args else '') + shjoin(args)
        else:
            result = '(cd ' + shquote(self.cwd) + ' && ' + env + \
                shortcmd + (' ' if args else '') + shjoin(args) + ')'
        return result


    class InvokeResult:
        def __init__(self, exitcode, stdout, stderr, joined, prefixed, walltime):
            self.exitcode = exitcode
            self.stdout = stdout
            self.stderr = stderr
            self.joined = joined
            self.prefixed = prefixed
            self.walltime = walltime

        @property
        def success(self):
            return self.exitcode == 0

        @property
        def output(self):
            return self.prefixed or self.joined or self.stdout or self.stderr


    def execute(self, onerror=EXCEPTION,
                print_stdout=False, print_stderr=False, print_prefixed=False, print_command=False, print_exitcode=False,
                return_stdout=False, return_stderr=False, return_joined=False, return_prefixed=False,
                stdout=None, stderr=None, std_joined=None, std_prefixed=None, forcepopen=False):

      cmdline = [str(s) for s in self.cmdline]
      cwd = None if (self.cwd is None) else str(self.cwd)
      env = Invoke.assemble_env(setenv=self.setenv, appendenv=self.appendenv) if self.setenv or self.appendenv else None
      stdout = self.stdout + Invoke.hlist(stdout)
      stderr = self.stderr + Invoke.hlist(stderr)
      std_joined = self.std_joined + Invoke.hlist(std_joined)
      std_prefixed = self.std_prefixed + Invoke.hlist(std_prefixed)
          
      stdout_print_xand_return = not (print_stdout and return_stdout)
      stderr_print_xand_return = not (print_stderr and return_stderr)

      def execute_run():
          if print_command:
              sys.stdout.flush()
              print('$',self.cmd(), file=sys.stderr)
              sys.stderr.flush()
          # To print real-time, mode must be none
          stdout_mode = None if print_stdout else subprocess.PIPE
          stderr_mode = None if print_stderr else subprocess.PIPE

          start = datetime.datetime.now()
          ret = subprocess.run(cmdline, cwd=cwd, env=env, check=(onerror==Invoke.EXCEPTION), stdout=stdout_mode, stderr=stderr_mode, universal_newlines=True)
          stop = datetime.datetime.now()
          walltime = stop - start

          exitcode = ret.returncode
          if exitcode:
                if onerror == Invoke.ABORT_EXITCODE:
                    abortmsg = "Command failed with code {rtncode}\n{command}".format(rtncode=exitcode, walltime=walltime, command=self.cmd())
                    print(abortmsg, file=sys.stderr)
                    exit(exitcode)
                if onerror == Invoke.ABORT:
                    # Treat as if the error was raised by this python program
                    exit(exitcode)

          if print_exitcode:
                exitmsg = "Exit with code {exitcode} in {walltime}".format(exitcode=exitcode, walltime=walltime)
                sys.stdout.flush()
                print(exitmsg, file=sys.stderr)
                sys.stderr.flush()

          stdout_str = ret.stdout if return_stdout else None
          stderr_str = ret.stderr if return_stderr else None
          return Invoke.InvokeResult(exitcode=exitcode, stdout=stdout_str, stderr=stderr_str, joined= None, prefixed=None, walltime=walltime)


      def execute_popen():
        stdin = None

        if print_command or return_prefixed or std_prefixed:
            command = self.cmd()

        stdouthandles = []
        stderrhandles = []
        prefixedhandles = []
        with contextlib.ExitStack() as stack:
            if print_stdout:
                stdouthandles.append(sys.stdout)
            if print_stderr:
                stderrhandles.append(sys.stderr)
            if print_prefixed:
                prefixedhandles.append(sys.stdout)

            result_stdout = None
            if return_stdout:
                result_stdout = io.StringIO()
                stdouthandles.append(result_stdout)
            result_stderr = None
            if return_stderr:
                result_stderr = io.StringIO()
                stderrhandles.append(result_stderr)
            result_joined = None
            if return_joined:
                result_joined = io.StringIO()
                stdouthandles.append(result_joined)
                stderrhandles.append(result_joined)
            result_prefixed = None
            if return_prefixed:
                result_prefixed = io.StringIO()
                prefixedhandles.append(result_prefixed)

            def open_or_handle(arg):
                if isinstance(arg, str) or isinstance(arg, pathlib.Path):
                    return stack.enter_context(open(file, 'w')) # Finally close
                return arg

            for file in stdout:
                stdouthandles.append(open_or_handle(file))
            for file in stderr:
                stderrhandles.append(open_or_handle(file))
            for file in std_joined:
                h = open_or_handle(file)
                stdouthandles.append(h)
                stderrhandles.append(h)
            for file in std_prefixed:
                prefixedhandles.append(open_or_handle(file))

            def flush_all():
                # Assuming that only stdout/stderr might point to the same targets, but not joined or prefixed
                for h in stdouthandles:
                    h.flush()
                for h in stderrhandles:
                    h.flush()

            flush_all()
            for h in prefixedhandles:
                print('$',command, file=h)
            if print_command:
                for h in prefixedhandles:
                    print('$',command, file=h)
                    h.flush()

            stdout_mode = subprocess.PIPE if stdouthandles or prefixedhandles else None
            stderr_mode = subprocess.PIPE if stderrhandles or prefixedhandles else None

            errmsg = None
            start = datetime.datetime.now()

            if onerror == Invoke.EXCEPTION:
                # Just forward if any exception
                p = subprocess.Popen(cmdline, cwd=cwd, env=env, stdout=stdout_mode, stderr=stderr_mode, universal_newlines=True, bufsize=1)
            else:
                try:
                    # bufize=1 ensures line buffering
                    p = subprocess.Popen(cmdline, cwd=cwd, env=env, stdout=stdout_mode, stderr=stderr_mode, universal_newlines=True, bufsize=1)
                except Exception as err:
                    # Exception can happen if e.g. the executable does not exist
                    # Process it further down
                    p = None
                    exitcode = 127  # Not sure whether this is sh/Linux-specific
                    errmsg = "Invocation error: {err}".format(err=err)

            if p:
                def catch_std(std, outhandles, prefix, prefixedhandles):
                    while True:
                        try:
                            line = std.readline()
                        except ValueError as e:
                            # TODO: Handle properly
                            print("Input prematurely closed")
                            break

                        if line is None or len(line) == 0:
                            break
                        for h in outhandles:
                            print(line, end='', file=h)
                            h.flush()
                        for h in prefixedhandles:
                            print(prefix, line, sep='', end='', file=h)


                tout = None
                if stdout_mode == subprocess.PIPE:
                    tout = threading.Thread(target=catch_std,
                                            kwargs={'std': p.stdout, 'outhandles': stdouthandles, 'prefix': "[stdout] ", 'prefixedhandles': prefixedhandles})
                    tout.daemon = True
                    tout.start()

                terr = None
                if stderr_mode == subprocess.PIPE:
                    terr = threading.Thread(target=catch_std,
                                            kwargs={'std': p.stderr, 'outhandles': stderrhandles, 'prefix': "[stderr] ", 'prefixedhandles': prefixedhandles})
                    terr.daemon = True
                    terr.start()

                if stdin != None:
                    p.communicate(input=stdin)

                exitcode = p.wait()

            stop = datetime.datetime.now()
            walltime = stop - start

            if p:
                if tout:
                    tout.join()
                    try:
                        p.stdout.close()
                    except:
                        # Stale file handle possible
                        # FIXME: why?
                        pass

                if terr:
                    terr.join()
                    try:
                        p.stderr.close()
                    except:
                        # Stale file handle possible
                        # FIXME: Why?
                        pass

            if errmsg:
                exitmsg = "Invocation failed in {walltime}: {errmsg}".format(exitcode=exitcode, walltime=walltime, errmsg=errmsg)
                abortmsg = "Command failed with code {rtncode}\n{errmsg}\n{command}".format(rtncode=exitcode, walltime=walltime, command=self.cmd(), errmsg=errmsg)
            else:
                exitmsg = "Exit with code {exitcode} in {walltime}".format(exitcode=exitcode, walltime=walltime)
                abortmsg = "Command failed with code {rtncode}\n{command}".format(rtncode=exitcode, walltime=walltime, command=self.cmd())

            for h in prefixedhandles:
                print(exitmsg.format(rtncode=exitcode), file=h)

            if exitcode:
                if onerror == Invoke.ABORT_EXITCODE:
                    flush_all()
                    for h in stderrhandles:
                        print(abortmsg,file=h)
                        h.flush()
                    exit(exitcode)
                if onerror == Invoke.ABORT:
                    # Treat as if the error was raised by this python program
                    exit(exitcode)
                elif onerror is Invoke.EXCEPTION:
                    # Allow application to handle this error
                    raise subprocess.CalledProcessError(returncode=exitcode, cmd=shjoin(cmdline))

            if print_exitcode:
                for h in prefixedhandles:
                    exitmsg = "Exit with code {exitcode} in {walltime}".format(exitcode=exitcode, walltime=walltime)
                    print(exitmsg, file=h)

            if isinstance(result_stdout, io.StringIO):
                result_stdout = result_stdout.getvalue()
            if isinstance(result_stderr, io.StringIO):
                result_stderr = result_stderr.getvalue()
            if isinstance(result_joined, io.StringIO):
                result_joined = result_joined.getvalue()
            if isinstance(result_prefixed, io.StringIO):
                result_prefixed = result_prefixed.getvalue()

        return Invoke.InvokeResult(exitcode=exitcode, stdout=result_stdout, stderr=result_stderr, joined=result_joined, prefixed=result_prefixed, walltime=walltime)

      if not forcepopen \
                and not stdout and not stderr and not std_joined and not std_prefixed \
                and stdout_print_xand_return and stderr_print_xand_return \
                and not print_prefixed \
                and not return_joined and not return_prefixed \
                and not stdout and not stderr and not std_joined and not std_prefixed:
          return execute_run()
      return execute_popen()


    # Execute as if this is the command itself
    def run(self, onerror=None, print_stdout=True, print_stderr=True, **kwargs):
        return self.execute(onerror=first_defined(onerror, Invoke.ABORT), print_stdout=print_stdout, print_stderr=print_stderr, **kwargs).exitcode


    # Diagnostic mode, execute with additional info
    def diag(self, onerror=None, print_stdout=False, print_stderr=False, print_prefixed=True, print_command=True, print_exitcode=True, **kwargs):
        return self.execute(onerror=first_defined(onerror, Invoke.IGNORE), print_stdout=print_stdout, print_stderr=print_stderr, print_prefixed=print_prefixed, print_exitcode=print_exitcode, **kwargs)


    # Execute to get the command's result
    def query(self, onerror=None, **kwargs):
        return self.execute(onerror=first_defined(onerror, Invoke.EXCEPTION), return_stdout=True, **kwargs).stdout


    def call(self, onerror=None, **kwargs):
        return self.execute(onerror=first_defined(onerror, Invoke.EXCEPTION), **kwargs)


def execute(cmd, *args, cwd=None, setenv=None, appendenv=None, **kwargs):
    return Invoke(cmd, *args, cwd=cwd, setenv=setenv, appendenv=appendenv).execute(**kwargs)


def run(cmd, *args, cwd=None, setenv=None, appendenv=None, **kwargs):
    return Invoke(cmd, *args,  cwd=cwd, setenv=setenv, appendenv=appendenv).run(**kwargs)


def query(cmd, *args, cwd=None, setenv=None, appendenv=None, **kwargs):
    return Invoke(cmd, *args,  cwd=cwd, setenv=setenv, appendenv=appendenv).query(**kwargs)


def call(cmd, *args, cwd=None, setenv=None, appendenv=None, **kwargs):
    return Invoke(cmd, *args,  cwd=cwd, setenv=setenv, appendenv=appendenv).call(**kwargs)


def gittool_taskset(cmd):
    pid = os.gitpid()
    psout = query('ps', '-o', 'psr', '-p', pid)
    pslines = psout.splitlines()
    tid = int(pslines[1])
    print("Running on core", tid)
    invoke('taskset', '-c', tid, *cmd)

