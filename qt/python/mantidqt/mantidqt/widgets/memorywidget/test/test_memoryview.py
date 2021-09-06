# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_assertions_helper import QtAssertionsHelper
from mantidqt.widgets.memorywidget.memoryview import MemoryView


@start_qapplication
class MemoryViewTest(unittest.TestCase, QtAssertionsHelper):
    def setUp(self):
        self.view = MemoryView(None)

    def test_invoke_set_value_connects_slot_set_value(self):
        self.assert_connected_once(self.view, self.view.set_value)
