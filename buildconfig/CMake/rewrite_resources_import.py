# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Post-process a pyrcc5-generated resources module so its Qt import is binding
agnostic. pyrcc5 hard-codes ``from PyQt5 import QtCore``; rewriting it to
``from qtpy import QtCore`` lets the generated module load under both PyQt5 and
PyQt6 (both expose qRegisterResourceData/qUnregisterResourceData)."""

import sys

path = sys.argv[1]
with open(path, encoding="utf-8") as handle:
    contents = handle.read()
contents = contents.replace("from PyQt5 import QtCore", "from qtpy import QtCore")
with open(path, "w", encoding="utf-8") as handle:
    handle.write(contents)
