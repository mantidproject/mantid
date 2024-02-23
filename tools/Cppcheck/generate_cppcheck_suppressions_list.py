#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Accepts a cppcheck.xml file and generates a list of suppressions to add to
the CppCheck_Suppressions.txt.in template file. Use this when upgrading to
a new version of cppcheck.
"""

import argparse
import sys
import xml.etree.ElementTree as ET
from typing import List

NEW_SOURCE_ROOT = "${CMAKE_SOURCE_DIR}"
OLD_SOURCE_ROOT = "/jenkins_workdir/workspace/pull_requests-cppcheck"


def main() -> int:
    """
    Main entry point for the program.
    """
    args = parse_arguments()
    old_source_root = args.path_to_source if args.path_to_source else OLD_SOURCE_ROOT
    tree = ET.parse(args.cppcheck_xml)
    suppressions = generate_suppressions(tree, old_source_root)

    with open(args.outfile, "w") as f:
        f.write("\n".join(suppressions))

    return 0


def parse_arguments() -> argparse.Namespace:
    """
    Process command-line arguments from sys.argv
    :return: An argparse.Namespace containing the arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--cppcheck_xml", type=str, help="An xml file containing a list of cppcheck defects.", required=True)
    parser.add_argument("--outfile", type=str, help="Name of file to write the cppcheck suppressions to.", required=True)
    parser.add_argument(
        "--path_to_source",
        type=str,
        help="Full path to the source, to be replaced with the source root that CMake expects in the suppressions file.",
        required=False,
    )

    return parser.parse_args()


def generate_suppressions(xml_tree: ET.ElementTree, old_source_root: str) -> List[str]:
    """
    Extract all cppcheck suppressions from the xml tree and return them as a list of strings
    in the format:
    <error_type>:<file_path>:<line_number>
    :param xml_tree: The xml tree containing cppcheck defects.
    :param old_source_root: Full path to the source to be replaced with NEW_SOURCE_ROOT.
    :return: A list of formatted strings.
    """
    results = xml_tree.getroot()
    errors_element = results.find("errors")
    errors = errors_element.findall("error")

    suppressions = []

    for error in errors:
        error_type = error.get("id")
        # checkersReport has no location
        if error_type == "checkersReport":
            continue
        # Only interested in the primary location, so just take the first location element.
        location = error.find("location")
        # Replace the root of the source file so that it is consistent with what cmake expects.
        file_path = location.get("file")
        file_path = file_path.replace(old_source_root, NEW_SOURCE_ROOT)
        line_number = location.get("line")
        suppression = f"{error_type}:{file_path}:{line_number}"

        if suppression not in suppressions:
            suppressions.append(suppression)

    return suppressions


if __name__ == "__main__":
    sys.exit(main())
