# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

from mantidqt.utils.testing.strict_mock import StrictMock


class MockCodeEditor(object):
    def __init__(self):
        self.setFocus = StrictMock()
        self.findFirst = StrictMock()
        self.findNext = StrictMock()
        self.hasSelectedText = StrictMock()
        self.selectedText = StrictMock()
        self.replace = StrictMock()
        self.replaceAll = StrictMock()
        self.fileName = StrictMock()
