# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest

from mantidqt.widgets.messagedisplay import MessageDisplay, Priority
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class MessageDisplayTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.display = MessageDisplay()

    def setUp(self):
        self.display.display_script_output = True
        self.display.display_framework_output = True

    def test_that_the_Filter_By_menu_is_in_the_context_menu(self):
        context_menu = self.display.generateContextMenu()
        menu_items = [action.iconText() for action in context_menu.actions()]
        self.assertIn('Filter by', menu_items)

    def test_that_text_appended_through_appendPython_toggles_visibility_when_toggling_filter_script(self):
        msg = 'Some script output'
        self.display.appendPython(msg, Priority.Notice)
        self.assertIn(msg, self.display.getTextEdit().toPlainText())
        self.display.toggle_filter_script_output()
        self.assertNotIn(msg, self.display.getTextEdit().toPlainText())
        self.display.toggle_filter_script_output()
        self.assertIn(msg, self.display.getTextEdit().toPlainText())

    def test_that_text_appended_through_appendNotice_toggles_visibility_when_toggling_filter_framework(self):
        msg = 'Some framework message'
        self.display.appendNotice(msg)
        self.assertIn(msg, self.display.getTextEdit().toPlainText())
        self.display.toggle_filter_framework_output()
        self.assertNotIn(msg, self.display.getTextEdit().toPlainText())
        self.display.toggle_filter_framework_output()
        self.assertIn(msg, self.display.getTextEdit().toPlainText())


if __name__ == "__main__":
    unittest.main()
