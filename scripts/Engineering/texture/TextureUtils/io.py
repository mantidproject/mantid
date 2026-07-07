# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from os import scandir
from pathlib import Path
from typing import Sequence


def find_all_files(directory: str) -> Sequence[str]:
    """
    find all the files in a directory

    directory: directory to iterate over
    """
    with scandir(directory) as entries:
        return [entry.path for entry in entries if entry.is_file()]


def mk(dir_path: str) -> None:
    """
    make a directory

    dir_path: path to make a directory at
    """
    p = Path(dir_path)
    if not p.exists():
        p.mkdir()
