#!/usr/bin/env python
"""Silly script colorizes scons output."""

import MantidBuild
import sys

if __name__=="__main__":
    MantidBuild.color_output( "scons " + " ".join(sys.argv[1:]), "." )