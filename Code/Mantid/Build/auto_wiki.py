#!/usr/bin/env python
import argparse
import os


def do_algorithm(args):
    algo = args.algo
    os.system("bin/WikiMaker " + algo + " wiki.txt")
    
    f = open('wiki.txt', 'r')
    contents = f.read()
    f.close()
    
    print contents

#======================================================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate the Wiki documentation for a '
                                      'particular algorithm and set it in the text.')
    parser.add_argument('algo', metavar='ALGORITHM', type=str,
                        help='The subproject under Framework/; e.g. Kernel')

    args = parser.parse_args()
    
    do_algorithm(args)