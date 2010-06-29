#!/usr/bin/env python
"""Silly script colorizes scons output."""

import os
import time
import sys
import glob
import subprocess
import select

COLOR = {"green":'\x1b[32m',
         "blue":'\x1b[34m',
         "red":'\x1b[31m',
         "orange":'\x1b[33m',
         "reset":'\x1b[0m',
         "boldred":'\x1b[91m'
         }

def color_output(arguments):
    """Run scons and color the output"""
    #Build the command line
    cmdline = "scons " + arguments

    #Start the subprocess 
    p = subprocess.Popen(cmdline, shell=True, bufsize=10000,
                         cwd=".",
                         stdin=subprocess.PIPE, stderr=subprocess.STDOUT,
                         stdout=subprocess.PIPE, close_fds=True)
    (put, get) = (p.stdin, p.stdout)

    line=get.readline()
    while line != "":
        if len(line)>1:
            line = line[:-1] #Remove trailing /n
        if ("Error:" in line) or ("error:" in line) \
            or ("terminate called after throwing an instance" in line) \
            or ("Segmentation fault" in line) \
            or ("  what(): " in line) \
            or ("Assertion" in line and " failed." in line) \
            :
            #An error line!
            print COLOR["red"] + line  + COLOR["reset"]
        elif "warning:" in line or "Warning:" in line:
            #Just a warning
            print COLOR["orange"] + line + COLOR["reset"]
        elif line.startswith("g++"):
            #compiler command
            print COLOR["blue"] + line + COLOR["reset"]
        else:
            #Print normally
            print line

        #Keep reading output.
        line=get.readline()


if __name__=="__main__":
#    for x in xrange(9):
#        print ("\x1b[3%dm this is orange %d? " % (x,x)) + COLOR["reset"]
    color_output( " ".join(sys.argv[1:]) )