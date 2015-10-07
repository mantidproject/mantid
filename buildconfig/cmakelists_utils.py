import sys
import os
import datetime
import re

#======================================================================================
def find_basedir(project, subproject):
    """ Returns the base directory. If the subproject is known to be in MantidQt or Vates, it uses that.
    The default is current dir + Framework

    Parameters
    ---------
        project : the project, Framework, MantidQt, etc.
        subproject : the subproject, Kernel, API, etc.

    Returns
    -------
         basedir = base directory
         header_folder = the folder name under the inc/ subfolder.
    """
    header_folder = "Mantid" + subproject
    if project == "MantidQt": header_folder = "MantidQt" + subproject
    scriptdir = os.path.split(__file__)[0] #Folder of Code/Build
    codedir = os.path.split(scriptdir)[0] #Folder of Code/
    basedir = os.path.join(codedir, project, subproject)
    return (basedir, header_folder)


#======================================================================================
def redo_cmake_section(lines, cmake_tag, add_this_line, remove_this_line=""):
    """ Read the LINES of a file. Find "set ( cmake_tag",
    read all the lines to get all the files,
    add your new line,
    sort them,
    rewrite. Yay!"""

    search_for1 = "set ( %s" % cmake_tag
    search_for2 = "set (%s" % cmake_tag
    # List of files in the thingie
    files = []
    lines_before = []
    lines_after = []
    section_num = 0
    for line in lines:
        if line.strip().startswith(search_for1): section_num = 1
        if line.strip().startswith(search_for2): section_num = 1

        if section_num == 0:
            # These are the lines before
            lines_before.append(line)
        elif section_num == 1:
            #this is a line with the name of a file
            line = line.strip()
            # Take off the tag
            if line.startswith(search_for1): line = line[len(search_for1):].strip()
            if line.startswith(search_for2): line = line[len(search_for2):].strip()
            # Did we reach the last one?
            if line.endswith(")"):
                section_num = 2
                line = line[0:len(line) - 1].strip()

            if len(line) > 0:
                files.append(line)
        else:
            # These are lines after
            lines_after.append(line)

    # Add the new file to the list of files
    if len(add_this_line) > 0:
        files.append(add_this_line)
    # Use a set to keep only unique linese
    files = set(files)

    # Remove an entry from the cmake list
    try:
        if len(remove_this_line) > 0:
            files.remove(remove_this_line)
    except:
        # Ignore missing entry.
        pass

    files = list(files)
    # Sort-em alphabetically
    files.sort()

    lines = lines_before
    lines.append("set ( %s" % cmake_tag)
    for file in files:
        lines.append("\t" + file)
    lines.append(")") # close the parentheses
    lines += lines_after

    return lines


#======================================================================
def fix_cmake_format(subproject):
    """ Just fix the CMAKE format"""
    cmake_path = os.path.join(os.path.curdir, "Framework/" + subproject + "/CMakeLists.txt")
    source = open(cmake_path).read()
    lines = source.split("\n");
    lines = redo_cmake_section(lines, "SRC_FILES", "")
    lines = redo_cmake_section(lines, "INC_FILES", "")
    lines = redo_cmake_section(lines, "TEST_FILES", "")

    f = open(cmake_path, 'w')
    text = "\n".join(lines)
    f.write(text)
    f.close()

#======================================================================
def fix_all_cmakes():
    """ Fix all cmake files """
    projects = ["Algorithms", "DataObjects", "MDAlgorithms", "API",
                       "Geometry", "CurveFitting", "ICat", "MDEvents",
                       "DataHandling", "Kernel", "Nexus", "Crystal"]
    for proj in projects:
        fix_cmake_format(proj)



#======================================================================
def add_to_cmake(subproject, classname, args, subfolder):
    """ Add the class to the cmake list of the given class
    Parameters:
        subproject : API, Kernel
        classname : name of the class
        args : argparse args
        subfolder : subfolder under inc and src
    """
    basedir, header_folder = find_basedir(args.project, subproject)
    cmake_path = os.path.join(basedir, "CMakeLists.txt")

    source = open(cmake_path).read()
    lines = source.split("\n");
    if args.header:
        lines = redo_cmake_section(lines, "INC_FILES", "inc/" + header_folder + "/" + subfolder + classname + ".h")
    if args.cpp:
        lines = redo_cmake_section(lines, "SRC_FILES", "src/" + subfolder + classname + ".cpp")
    if args.test:
        lines = redo_cmake_section(lines, "TEST_FILES", classname + "Test.h")

    f = open(cmake_path, 'w')
    text = "\n".join(lines)
    f.write(text)
    f.close()


#======================================================================
def remove_from_cmake(subproject, classname, args, subfolder):
    """ Removes the class from the cmake list of the given project """
    basedir, header_folder = find_basedir(args.project, subproject)
    cmake_path = os.path.join(basedir, "CMakeLists.txt")

    source = open(cmake_path).read()
    lines = source.split("\n");
    if args.header:
        lines = redo_cmake_section(lines, "INC_FILES", "",  "inc/" + header_folder + "/"+ subfolder + classname + ".h")
    if args.cpp:
        lines = redo_cmake_section(lines, "SRC_FILES", "",  "src/" + subfolder + classname + ".cpp")
    if args.test:
        lines = redo_cmake_section(lines, "TEST_FILES", "", classname + "Test.h")

    f = open(cmake_path, 'w')
    text = "\n".join(lines)
    f.write(text)
    f.close()
