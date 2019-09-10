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

    def setUp(self):
        self.display = MessageDisplay()
        self.display.setShowFrameworkOutput(True)
        self.display.setShowScriptOutput(True)

    def get_message_window_contents(self):
        return self.display.getTextEdit().toPlainText()

    def test_that_the_Filter_By_menu_is_in_the_context_menu(self):
        context_menu = self.display.generateContextMenu()
        menu_items = [action.iconText() for action in context_menu.actions()]
        self.assertIn('Filter by', menu_items)

    def test_that_text_appended_through_appendPython_toggles_visibility_when_toggling_filter_script(self):
        msg = 'Some script output'
        self.display.appendPython(msg, Priority.Notice)
        self.assertIn(msg, self.get_message_window_contents())
        self.display.toggle_filter_script_output()
        self.assertNotIn(msg, self.get_message_window_contents())
        self.display.toggle_filter_script_output()
        self.assertIn(msg, self.get_message_window_contents())

    def test_that_text_appended_through_appendNotice_toggles_visibility_when_toggling_filter_framework(self):
        msg = 'Some framework message'
        self.display.appendNotice(msg)
        self.assertIn(msg, self.get_message_window_contents())
        self.display.toggle_filter_framework_output()
        self.assertNotIn(msg, self.get_message_window_contents())
        self.display.toggle_filter_framework_output()
        self.assertIn(msg, self.get_message_window_contents())

    def test_new_script_output_is_not_shown_if_showScriptOutput_is_False(self):
        self.display.setShowScriptOutput(False)
        script_msg = 'A new script message'
        self.display.append_script_notice(script_msg)
        self.assertNotIn(script_msg, self.get_message_window_contents())

    def test_new_framework_output_is_shown_if_showScriptOutput_is_False(self):
        self.display.setShowScriptOutput(False)
        framework_msg = 'A new framework message'
        self.display.appendNotice(framework_msg)
        self.assertIn(framework_msg, self.get_message_window_contents())

    def test_new_script_output_is_shown_if_showFrameworkOutput_is_False(self):
        self.display.setShowFrameworkOutput(False)
        script_msg = 'A new script message'
        self.display.append_script_notice(script_msg)
        self.assertIn(script_msg, self.get_message_window_contents())

    def test_new_framework_output_is_not_shown_if_showFrameworkOutput_is_False(self):
        self.display.setShowFrameworkOutput(False)
        framework_msg = 'A new framework message'
        self.display.appendNotice(framework_msg)
        self.assertNotIn(framework_msg, self.get_message_window_contents())


if __name__ == "__main__":
    unittest.main()
