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
    in a %Docstring block.

    @param filename :: .sip file to read
    @return outlines :: processed sip file
    @return wiki :: docstring lines only
    """

    root = os.path.split(os.path.abspath(filename))[0]
    # Read and split into a buncha lines
    lines = open(filename, 'r').read().splitlines()
    in_docstring = False
    outlines = []
    wikilines = []
    for i in range(len(lines)):
        line = lines[i].strip().lower()
        if in_docstring:
            # Stop at the %end directive
            if line.startswith("%end"):
                in_docstring = False
            else:
                # Save the docstring
                wikilines.append(" " + lines[i])
                # Class-separator line = this is the second line of a new class docstring
                if line.startswith("==="):
                    # Make a wiki-markup header
                    wikilines[-1] = " "
                    wikilines[-2] = "=== %s ===" % (wikilines[-2].strip())
        else:
            # Not in a docstring.
            if line.startswith("%docstring"):
                in_docstring = True
                # Add a blank line separating doc strings if not present
                if len(wikilines) > 0 and wikilines[-1] != "":
                    wikilines.append("")
            # Copy to output
            if not in_docstring:
                outlines.append(lines[i])
                if line.startswith("//*wiki*"):
                    # Manually add wiki markup
                    s = lines[i].strip()
                    s = s[8:].strip()
                    wikilines.append(s)

    # Give back the generated lines
    return (outlines, wikilines)

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

    parser.add_option('-w', metavar='WIKIFILE', dest="wikifile",
                        help='The name of the file containing wiki text for documenting')



    (options, args) = parser.parse_args()

    if options.sipfile is None:
        raise Exception("Must specify an input file with -i !")
    if options.outputfile is None:
        raise Exception("Must specify an output file with -o !")

    print "---- Stripping docstrings from %s ---- " % options.sipfile
    (out, wiki) = process_sip(options.sipfile)

    print "---- Writing to %s ---- " % options.outputfile
    f = open(options.outputfile, 'w')
    f.write('\n'.join(out))
    f.close()

    if not options.wikifile is None:
        print "---- Writing wiki text to %s ---- " % options.wikifile
        f = open(options.wikifile, 'w')
        f.write('\n'.join(wiki))
        f.close()

