#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import argparse
import datetime
import os
import pprint
import re

######################################################################################################################
# Script level variables

# Compiled Regexes
# old style statement, year in group 1
regex_old_style = re.compile(
    r"\n?\s*Copyright\s.{0,20}?(\d{4}).{100,1000}?License (for) more details.*?($|(?=\*\/))"  # required section
    + r"(.*licenses(\/){0,1}\>.*?($|(?=\*\/))){0,1}"  # optional license link section
    + r"(.*mantid\>.*?($|(?=\*\/))){0,1}"  # optional change history line
    + r"(.*http://doxygen.mantidproject.org.*?($|(?=\*\/))){0,1}",
    # optional code doc line
    re.IGNORECASE | re.DOTALL | re.MULTILINE,
)
# new style statement, year in group 1
regex_new_style = re.compile(
    r"^\W*Mantid.{0,100}?(\d{4}).{100,300}?SPDX - License - Identifier.*?$[\s]*", re.IGNORECASE | re.DOTALL | re.MULTILINE
)
# Other copyright statement
regex_other_style = re.compile(r"^.*?Copyright\s.{0,200}?(\d{4}).{0,400}?$", re.IGNORECASE | re.MULTILINE)

# lines to skip when determining where to put the copyright statement (they must be from the start of the file)
regex_lines_to_skip = [
    re.compile(r"^#!.*?$[\s]*", re.MULTILINE),  # shebang line
    re.compile(r"^# -\*- coding: .+?$", re.MULTILINE),
]  # encoding definition

# Finds empty C++ multiline comments
regex_empty_comments = re.compile(r"(\/\*)[\/\s\*]{0,1000}?(\*\/)", re.MULTILINE)

# Directories to ignore - any pathss including these strings will be ignored, so it will cascade
directories_to_ignore = ["external", "CMake", "GSoapGenerated", "buildconfig"]
# Accepted file extensions
accepted_file_extensions = [".py", ".cpp", ".h", ".tcc", ".in", ".hh"]
# excluded_file_tokens
excluded_files = [".cmake.in", ".desktop.in", ".rb.in", ".py.in", "systemtest.in", "systemtest.bat.in"]
# python file exxtensions
python_file_extensions = [".py"]
# extensions to ignore, don't even report these
exts_to_ignore = [
    ".txt",
    ".pyc",
    ".sh",
    ".template",
    ".png",
    ".odg",
    ".md",
    ".doxyfile",
    ".properties",
    ".pbs",
    ".rst",
    ".md5",
    ".xml",
    ".dot",
    ".ui",
    ".jpg",
    ".png",
    ".svg",
]
# manually edit these files
manually_editable_files = [
    "Testing/SystemTests/scripts/systemtest.in",
    "Testing/SystemTests/scripts/systemtest.bat.in",
    "installers/MacInstaller/Info.plist.in",
]

# global reporting dictionaries
report_new_statements_updated = {}
report_old_statements_updated = {}
report_new_statement_added = {}
report_new_statement_current = {}
report_unrecognized_statement = {}
report_unmatched_files = {}

reporting_dictionaries = {
    "new_statements_updated.txt": report_new_statements_updated,
    "old_statements_updated.txt": report_old_statements_updated,
    "new_statements_added.txt": report_new_statement_added,
    "new_statements_current.txt": report_new_statement_current,
    "unrecognized_statement.txt": report_unrecognized_statement,
    "unmatched_files.txt": report_unmatched_files,
}


######################################################################################################################
# Functions


def get_copyright(year, comment_prefix="//"):
    """
    Creates a copyright statement as a string
    """
    return """{0} Mantid Repository : https://github.com/mantidproject/mantid
{0}
{0} Copyright &copy; {1} ISIS Rutherford Appleton Laboratory UKRI,
{0}   NScD Oak Ridge National Laboratory, European Spallation Source,
{0}   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
{0} SPDX - License - Identifier: GPL - 3.0 +""".format(comment_prefix, year)


def process_file_tree(path):
    """
    Walks the file tree processing each file
    :param path: The file path to start from
    :return: None
    """
    for dir_name, subdir_list, file_list in os.walk(path):
        print("Found directory: %s" % dir_name)
        skipdir = False
        for ignore_directory in directories_to_ignore:
            if ignore_directory in dir_name:
                print("\tignoring directory as it includes ", ignore_directory)
                skipdir = True
        if skipdir:
            # skip this directory
            continue
        for filename in file_list:
            print("\t%s" % filename)
            basename, file_extension = os.path.splitext(filename)
            if file_extension.lower() in accepted_file_extensions:
                process_file(os.path.join(dir_name, filename))
            elif file_extension.lower() in exts_to_ignore:
                print("\t\tignoring file with extension", file_extension)
            else:
                print("\t\tunknown extension", file_extension)
                report_unmatched_files[os.path.join(dir_name, filename)] = file_extension.lower()


def process_file(filename):
    """
    Processes the file, replacing the copyright statement if necessary

    Old copyright statement are replaced by new ones maintaining the date
    New copyright statements are replaced if necessary.  If the year is not present then this year is used
    Files with an unrecognised copyright statement are not altered
    Files without a statement have a new one added

    :param filename: The file to process
    :return: None
    """
    # Skip the file if it contains an excluded token
    for excluded_file in excluded_files:
        if excluded_file in filename:
            print("\t\tFile excluded by token", excluded_file)
            return

    comment_prefix = r"//"
    basename, file_extension = os.path.splitext(filename)
    if file_extension.lower() in python_file_extensions:
        comment_prefix = r"#"

    # load file text
    file_text = ""
    with open(filename, "r", encoding="utf-8") as myfile:
        file_text = myfile.read()

    # find old style statement - remove and replace
    match_old = regex_old_style.search(file_text)
    year = None
    if match_old:
        file_text = remove_text_section(file_text, match_old.start(), match_old.end())
        if comment_prefix == r"//":
            file_text = regex_empty_comments.sub("", file_text)
        year = match_old.group(1)
        print("\t\tOld statement", year)
        report_old_statements_updated[filename] = year
    else:
        # find new style statement - update if necessary
        match_new = regex_new_style.search(file_text)
        if match_new:
            year = match_new.group(1)
            if get_copyright(year, comment_prefix).strip() == match_new.group(0).strip():
                print("\t\tStatement up to date", filename)
                report_new_statement_current[filename] = year
                return
            file_text = remove_text_section(file_text, match_new.start(), match_new.end())
            if comment_prefix == r"//":
                file_text = regex_empty_comments.sub("", file_text)
            print("\t\tNew statement", year)
            report_new_statements_updated[filename] = year
        else:
            # find unrecognized statement - Leave alone, just report
            match_other = regex_other_style.search(file_text)
            if match_other:
                year = match_other.group(1)
                print("\t\tUNRECOGNIZED COPYRIGHT STATEMENT", filename)
                report_unrecognized_statement[filename] = match_other.group(0)
                return
            else:
                print("\t\tNo copyright statement found")
                year = int(datetime.datetime.now().year)
                report_new_statement_added[filename] = year

    # add the new copyright statement
    copyright_statement = get_copyright(year, comment_prefix)
    file_text = add_copyright_statement(copyright_statement, file_text)

    if not dry_run:
        # save file text
        with open(filename, "w", encoding="utf-8") as myfile:
            myfile.write(file_text)


def add_copyright_statement(copyright_statement, file_text):
    """
    Adds the new copyright statement to the top of the file,
    unless the first line is identified as a like to skip,
    in which case it will be afterwards.
    :param copyright_statement: the copyright statement to add
    :param file_text: the text of the file
    :return: the text of the file including the copyright statement
    """
    start_pos = 0
    for regex in regex_lines_to_skip:
        match = regex.match(file_text)
        if match:
            if match.end(0) > start_pos:
                start_pos = match.end(0)

    output_text = ""
    if start_pos > 0:
        output_text = file_text[:start_pos]
    output_text += copyright_statement + "\n" + file_text[start_pos:]
    return output_text


def remove_text_section(text, start, end):
    """
    Removes a section from a string
    :param text: the input string
    :param start: the start index of the section to remove
    :param end: the end section of the string to remove
    :return: the string after the section has been removed
    """
    return text[:start] + text[end:]


######################################################################################################################
# Main function

# Set up command line arguments
parser = argparse.ArgumentParser(description="Updates copyright statements in Python and C++.")
parser.add_argument("-i", "--input", type=str, help="The root path for files")
parser.add_argument("-d", "--dryrun", help="Process and report on changes without altering files", action="store_true")
parser.add_argument("-n", "--noreport", help="Suppress the writing of reporting files", action="store_true")
args = parser.parse_args()

# Handle the arguments
root_path = args.input
if root_path is None:
    root_path = "."
dry_run = args.dryrun

# Process the files
process_file_tree(root_path)

# Reporting
if not args.noreport:
    # write out reporting files
    for reporting_filename, reporting_dict in reporting_dictionaries.items():
        with open(reporting_filename, "w") as reporting_file:
            for key, value in reporting_dict.items():
                reporting_file.write("{0}\t{1}{2}".format(key, value, os.linesep))

# Final comments
print()
print("The following files must be manually edited:\n{}".format(pprint.pformat(manually_editable_files)))
