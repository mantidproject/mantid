#!/usr/bin/python

import sys

def abort( problem ):
    '''Print error message and exit'''
    sys.stderr.write( '\n' )
    sys.stderr.write( problem )
    sys.stderr.write( '\n\n' )
    sys.exit(2)

#
# Copyright 2008 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
# retains certain rights in this software.
#

