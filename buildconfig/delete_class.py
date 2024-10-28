#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Utility for deleting a class file"""

import argparse
import os
from cmakelists_utils import remove_from_cmake


def delete_one(oldfilename):
    cmd = "git rm " + oldfilename
    print("Running:", cmd)
    os.system(cmd)


def delete_all(subproject, classname, args):
    # Directory at base of subproject
    basedir = os.path.join(os.path.curdir, "Framework/" + subproject)

    headerfile = os.path.join(basedir, "inc/Mantid" + subproject + "/" + args.source_subfolder + classname + ".h")
    sourcefile = os.path.join(basedir, "src/" + args.source_subfolder + classname + ".cpp")
    testfile = os.path.join(basedir, "test/" + classname + "Test.h")

    print()
    if args.header:
        delete_one(headerfile)
    if args.cpp:
        delete_one(sourcefile)
    if args.test:
        delete_one(testfile)

    # Insert into the cmake list
    remove_from_cmake(subproject, classname, args, args.source_subfolder)

    print("   Files were removed to Framework/{}/CMakeLists.txt !".format(subproject))
    print()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Utility to delete a Mantid class from a project. "
        "Please note, you may still have more fixes to do to get compilation!"
    )

    parser.add_argument("subproject", metavar="SUBPROJECT", type=str, help="The subproject under Framework/; e.g. Kernel")
    parser.add_argument("classname", metavar="CLASSNAME", type=str, help="Name of the class to create")
    parser.add_argument(
        "--subfolder",
        dest="source_subfolder",
        default="",
        help="The source is in a subfolder below the main part of the project, e.g. Geometry/Instrument.",
    )

    parser.add_argument("--no-header", dest="header", action="store_const", const=False, default=True, help="Don't delete the header file")
    parser.add_argument("--no-test", dest="test", action="store_const", const=False, default=True, help="Don't delete the test file")
    parser.add_argument("--no-cpp", dest="cpp", action="store_const", const=False, default=True, help="Don't delete the cpp file")

    parser.add_argument(
        "--project", dest="project", default="Framework", help="The project in which this goes. Default: Framework. Can be MantidQt"
    )

    args = parser.parse_args()
    subproject = args.subproject
    classname = args.classname

    # Make sure the subfolders end with a /
    if args.source_subfolder != "":
        if args.source_subfolder[-1:] != "/":
            args.source_subfolder += "/"

    delete_all(subproject, classname, args)
