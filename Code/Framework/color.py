#!/usr/bin/env python
"""Silly script colorizes output of any command line."""

import MantidBuild
import sys

if __name__=="__main__":
    MantidBuild.color_output( " ".join(sys.argv[1:]), "." )