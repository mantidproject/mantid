# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import unittest
from unittest import mock

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QApplication, QTabBar
import sip

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter


@start_qapplication
class MultiPythonFileInterpreterViewTest(unittest.TestCase, QtWidgetFinder):
    def setUp(self):
        self.widget = MultiPythonFileInterpreter()

    def tearDown(self):
        if not sip.isdeleted(self.widget):
            sip.delete(self.widget)

    def test_editor_widget_not_leaked(self):
        self.assertEqual(1, self.widget.editor_count)
        self.widget.append_new_editor()
        self.assertEqual(2, self.widget.editor_count)
        self.widget.append_new_editor()
        self.assertEqual(3, self.widget.editor_count)

        self.widget.close_tab(0)

        QApplication.sendPostedEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 2)
        self.widget.close_tab(0)

        QApplication.sendPostedEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)

        self.widget.close_all()

        QApplication.sendPostedEvents()
        # there should be zero interpreters
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 0)

        # close the whole widget, this should delete everything from the QApplication
        self.widget.close()
        QApplication.sendPostedEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 0)
        self.assert_no_toplevel_widgets()

    def test_editor_widget_deletes_find_replace_dialog(self):
        self.assertEqual(1, self.widget.editor_count)
        self.widget.append_new_editor()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 2)
        self.widget.current_editor().show_find_replace_dialog()
        self.assert_number_of_widgets_matching("Embedded", 1)

        self.widget.close_tab(1)

        QApplication.sendPostedEvents()
        # there will always be 1, because we never allow an empty editor widget
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)
        self.assert_number_of_widgets_matching("Embedded", 0)

        # close the whole widget, this should delete everything from the QApplication
        self.widget.close()
        QApplication.sendPostedEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 0)
        self.assert_no_toplevel_widgets()

    def test_editor_widget_doesnt_create_find_replace_unless_requested(self):
        self.assertEqual(1, self.widget.editor_count)
        self.widget.append_new_editor()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 2)
        self.assert_number_of_widgets_matching("Embedded", 0)

        self.widget.close_tab(1)

        QApplication.sendPostedEvents()
        # there will always be 1, because we never allow an empty editor widget
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 1)
        self.assert_number_of_widgets_matching("Embedded", 0)

        # close the whole widget, this should delete everything from the QApplication
        self.widget.close()
        QApplication.sendPostedEvents()
        self.assert_number_of_widgets_matching(".interpreter.PythonFileInterpreter", 0)
        self.assert_no_toplevel_widgets()

    def test_close_tab_button_closes_the_correct_tab(self):
        self.widget.close_tab = mock.MagicMock()

        self.assertEqual(1, self.widget.editor_count)
        close_tab_button1 = self.widget._tabs.tabBar().tabButton(0, QTabBar.RightSide)

        self.widget.append_new_editor()
        self.assertEqual(2, self.widget.editor_count)
        close_tab_button2 = self.widget._tabs.tabBar().tabButton(1, QTabBar.RightSide)

        QTest.mouseClick(close_tab_button2, Qt.LeftButton)
        QApplication.sendPostedEvents()

        self.widget.close_tab.assert_any_call(1)
        # Assert that the first tab has not been closed on call to tab index 1
        self.assertFalse(mock.call(0) in self.widget.close_tab)

        QTest.mouseClick(close_tab_button1, Qt.LeftButton)
        QApplication.sendPostedEvents()

        self.widget.close_tab.assert_any_call(0)


if __name__ == '__main__':
    unittest.main()
