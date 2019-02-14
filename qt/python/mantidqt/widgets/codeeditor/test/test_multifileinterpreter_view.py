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

from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.test import GuiTest
from mantidqt.utils.qt.test.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter


class MultiPythonFileInterpreterDeletionTest(GuiTest, QtWidgetFinder):
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

    def test_editor_widget_deletes_find_replace_dialog(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 2)
        widget.current_editor().show_find_replace_dialog()
        self.assert_number_of_widgets_matching("Embedded", 1)

        widget.close_tab(1)

        QApplication.processEvents()
        # there will always be 1, because we never allow an empty editor widget
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)
        self.assert_number_of_widgets_matching("Embedded", 0)

    def test_editor_widget_doesnt_create_find_replace_unless_requested(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 2)
        self.assert_number_of_widgets_matching("Embedded", 0)

        widget.close_tab(1)

        QApplication.processEvents()
        # there will always be 1, because we never allow an empty editor widget
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)
        self.assert_number_of_widgets_matching("Embedded", 0)
