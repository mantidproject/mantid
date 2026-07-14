#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Accepts a cppcheck XML or SARIF report and generates a list of suppressions to add to
the CppCheck_Suppressions.txt.in template file. Use this when upgrading to a new version
of cppcheck.

To regenerate the report this script needs, build one of the dedicated targets:

    pixi run --frozen cmake --preset=cppcheck-ci ..
    pixi run --frozen cmake --build . --target cppcheck-xml

or:

    pixi run --frozen cmake --preset=cppcheck-ci ..
    pixi run --frozen cmake --build . --target cppcheck-sarif

These write the report to <build-dir>/cppcheck.xml or <build-dir>/cppcheck.sarif; pass one
of them to this script via --cppcheck_xml or --cppcheck_sarif.
"""

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Any, List, Optional

NEW_SOURCE_ROOT = "${CMAKE_SOURCE_DIR}"
OLD_SOURCE_ROOT = "/jenkins_workdir/workspace/pull_requests-cppcheck"


@dataclass
class CppcheckSuppression:
    error_type: str
    file_path: str
    line_number: int

    # Comparison operator for sorting suppressions based on file name and line number.
    def __lt__(self, other):
        if self.file_path < other.file_path:
            return True
        elif self.file_path == other.file_path:
            return self.line_number < other.line_number
        return False

    def suppression_string(self) -> str:
        return f"{self.error_type}:{self.file_path}:{self.line_number}"


def main() -> int:
    """
    Main entry point for the program.
    """
    args = parse_arguments()
    old_source_root = args.path_to_source or OLD_SOURCE_ROOT
    if args.cppcheck_xml:
        report = ET.parse(args.cppcheck_xml)
        suppressions = generate_suppressions_from_xml(report, old_source_root)
    else:
        with open(args.cppcheck_sarif) as f:
            report = json.load(f)
        suppressions = generate_suppressions_from_sarif(report, old_source_root)

    with open(args.outfile, "w") as f:
        f.write("\n".join(suppressions))

    return 0


def parse_arguments() -> argparse.Namespace:
    """
    Process command-line arguments from sys.argv
    :return: An argparse.Namespace containing the arguments
    """
    parser = argparse.ArgumentParser()
    report_group = parser.add_mutually_exclusive_group(required=True)
    report_group.add_argument("--cppcheck_xml", type=str, help="An XML file containing a list of cppcheck defects.")
    report_group.add_argument("--cppcheck_sarif", type=str, help="A SARIF file containing a list of cppcheck defects.")
    parser.add_argument("--outfile", type=str, help="Name of file to write the cppcheck suppressions to.", required=True)
    parser.add_argument(
        "--path_to_source",
        type=str,
        help="Full path to the source, to be replaced with the source root that CMake expects in the suppressions file.",
        required=False,
    )

    return parser.parse_args()


def generate_suppressions_from_xml(xml_tree: ET.ElementTree, old_source_root: str) -> List[str]:
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

    return format_suppressions(extract_suppressions_from_xml(errors, old_source_root))


def extract_suppressions_from_xml(errors: List[ET.Element], old_source_root: str) -> List[CppcheckSuppression]:
    suppressions = []
    for error in errors:
        error_type = error.get("id")
        # checkersReport has no location
        if error_type == "checkersReport":
            continue
        # Only interested in the primary location, so just take the first location element.
        location = error.find("location")
        if location is None:
            continue
        file_path = normalize_path(location.get("file"), old_source_root)
        line_number = int(location.get("line"))
        suppressions.append(CppcheckSuppression(error_type=error_type, file_path=file_path, line_number=line_number))

    return suppressions


def generate_suppressions_from_sarif(sarif_report: Any, old_source_root: str) -> List[str]:
    """
    Extract all cppcheck suppressions from the SARIF report and return them as a list of strings
    in the format:
    <error_type>:<file_path>:<line_number>
    :param sarif_report: The parsed SARIF report containing cppcheck defects.
    :param old_source_root: Full path to the source to be replaced with NEW_SOURCE_ROOT.
    :return: A list of formatted strings.
    """
    suppressions = []
    for run in sarif_report.get("runs", []):
        for result in run.get("results", []):
            error_type = result.get("ruleId")
            if error_type == "checkersReport":
                continue

            locations = result.get("locations", [])
            if not locations:
                continue

            physical_location = locations[0].get("physicalLocation", {})
            artifact_location = physical_location.get("artifactLocation", {})
            region = physical_location.get("region", {})
            file_path = normalize_path(artifact_location.get("uri"), old_source_root)
            line_number = region.get("startLine")
            if file_path is None or line_number is None:
                continue

            suppressions.append(CppcheckSuppression(error_type=error_type, file_path=file_path, line_number=int(line_number)))

    return format_suppressions(suppressions)


def normalize_path(file_path: Optional[str], old_source_root: str) -> Optional[str]:
    if file_path is None:
        return None

    file_path = file_path.removeprefix("file://")

    # Replace the root of the source file so that it is consistent with what cmake expects.
    return file_path.replace(old_source_root, NEW_SOURCE_ROOT)


def format_suppressions(suppressions: List[CppcheckSuppression]) -> List[str]:
    """
    Sort, deduplicate and group internal errors at the end of the suppressions list.
    """
    regular_suppressions = []
    internal_errors = []
    for suppression in suppressions:
        if suppression.error_type == "internalError":
            internal_errors.append(suppression)
        else:
            regular_suppressions.append(suppression)

    # Sort the suppressions by file name and line number.
    regular_suppressions.sort()
    internal_errors.sort()

    # Convert to strings and remove any duplicates.
    suppression_strings = []

    for suppression in regular_suppressions:
        suppression_string = suppression.suppression_string()
        if suppression_string not in suppression_strings:
            suppression_strings.append(suppression_string)

    # Group the internal errors together so they can be moved out of the suppressions list easily.
    for internal_error in internal_errors:
        suppression_string = internal_error.suppression_string()
        if suppression_string not in suppression_strings:
            suppression_strings.append(suppression_string)

    return suppression_strings


if __name__ == "__main__":
    sys.exit(main())
