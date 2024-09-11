#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Accepts a compile_commands.json and removes entries for matching source
files. Used to remove entries for generated cpp files that we want cppcheck
to skip.
"""

import argparse
import json
import logging
import re
import sys
from typing import Mapping, Sequence

DEFAULT_LOG_LEVEL = logging.INFO
# Reject any file whose regex matches this
REJECTED_FILE_RE = re.compile(r"^.*?(?:moc|qrc|sip)_.+?.cpp$")

# A list of compile-command entries
CompileCommands = Sequence[Mapping[str, str]]


def main() -> int:
    """
    Main entry point for the program.
    """
    logging.basicConfig(format="", level=DEFAULT_LOG_LEVEL)
    args = parse_arguments()

    logging.info(f"Cleaning {args.commandsjson}")
    with open(args.commandsjson, "r") as fp:
        cleaned_commands = remove_source_entries(json.load(fp))

    logging.info(f"Writing cleaned compile commands to {args.outfile}")
    with open(args.outfile, "w") as fp:
        json.dump(cleaned_commands, fp)

    return 0


def parse_arguments() -> argparse.Namespace:
    """
    Process command-line arguments from sys.argv
    :return: A argparse.Namespace containing the arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("commandsjson", type=str, help="A file containing a json-array of compile commands.")
    parser.add_argument("--outfile", type=str, help="Name of file to write the sanitized output to.")

    return parser.parse_args()


def remove_source_entries(compile_commands: CompileCommands) -> CompileCommands:
    """
    Given a list of filepaths check the list of compile commands and remove
    any entries whose file element matches

    :param compile_commands: An array of dicts defining compile commands for a
                             sequence of files.
    :return: A dictionary cleaned of entries referencing provided file paths.
    """
    return list(filter(keep_entry, compile_commands))


def keep_entry(compile_entry: str) -> bool:
    """Return True if this entry should be kept"""

    # .match returns None if no match occurs meaning we want to keep the entry
    return REJECTED_FILE_RE.match(compile_entry["file"]) is None


if __name__ == "__main__":
    sys.exit(main())
