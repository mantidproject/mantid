#!/usr/bin/env python

import subprocess as sub
import argparse
import os
import sys
import shlex

mantid_initialized = False

#======================================================================
def get_all_algorithms():
    """REturns a list of all algorithm names"""
    temp = mtd._getRegisteredAlgorithms(True)
    algos = []
    for (x, versions) in temp:
        for v in versions:
            if len(versions) > 1:
                algos.append( x + str(v) )
            else:
                algos.append( x )
    print "\n".join(algos)
    return algos

#======================================================================
def initialize_Mantid():
    """ Start mantid framework """
    global mtd
    global mantid_initialized
    if mantid_initialized:   return
    sys.path.append(os.getcwd())
    sys.path.append( os.path.join( os.getcwd(), 'bin') )
    sys.path.append( os.path.join( os.getcwd(), '../dbg/bin') )
    import MantidFramework
    from MantidFramework import mtd
    mtd.initialise()
    mantid_initialized = True

#======================================================================
if __name__ == "__main__":
    global mtd
    
    parser = argparse.ArgumentParser(description="Utility to list all algorithms written by yourself. Requires: ack-grep. " + \
                                     "You can use this output as the arguments to auto_wiki.py to quickly generate all " + \
                                     "your wiki pages. Fun!")  
    
    parser.add_argument('name', metavar='NAME', type=str,
                        help='Your name, to search for. Can be a regular expression')
    
    args = parser.parse_args()
    name = args.name
 
    search = "@author %s" % name
    print 'Searching in files for: "%s"' % search
    
    args = shlex.split('ack-grep -l "%s"' % search)
    p = sub.Popen(args, stdout=sub.PIPE,stderr=sub.PIPE)
    output, errors = p.communicate()
    
    files = output.split('\n')
    
    print len(files), "files were found."
    print 
    
    initialize_Mantid()
    all_algos = get_all_algorithms()
    
    my_algos = set()
    
    for file in files:
        name = os.path.basename(file)
        name = name.replace(".h", "")
        name = name.replace(".cpp", "")
        if name in all_algos:
            my_algos.add(name)
        if name+'1' in all_algos:
            my_algos.add(name+'1')
    
    my_algos = list(my_algos)
    my_algos.sort()        
    
    print
    print "You are the author of %d algorithms." % len(my_algos)
    print
    print " ".join(my_algos)
    