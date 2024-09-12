#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Utility for moving a class file to a different project."""

import argparse

from cmakelists_utils import *


def move_one(subproject, classname, newproject, newclassname, oldfilename, newfilename, args):
    """Move one file"""
    # Move the file
    try:
        cmd = "mv " + oldfilename + " " + newfilename
        cmd = cmd.replace("\\", "/")
        if not args.no_vcs:
            cmd = "git " + cmd
        print("Running:", cmd)
        retval = os.system(cmd)
        if retval != 0:
            raise RuntimeError("Error executing cmd '{}'".format(cmd))

        f = open(newfilename, "r")
        text = f.read()
        f.close()

        # Replace any includes of it
        text = text.replace(
            "Mantid" + subproject + "/" + args.source_subfolder + classname + ".h",
            "Mantid" + newproject + "/" + args.dest_subfolder + newclassname + ".h",
        )

        # Replace the guard
        old_guard = "MANTID_{}_{}_H_".format(subproject.upper(), classname.upper())
        new_guard = "MANTID_{}_{}_H_".format(newproject.upper(), newclassname.upper())
        text = text.replace(old_guard, new_guard)

        # Replace the namespace declaration
        text = text.replace("namespace " + subproject, "namespace " + newproject)
        # Replace the contents
        f = open(newfilename, "w")
        f.write(text)
    except RuntimeError as err:
        print(err)


def move_all(subproject, classname, newproject, newclassname, args):
    # Directory at base of subproject
    basedir, header_folder = find_basedir(args.project, subproject)
    newbasedir, new_header_folder = find_basedir(args.project, newproject)

    headerfile = os.path.join(basedir, "inc/" + header_folder + "/" + args.source_subfolder + classname + ".h")
    sourcefile = os.path.join(basedir, "src/" + args.source_subfolder + classname + ".cpp")
    testfile = os.path.join(basedir, "test/" + classname + "Test.h")

    newheaderfile = os.path.join(newbasedir, "inc/" + new_header_folder + "/" + args.dest_subfolder + newclassname + ".h")
    newsourcefile = os.path.join(newbasedir, "src/" + args.dest_subfolder + newclassname + ".cpp")
    newtestfile = os.path.join(newbasedir, "test/" + args.dest_subfolder + newclassname + "Test.h")

    if args.header and not overwrite and os.path.exists(newheaderfile):
        print("\nError! Header file {} already exists. Use --force to overwrite.\n".format(newheaderfile))
        return
    if args.cpp and not overwrite and os.path.exists(newsourcefile):
        print("\nError! Source file {} already exists. Use --force to overwrite.\n".format(newsourcefile))
        return
    if args.test and not overwrite and os.path.exists(newtestfile):
        print("\nError! Test file {} already exists. Use --force to overwrite.\n".format(newtestfile))
        return

    print()
    if args.header:
        move_one(subproject, classname, newproject, newclassname, headerfile, newheaderfile, args)
    if args.cpp:
        move_one(subproject, classname, newproject, newclassname, sourcefile, newsourcefile, args)
    if args.test:
        move_one(subproject, classname, newproject, newclassname, testfile, newtestfile, args)

    # Insert into the cmake list
    remove_from_cmake(subproject, classname, args, args.source_subfolder)
    add_to_cmake(newproject, newclassname, args, args.dest_subfolder)

    print("   Files were removed to Framework/{}/CMakeLists.txt !".format(subproject))
    print("   Files were added to Framework/{}/CMakeLists.txt !".format(newproject))
    print()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Utility to move a Mantid class from one project to another. "
        "Please note, you may still have more fixes to do to get compilation!"
    )
    parser.add_argument("subproject", metavar="SUBPROJECT", type=str, help="The subproject under Framework/; e.g. Kernel")
    parser.add_argument("classname", metavar="CLASSNAME", type=str, help="Name of the class to move")
    parser.add_argument("newproject", metavar="NEWPROJECT", type=str, help="Name of the project to which to move the class.")
    parser.add_argument("newclassname", metavar="NEWCLASSNAME", type=str, help="Name of the new class.")
    parser.add_argument(
        "--force",
        dest="force",
        action="store_const",
        const=True,
        default=False,
        help="Force overwriting existing files. Use with caution!",
    )
    parser.add_argument(
        "--no-vcs",
        dest="no_vcs",
        action="store_const",
        const=True,
        default=False,
        help="Can be used to move a class that is not yet under version control. Default: False",
    )
    parser.add_argument("--no-header", dest="header", action="store_const", const=False, default=True, help="Don't move the header file")
    parser.add_argument("--no-test", dest="test", action="store_const", const=False, default=True, help="Don't move the test file")
    parser.add_argument("--no-cpp", dest="cpp", action="store_const", const=False, default=True, help="Don't move the cpp file")
    parser.add_argument(
        "--source-subfolder",
        dest="source_subfolder",
        default="",
        help="The source is in a subfolder below the main part of the project, e.g. Geometry/Instrument.",
    )
    parser.add_argument(
        "--dest-subfolder",
        dest="dest_subfolder",
        default="",
        help="The destination is in a subfolder below the main part of the project, e.g. Geometry/Instrument.",
    )
    parser.add_argument(
        "--project", dest="project", default="Framework", help="The project in which this goes. Default: Framework. Can be MantidQt"
    )

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
