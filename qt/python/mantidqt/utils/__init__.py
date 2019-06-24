# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package

import sys
from copy import copy


class AddedToSysPath:
    """Context manager to add paths to the system path."""

    def __init__(self, paths):
        self.original_sys_path = copy(sys.path)
        self.paths = paths

    def __enter__(self):
        if self.paths:
            [sys.path.append(path) for path in self.paths]

    def __exit__(self, exc_type, exc_val, exc_tb):
        sys.path = self.original_sys_path
