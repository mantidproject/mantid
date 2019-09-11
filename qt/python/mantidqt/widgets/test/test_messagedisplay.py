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

from mantid.py3compat.mock import Mock, call, patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.messagedisplay import MessageDisplay, Priority


@start_qapplication
class MessageDisplayTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.unix_path = '/path/to/MyScript.py'
        cls.win_path = 'C://path/to/MyScript(1).py'

    def setUp(self):
        self.display = MessageDisplay()
        self.display.setShowFrameworkOutput(True)
        self.display.setShowAllScriptOutput(True)

    def get_message_window_contents(self):
        return self.display.getTextEdit().toPlainText()

    def test_that_the_Filter_By_menu_is_in_the_context_menu(self):
        context_menu = self.display.generateContextMenu()
        menu_items = [action.iconText() for action in context_menu.actions()]
        self.assertIn('Filter by', menu_items)

    def test_that_text_appended_through_appendPython_toggles_visibility_when_toggling_filter_script(self):
        msg = 'Some script output'
        self.display.appendPython(msg, Priority.Notice, '')
        self.assertIn(msg, self.get_message_window_contents())
        self.display.toggle_filter_all_script_output()
        self.assertNotIn(msg, self.get_message_window_contents())
        self.display.toggle_filter_all_script_output()
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
        self.display.setShowAllScriptOutput(False)
        script_msg = 'A new script message'
        self.display.append_script_notice(script_msg)
        self.assertNotIn(script_msg, self.get_message_window_contents())

    def test_new_framework_output_is_shown_if_showScriptOutput_is_False(self):
        self.display.setShowAllScriptOutput(False)
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

    def test_that_executed_scripts_are_not_duplicated_in_executed_scripts_list(self):
        self._simulate_script_executions([self.unix_path, self.win_path, self.unix_path])
        self.assertEqual(1, list(self.display.executed_scripts).count(self.unix_path))
        self.assertEqual(1, list(self.display.executed_scripts).count(self.win_path))

    def test_that_executed_scripts_appear_unduplicated_in_the_Filter_by_context_menu(self):
        self._simulate_script_executions([self.unix_path, self.win_path, self.unix_path])
        filter_by_menu = self._get_Filter_by_menu(self.display.generateContextMenu())
        filter_options = [action.text() for action in filter_by_menu.actions()]
        self.assertEqual(1, filter_options.count('MyScript.py'))
        self.assertEqual(1, filter_options.count('MyScript(1).py'))

    def test_clicking_on_a_script_in_the_Filter_by_menu_calls_toggle_with_correct_path(self):
        """
        This covers a bug where clicking any filter scripts action would
        always call the final script's slot, not the one that was clicked
        """
        script_paths = ["Script 1.py", "Script 2.py", "Script 3.py"]
        self._simulate_script_executions(script_paths)
        filter_by_menu = self._get_Filter_by_menu(self.display.generateContextMenu())
        filter_by_script_mock = Mock()
        with patch.object(self.display, 'toggle_filter_by_script', filter_by_script_mock):
            for action_text in script_paths:
                action = self._get_menu_action(filter_by_menu, action_text)
                action.trigger()

        self.assertEqual(
            [call("Script 1.py"), call("Script 2.py"),
             call("Script 3.py")], filter_by_script_mock.call_args_list)

    def _get_menu_action(self, menu, action_text):
        for action in menu.actions():
            if action_text in action.iconText():
                return action
        self.fail("Could not find action with text '{}' in menu '{}'."
                  "".format(action_text, menu))

    def _simulate_script_executions(self, paths):
        for path in paths:
            self.display.script_executing(path)

    def _get_Filter_by_menu(self, context_menu):
        for action in context_menu.actions():
            if "Filter by" in action.text():
                return action.menu()
        self.fail("Could not find \"Filter by\" menu in the given context menu.")


if __name__ == "__main__":
    unittest.main()
