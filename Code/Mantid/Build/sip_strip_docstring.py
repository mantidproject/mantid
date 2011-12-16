#!/usr/bin/env python
"""Script to strip Docstring directives
from a .sip file. 
For use with old version of sip (<4.10) e.g. RHEL5 build.

This will not be needed when RHEL5 is no longer supported"""
import os
import sys
from optparse import OptionParser


    
#----------------------------------------------------------
def process_sip(filename):
    """ Reads an input .sip file and removes anything 
    in a %Docstring block"""
    
    root = os.path.split(os.path.abspath(filename))[0] 
    # Read and split into a buncha lines
    lines = open(filename, 'r').read().split('\n')
    in_docstring = False
    outlines = []
    for i in range(len(lines)):
        line = lines[i].strip().lower()
        if in_docstring:
            # Stop at the %end directive
            if line.startswith("%end"):
                in_docstring = False
        else:
            # Not in a docstring.
            if line.startswith("%docstring"):
                in_docstring = True
            # Copy to output
            if not in_docstring:
                outlines.append(lines[i])

    # Give back the generated lines
    return outlines
    
#----------------------------------------------------------
if __name__=="__main__":
    
    parser = OptionParser(description=
"""Script to strip Docstring directives from a .sip file. 
For use with old version of sip (<4.10) e.g. RHEL5 build.
This will not be needed when RHEL5 is no longer supported
""")
    parser.add_option('-i', metavar='SIPFILE', dest="sipfile",
                        help='The .sip input file')
    
    parser.add_option('-o', metavar='OUTPUTFILE', dest="outputfile",
                        help='The name of the output file')

    (options, args) = parser.parse_args()
    
    if options.sipfile is None:
        raise Exception("Must specify an input file with -i !")
    if options.outputfile is None:
        raise Exception("Must specify an output file with -o !")
    
    print "---- Stripping docstrings from %s ---- " % options.sipfile
    out = process_sip(options.sipfile)
    
    print "---- Writing to %s ---- " % options.outputfile
    f = open(options.outputfile, 'w')
    f.write('\n'.join(out))
    f.close()

    