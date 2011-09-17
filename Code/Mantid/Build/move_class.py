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
    
    # Do an SVN move
    cmd = "svn mv " + oldfilename + " " + newfilename
    print "Running:", cmd
    os.system(cmd)
    
    f = open(newfilename, 'r')
    text = f.read()
    f.close()
    
    # Replace any includes of it
    text = text.replace("Mantid" + subproject + "/" + args.source_subfolder + classname + ".h", 
                        "Mantid" + newproject + "/" + args.dest_subfolder + classname + ".h");
    
    # Replace the namespace declaration
    text = text.replace("namespace " + subproject, "namespace " + newproject)
    # Replace the conents
    f = open(newfilename, 'w')
    f.write(text)
    
    

#======================================================================
def move_all(subproject, classname, newproject, newclassname, args):
    
    # Directory at base of subproject
    basedir = os.path.join(os.path.curdir, "Framework/" + subproject)
    
    newbasedir = os.path.join(os.path.curdir, "Framework/" + newproject)
    
    headerfile = os.path.join(basedir, "inc/Mantid" + subproject + "/" + args.source_subfolder + classname + ".h")
    sourcefile = os.path.join(basedir, "src/" + args.source_subfolder + classname + ".cpp")
    testfile = os.path.join(basedir, "test/" + classname + "Test.h")

    newheaderfile = os.path.join(newbasedir, "inc/Mantid" + newproject + "/" + args.dest_subfolder + newclassname + ".h")
    newsourcefile = os.path.join(newbasedir, "src/" + args.dest_subfolder + newclassname + ".cpp")
    newtestfile = os.path.join(newbasedir, "test/" + newclassname + "Test.h")
    
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
    remove_from_cmake(subproject, classname, args, args.source_subfolder)
    add_to_cmake(newproject, newclassname, args, args.dest_subfolder)
    
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
                        help='Name of the class to move')
    parser.add_argument('newproject', metavar='NEWPROJECT', type=str,
                        help='Name of the project to which to move the class.')
    parser.add_argument('newclassname', metavar='NEWCLASSNAME', type=str,
                        help='Name of the new class.')
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
    parser.add_argument('--source-subfolder', dest='source_subfolder', 
                        default="",
                        help='The source is in a subfolder below the main part of the project, e.g. Geometry/Instrument.')
    parser.add_argument('--dest-subfolder', dest='dest_subfolder', 
                        default="",
                        help='The destination is in a subfolder below the main part of the project, e.g. Geometry/Instrument.')

    args = parser.parse_args()
    subproject = args.subproject
    newproject = args.newproject
    classname = args.classname
    newclassname = args.newclassname
    overwrite = args.force
    
    # Make sure the subfolders end with a /
    if args.source_subfolder != "":
        if args.source_subfolder[-1:] != "/":
            args.source_subfolder += "/"
    
    if args.dest_subfolder != "":
        if args.dest_subfolder[-1:] != "/":
            args.dest_subfolder += "/"
    
    move_all(subproject, classname, newproject, newclassname, args)
