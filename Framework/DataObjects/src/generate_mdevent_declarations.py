# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Simple script that generates references to all
needed MDEvent<X>/MDLeanEvent<X> instantiations."""

import os
import datetime
import re

# List of every possible MDEvent or MDLeanEvent types.
mdevent_types = ["MDLeanEvent", "MDEvent"]

header = """/* Code below Auto-generated by '%s'
 *     on %s
 *
 * DO NOT EDIT!
 */
""" % (
    os.path.basename(__file__),
    datetime.datetime.now(),
)

footer = """
/* CODE ABOWE WAS AUTO-GENERATED BY %s - DO NOT EDIT! */
""" % (os.path.basename(__file__))


def build_macro(padding, min_dimension=1, max_dimensions=4, const=""):
    """Return the macro code CALL_MDEVENT_FUNCTION
    Parameter:
        min_dimension :: to avoid compiler warnings, limit to dimensions higher than this
        mad_dimension :: by default, maximal numner of dimesnions to be generated
        const :: set to "const " to make a const equivalent
    """
    # for the calling function macro
    macro_top = """
/** Macro that makes it possible to call a templated method for
 * a MDEventWorkspace using a IMDEventWorkspace_sptr as the input.
 *
 * @param funcname :: name of the function that will be called.
 * @param workspace :: IMDEventWorkspace_sptr input workspace.
*/

#define %sCALL_MDEVENT_FUNCTION%s(funcname, workspace) \\
{ \\
"""

    macro = """%s%sMDEventWorkspace<%s, %d>::sptr %s = std::dynamic_pointer_cast<%sMDEventWorkspace<%s, %d> >(workspace); \\
if (%s) funcname<%s, %d>(%s); \\
    """

    suffix = ""
    prefix = ""
    if min_dimension > 1:
        suffix = "%d" % min_dimension
    if const != "":
        prefix = "CONST_"
    s = macro_top % (prefix, suffix)

    for mdevent_type in mdevent_types:
        for nd in range(1, max_dimensions + 1):
            if nd >= min_dimension:
                eventType = "%s<%d>" % (mdevent_type, nd)
                varname = "MDEW_%s_%d" % (mdevent_type.upper(), nd)
                if const != "":
                    varname = "CONST_" + varname
                s += macro % (padding, const, eventType, nd, varname, const, eventType, nd, varname, eventType, nd, varname)
    s += "} \n  \n  \n"

    return s.split("\n")


def get_padding(line):
    """Return a string with the spaces padding the start of the given line."""
    out = ""
    for c in line:
        if c == " ":
            out += " "
        else:
            break
    return out


def find_line_number(lines, pattern, startat=0):
    """Look line-by-line in lines[] for a line that starts with pattern. Return
    the line number in source where the line was found, and the padding (in spaces) before it"""
    searcher = re.compile(pattern)
    for n in range(startat, len(lines)):
        found = searcher.search(lines[n])
        if found:
            # How much padding?
            padding = get_padding(lines[n])
            return (n, padding)

    return (None, None)


def find_num_dim(lines):
    """Look up through header file and the string which identifies how many dimensions have to be instantiated"""
    searcher = re.compile(r"(?<=MAX_MD_DIMENSIONS_NUM)(\s*\=\s*)\d+")
    for i in range(len(lines)):
        found = searcher.search(lines[i])
        if found:
            rez = found.group()
            return re.search(r"\d", rez).group()

    raise IOError("can not find the string which defines the number of dimensions to process")


def parse_file(file_name, start_marker, end_marker):
    """Read the file and separate it into three parts with the part between input markers to be generated and two others left unchanged.

    @param -- file_name -- full file name to open
    @param -- start_marker -- the marker which indicate first line of autogenerated file
    @param -- end_marger -- the margker which indicate last line of autogenerated file

    @return padding -- the number of spaces to insert in front of autogenerated lines
    @return lines_before -- list of lines before autogenerated one (including start_marker)
    @return lines_after  -- list of lines after autogenerated one (including end_marker)
    """
    # First, open the header and read all the lines
    f = open(file_name, "r")
    s = f.read()

    lines = s.split("\n")
    (n1, padding) = find_line_number(lines, start_marker, startat=0)
    (n2, padding_ignored) = find_line_number(lines, end_marker, startat=n1)

    print("Lines for autogenerated code: ", n1, n2)
    if n1 is None or n2 is None:
        raise Exception("Could not find the marker in the " + file_name + " file.")

    lines_before = lines[: n1 + 1]
    lines_after = lines[n2:]
    f.close()

    return (padding, lines_before, lines_after)


def generate():
    print("Generating MDEventFactory")

    # Classes that have a .cpp file (and will get an Include line)
    classes_cpp = ["MDBoxBase", "MDBox", "MDEventWorkspace", "MDGridBox", "MDBin", "MDBoxIterator"]

    padding, lines, lines_after = parse_file(
        "../inc/MantidDataObjects/MDEventFactory.h", "//### BEGIN AUTO-GENERATED CODE", "//### END AUTO-GENERATED CODE"
    )

    nDim = int(find_num_dim(lines))
    print(" numDimensions to be generated: ", nDim)
    # List of the dimensions to instantiate
    dimensions = range(1, nDim + 1)

    header_lines = map(lambda x: padding + x, header.split("\n"))
    footer_lines = map(lambda x: padding + x, footer.split("\n"))

    lines += header_lines

    macro_lines = build_macro(padding, 1, nDim) + build_macro(padding, 3, nDim) + build_macro(padding, 1, nDim, "const ")

    lines += macro_lines

    lines.append("\n")
    classes = ["MDBox", "MDBoxBase", "MDGridBox", "MDEventWorkspace", "MDBin"]
    for c in classes:
        lines.append("\n%s// ------------- Typedefs for %s ------------------\n" % (padding, c))
        mdevent_type = "MDLeanEvent"
        for nd in dimensions:
            lines.append("%s/// Typedef for a %s with %d dimension%s " % (padding, c, nd, ["", "s"][nd > 1]))
            lines.append("%s typedef %s<%s<%d>, %d> %s%dLean;" % (padding, c, mdevent_type, nd, nd, c, nd))
        mdevent_type = "MDEvent"
        for nd in dimensions:
            lines.append("%s/// Typedef for a %s with %d dimension%s " % (padding, c, nd, ["", "s"][nd > 1]))
            lines.append("%stypedef %s<%s<%d>, %d> %s%d;" % (padding, c, mdevent_type, nd, nd, c, nd))

        lines.append("\n")

    lines += footer_lines + lines_after

    f = open("../inc/MantidDataObjects/MDEventFactory.h", "w")
    for line in lines:
        f.write(line + "\n")
    f.close()

    padding, lines, lines_after = parse_file("./MDEventFactory.cpp", "//### BEGIN AUTO-GENERATED CODE", "//### END AUTO-GENERATED CODE")

    header_lines = map(lambda x: padding + x, header.split("\n"))
    footer_lines = map(lambda x: padding + x, footer.split("\n"))

    lines += header_lines

    ## MDEvent and MDLeanEvent type (just one template arg)
    for c in mdevent_types:
        lines.append("%s// Instantiations for %s" % (padding, c))
        for nd in dimensions:
            lines.append("%s template class DLLExport %s<%d>;" % (padding, c, nd))

    # Classes with MDLeanEvent<x>,x
    for c in classes_cpp:
        lines.append("%s// Instantiations for %s" % (padding, c))
        for mdevent_type in mdevent_types:
            for nd in dimensions:
                lines.append("%s template class DLLExport %s<%s<%d>, %d>;" % (padding, c, mdevent_type, nd, nd))
        lines.append("\n ")

    lines += footer_lines + lines_after
    f = open("./MDEventFactory.cpp", "w")
    for line in lines:
        f.write(line + "\n")
    f.close()

    # Post message about updating the id strings in the python layer to
    # understand the new structure
    print("")
    print("The available IDs on the templated MDEventWorkspace classes may have changed.")
    print("Please update the casting IDs in PythonInterface/mantid/api/IMDEventWorkspace accordingly")


if __name__ == "__main__":
    generate()
