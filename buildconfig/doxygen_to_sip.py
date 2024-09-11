#!/usr/bin/env python
"""Script that will grab doxygen strings from a
.cpp file and stuff them in the corresponding
.sip file under a %Docstring tag"""

import os
from optparse import OptionParser


def findInSubdirectory(filename, subdirectory=""):
    if subdirectory:
        path = subdirectory
    else:
        path = os.getcwd()
    for root, dirs, names in os.walk(path):
        if filename in names:
            return os.path.join(root, filename)
    return None


def find_cpp_file(basedir, classname):
    return findInSubdirectory(classname + ".cpp", basedir)


def grab_doxygen(cppfile, method):
    """Grab the DOXYGEN documentation string from a .cpp file
    cppfile :: full path to the .cpp file
    method :: method definition to look for
    """
    if cppfile is None:
        return None
    lines = open(cppfile, "r").read().split("\n")
    # print method
    out = []
    for i in range(len(lines)):
        line = lines[i].strip()
        if line.startswith(method):
            # Go backwards above the class to grab the doxygen
            for j in range(i - 1, -1, -1):
                out.insert(0, lines[j])
                # Stop when you reach the start of the comment "/**"
                if lines[j].strip().startswith("/**"):
                    break
            # OK return the lines
            return out
    print("WARNING: Could not find method %s" % method)
    return None


def start_python_doc_section(out, line, look_for, section_header):
    """Modify the lines by adding a section like Args

    @param out :: list of dosctring lines
    @param line :: last line read, no spaces
    @param look_for :: @string to look at
    @param section_header :: line text
    @return True if the section was added
    """
    # Add the 'Args:' line.
    if line.strip().startswith(look_for):
        # Add a blank line if there isn't one
        if len(out) > 0:
            if out[-1].strip() != "":
                out.append("    ")
        out.append(section_header)
        return True
    return False


def doxygen_to_docstring(doxygen, method):
    """Takes an array of DOXYGEN lines, and converts
    them to a more pleasing python docstring format
    @param doxygen :: list of strings containing the doxygen
    @param method :: method declaration string"""
    out = []
    if doxygen is None:
        return out
    out.append("%Docstring")
    out.append(method)
    out.append("-" * len(method))

    args_started = False
    returns_started = False
    raises_started = False

    for line in doxygen:
        line = line.strip()
        if line.startswith("/**"):
            line = line[3:]
        if line.endswith("*/"):
            line = line[:-2]
        if line.startswith("*"):
            line = line[1:]

        # Add the 'Args:' line.
        if not args_started:
            args_started = start_python_doc_section(out, line, "@param", "    Args:")

        # Add the 'Returns:' line.
        if not returns_started:
            returns_started = start_python_doc_section(out, line, "@return", "    Returns:")

        # Add the 'Raises:' line.
        if not raises_started:
            raises_started = start_python_doc_section(out, line, "@throw", "    Raises:")

        # Replace the doxygen codes with an indent
        line = line.replace("@param ", "    ")
        line = line.replace("@returns ", "    ")
        line = line.replace("@return ", "    ")
        line = line.replace("@throws ", "    ")
        line = line.replace("@throw ", "    ")

        # Make the text indented by 4 spaces
        line = "   " + line
        out.append(line)
    out.append("%End")
    out.append("")
    return out


def process_sip(filename):
    """Reads an input .sip file and finds methods from
    classes. Retrieves doxygen and adds them as
    docstrings
    @param filename :: input .sip
    @return outlines :: processed sip file, as a list of strings
    @return dirtext :: text for use in the __dir__() method
            (basically a list of the method names found).
    """

    root = os.path.split(os.path.abspath(filename))[0]
    # Read and split into a buncha lines
    lines = open(filename, "r").read().split("\n")
    i = 0
    classname = ""
    classcpp = ""
    outlines = []
    dirtext = ""

    for i in range(len(lines)):
        # Copy to output
        outlines.append(lines[i])

        line = lines[i].strip()
        if line.startswith("class "):
            classname = line[6:].strip()
            n = classname.find(":")
            if n > 0:
                classname = classname[0:n].strip()
            # Now, we look for the .cpp file
            classcpp = find_cpp_file(root, classname)
            if classcpp is None:
                print("WARNING: Could not find cpp file for class %s" % classname)
            else:
                print("Found class '%s' .cpp file " % classname)

            dirtext += "\n\n__dir__() for %s\nreturn [" % classname

        if classname != "":
            # We are within a real class
            if line.endswith(";"):
                # Within a function declaration
                n = line.find(")")
                if n > 0:
                    method = line[0 : n + 1]
                    n = method.find(" ")

                    # Find the name of the method (for the __dir__ method)
                    methodname = method[n + 1 :]
                    n2 = methodname.find("(")
                    methodname = methodname[0:n2]
                    dirtext += '"%s", ' % methodname

                    # Make the string like this:
                    # "void ClassName::method(arguments)"
                    method = method[0 : n + 1] + classname + "::" + method[n + 1 :]

                    # Now extract the doxygen
                    doxygen = grab_doxygen(classcpp, method)
                    # Convert to a docstring
                    docstring = doxygen_to_docstring(doxygen, method)
                    # And add to the output
                    outlines += docstring
    # Give back the generated lines
    return outlines, dirtext


if __name__ == "__main__":
    parser = OptionParser(
        description="""Automatically adds Docstring directives to an input .sip file.
REQUIREMENTS:
- All method declarations in the sip file must be on one line.
- The cpp file must be = to ClassName.cpp
- The method declaration must match exactly the sip entry.
- The Doxygen must be just before the method in the .cpp file.
"""
    )
    parser.add_option("-i", metavar="SIPFILE", dest="sipfile", help="The .sip input file")

    parser.add_option("-o", metavar="OUTPUTFILE", dest="outputfile", help="The name of the output file")

    parser.add_option("-d", metavar="DIRFILE", dest="dirfile", help="The name of the file containing __dir__ methods")

    (options, args) = parser.parse_args()

    if options.sipfile is None:
        raise Exception("Must specify an input file with -i !")
    if options.outputfile is None:
        raise Exception("Must specify an output file with -o !")

    print("---- Reading from %s ---- " % options.sipfile)
    (out, dirtext) = process_sip(options.sipfile)

    if options.outputfile is not None:
        print("---- Writing to %s ---- " % options.outputfile)
        f = open(options.outputfile, "w")
        f.write("\n".join(out))
        f.close()

    if options.dirfile is not None:
        print("---- Writing to %s ---- " % options.dirfile)
        f = open(options.dirfile, "w")
        f.write(dirtext)
        f.close()
