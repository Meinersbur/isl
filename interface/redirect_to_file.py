#! /usr/bin/env python
# -*- coding: utf-8 -*-

import subprocess
import sys
import argparse

parser = argparse.ArgumentParser(allow_abbrev=False)
parser.add_argument('output')
parser.add_argument('--prepend',action='append',default=[])
parser.add_argument('--append',action='append',default=[])

args,cmdline = parser.parse_known_intermixed_args()

with open(args.output,'w+') as fd:
    for pre in args.prepend:
        with open(pre,'r') as pfd:
            fd.write(pfd.read())

    p = subprocess.run(cmdline,check=True,stdout=subprocess.PIPE,text=True)
    fd.write(p.stdout)

    for app in args.append:
        with open(app,'r') as pfd:
            fd.write(pfd.read())
