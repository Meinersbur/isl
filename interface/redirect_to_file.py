#! /usr/bin/env python
# -*- coding: utf-8 -*-

import subprocess
import sys

outputfile = sys.argv[1]
cmdline = sys.argv[2:]
subprocess.run(cmdline,check=True,stdout=open(outputfile,'w+'),text=True)
