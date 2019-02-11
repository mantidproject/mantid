# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

import unittest

from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.test import GuiTest
from mantidqt.utils.qt.test.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter


class MultiPythonFileInterpreterTest(GuiTest, QtWidgetFinder):

    def test_default_contains_single_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)

    def test_add_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(2, widget.editor_count)

    def test_editor_widget_not_leaked(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(2, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(3, widget.editor_count)

        widget.close_tab(0)

        QApplication.processEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 2)
        widget.close_tab(0)

        QApplication.processEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)

        widget.close_all()

        QApplication.processEvents()
        # there will always be 1, because we never allow an empty editor widget
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)


if __name__ == '__main__':
    unittest.main()
