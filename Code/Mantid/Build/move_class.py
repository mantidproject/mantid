#!/usr/bin/env python
""" Utility for moving a class file to a different project."""

import sys
import os
import argparse
import datetime
import re
from cmakelists_utils import *


    
#======================================================================
def move_one(subproject, classname, newproject, oldfilename, newfilename, args):
    """Move one file """
    text = open(oldfilename, 'r').read()
    # Replace any includes of it
    text = text.replace("Mantid" + subproject + "/" + classname + ".h", "Mantid" + newproject + "/" + classname + ".h");
    
    # Replace the namespace declaration
    text = text.replace("namespace " + subproject, "namespace " + newproject) 
    
    f = open(newfilename, 'w')
    f.write(text)
    
    # Delete original?
    if args.delete:
        os.remove(oldfilename)
    

#======================================================================
def move_all(subproject, classname, newproject, args):
    
    # Directory at base of subproject
    basedir = os.path.join(os.path.curdir, "Framework/" + subproject)
    
    newbasedir = os.path.join(os.path.curdir, "Framework/" + newproject)
    
    headerfile = os.path.join(basedir, "inc/Mantid" + subproject + "/" + classname + ".h")
    sourcefile = os.path.join(basedir, "src/" + classname + ".cpp")
    testfile = os.path.join(basedir, "test/" + classname + "Test.h")
    
    newheaderfile = os.path.join(newbasedir, "inc/Mantid" + newproject + "/" + classname + ".h")
    newsourcefile = os.path.join(newbasedir, "src/" + classname + ".cpp")
    newtestfile = os.path.join(newbasedir, "test/" + classname + "Test.h")
    
    if args.header and not overwrite and os.path.exists(newheaderfile):
        print "\nError! Header file %s already exists. Use --force to overwrite.\n" % newheaderfile
        return
    if args.cpp and not overwrite and os.path.exists(newsourcefile):
        print "\nError! Source file %s already exists. Use --force to overwrite.\n" % newsourcefile
        return
    if args.test and not overwrite and os.path.exists(newtestfile):
        print "\nError! Test file %s already exists. Use --force to overwrite.\n" % newtestfile
        return
      
    print
    if args.header:
        move_one(subproject, classname, newproject, headerfile, newheaderfile, args)
    if args.cpp:
        move_one(subproject, classname, newproject, sourcefile, newsourcefile, args)
    if args.test:
        move_one(subproject, classname, newproject, testfile, newtestfile, args)
    
    # Insert into the cmake list
    add_to_cmake(newproject, classname, args)
    remove_from_cmake(subproject, classname, args)
    
    print "   Files were removed to Framework/%s/CMakeLists.txt !" % subproject
    print "   Files were added to Framework/%s/CMakeLists.txt !" % newproject
    print 
#    if not test_only:
#        print "\tsrc/%s.cpp" % (classname)
#        print "\tinc/Mantid%s/%s.h" % (subproject, classname)
#    print "\ttest/%sTest.h" % (classname)
#    print  



#======================================================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Utility to move a Mantid class from one project to another. ' 
                                     'Please note, you may still have more fixes to do to get compilation!')
    parser.add_argument('subproject', metavar='SUBPROJECT', type=str,
                        help='The subproject under Framework/; e.g. Kernel')
    parser.add_argument('classname', metavar='CLASSNAME', type=str,
                        help='Name of the class to create')
    parser.add_argument('newproject', metavar='NEWPROJECT', type=str,
                        help='Name of the project to which to move the class.')
    parser.add_argument('--force', dest='force', action='store_const',
                        const=True, default=False,
                        help='Force overwriting existing files. Use with caution!')
    parser.add_argument('--no-header', dest='header', action='store_const',
                        const=False, default=True,
                        help="Don't move the header file")
    parser.add_argument('--no-test', dest='test', action='store_const',
                        const=False, default=True,
                        help="Don't move the test file")
    parser.add_argument('--no-cpp', dest='cpp', action='store_const',
                        const=False, default=True,
                        help="Don't move the cpp file")
    parser.add_argument('--delete', dest='delete', action='store_const',
                        const=True, default=False,
                        help="Delete the original files (default False)")
     
    args = parser.parse_args()
    subproject = args.subproject
    newproject = args.newproject
    classname = args.classname
    overwrite = args.force
    
    move_all(subproject, classname, newproject, args)
