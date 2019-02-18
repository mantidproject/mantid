# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mock import Mock


class MockCodeEditor(object):
    def __init__(self):
        self.setFocus = Mock()
        self.findFirst = Mock()
        self.findNext = Mock()
        self.hasSelectedText = Mock()
        self.selectedText = Mock()
        self.replace = Mock()
        self.replaceAll = Mock()
