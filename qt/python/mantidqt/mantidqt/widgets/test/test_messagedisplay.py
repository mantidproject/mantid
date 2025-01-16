# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#


import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.messagedisplay import MessageDisplay, Priority


@start_qapplication
class MessageDisplayTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.unix_path = "/path/to/MyScript.py"
        cls.win_path = "C://path/to/MyScript(1).py"

    def setUp(self):
        self.display = MessageDisplay()

    def test_that_the_Filter_By_menu_is_in_the_context_menu(self):
        context_menu = self.display.generateContextMenu()
        menu_items = [action.iconText() for action in context_menu.actions()]
        self.assertIn("View", menu_items)

    def test_that_text_appended_through_appendPython_toggles_visibility_when_toggling_show_and_hide_script(self):
        msg = "Some script output"
        self.display.appendPython(msg, Priority.Notice, self.unix_path)
        self.assertIn(msg, self._get_message_window_contents())
        self.display.hide_all_scripts()
        self.assertNotIn(msg, self._get_message_window_contents())
        self.display.show_all_scripts()
        self.assertIn(msg, self._get_message_window_contents())

    def test_that_text_appended_through_appendNotice_toggles_visibility_when_toggling_filter_framework(self):
        msg = "Some framework message"
        self.display.appendNotice(msg)
        self.assertIn(msg, self._get_message_window_contents())
        self.display.toggle_filter_framework_output()
        self.assertNotIn(msg, self._get_message_window_contents())
        self.display.toggle_filter_framework_output()
        self.assertIn(msg, self._get_message_window_contents())

    def test_new_framework_output_is_shown_if_showAllScriptOutput_is_False(self):
        self.display.setShowAllScriptOutput(False)
        framework_msg = "A new framework message"
        self.display.appendNotice(framework_msg)
        self.assertIn(framework_msg, self._get_message_window_contents())

    def test_new_script_output_is_shown_if_showFrameworkOutput_is_False_and_showAllScriptOutput_is_True(self):
        self.display.setShowFrameworkOutput(False)
        self.display.setShowAllScriptOutput(True)
        script_msg = "A new script message"
        self.display.last_executed_script = self.unix_path
        self.display.append_script_notice(script_msg)
        self.assertIn(script_msg, self._get_message_window_contents())

    def test_new_framework_output_is_not_shown_if_showFrameworkOutput_is_False(self):
        self.display.setShowFrameworkOutput(False)
        framework_msg = "A new framework message"
        self.display.appendNotice(framework_msg)
        self.assertNotIn(framework_msg, self._get_message_window_contents())

    def test_that_changing_current_tab_hides_output_from_previous_tab_and_shows_output_from_the_new(self):
        self.display.setActiveScript(self.unix_path)
        self.display.show_active_script()
        self._simulate_scripts_printing({self.unix_path: ["Message 1"], self.win_path: ["Message 2"]})
        self.assertIn("Message 1", self._get_message_window_contents())
        self.assertNotIn("Message 2", self._get_message_window_contents())
        self.display.current_tab_changed(self.win_path)
        self.assertNotIn("Message 1", self._get_message_window_contents())
        self.assertIn("Message 2", self._get_message_window_contents())

    def test_that_new_notices_from_script_are_not_displayed_if_showAllScript_and_showActiveScriptOutput_are_False(self):
        self.display.setShowAllScriptOutput(False)
        self.display.setShowActiveScriptOutput(False)
        self._simulate_scripts_printing({self.unix_path: ["Message 1"], self.win_path: ["Message 2"]})
        self.assertNotIn("Message 1", self._get_message_window_contents())
        self.assertNotIn("Message 2", self._get_message_window_contents())

    def test_that_a_scripts_messages_are_still_visible_after_it_has_been_renamed_and_hide_all_toggled_on_and_off(self):
        self.display.show_active_script()
        self.display.setActiveScript(self.unix_path)
        self._simulate_scripts_printing({self.unix_path: ["Message"]})
        self.assertIn("Message", self._get_message_window_contents())
        self.display.hide_all_scripts()
        self.assertNotIn("Message", self._get_message_window_contents())
        self.display.file_name_modified(self.unix_path, self.win_path)
        self.display.show_active_script()
        self.assertIn("Message", self._get_message_window_contents())

    def test_that_a_scripts_messages_are_visible_after_it_has_been_renamed_and_tabs_have_been_changed_to_and_from(self):
        self.display.show_active_script()
        self.display.setActiveScript(self.unix_path)
        self._simulate_scripts_printing({self.unix_path: ["Message"]})
        self.assertIn("Message", self._get_message_window_contents())
        self.display.current_tab_changed(self.win_path)
        self.assertNotIn("Message", self._get_message_window_contents())
        self.display.file_name_modified(self.unix_path, "New Path")
        self.display.current_tab_changed("New Path")
        self.assertIn("Message", self._get_message_window_contents())

    def test_that_messages_with_error_priority_are_displayed_even_if_the_script_is_not_the_active_tab(self):
        err_msg = "An error message"
        notice_msg = "A notice message"
        self.display.show_active_script()
        self._simulate_script_executions([self.unix_path])
        self.display.current_tab_changed(self.win_path)
        self.display.append_script_error(err_msg)
        self.display.append_script_notice(notice_msg)
        self.assertIn(err_msg, self._get_message_window_contents())
        self.assertNotIn(notice_msg, self._get_message_window_contents())

    def test_that_messages_with_error_priority_are_filtered_out_after_toggling_filter_on_script(self):
        err_msg = "An error message"
        self.display.show_active_script()
        self.display.script_executing(self.unix_path)
        self.display.append_script_error(err_msg)
        self.assertIn(err_msg, self._get_message_window_contents())
        self.display.current_tab_changed(self.win_path)
        self.assertNotIn(err_msg, self._get_message_window_contents())

    def test_that_hide_all_scripts_hides_all_script_messages(self):
        self._simulate_scripts_printing({self.unix_path: ["Message 1"], self.win_path: ["Message 2"]})
        self.display.hide_all_scripts()
        self.assertNotIn("Message 1", self._get_message_window_contents())
        self.assertNotIn("Message 2", self._get_message_window_contents())

    def test_that_if_hide_all_script_selected_no_new_script_output_is_displayed(self):
        self.display.hide_all_scripts()
        self._simulate_scripts_printing({self.unix_path: ["Message 1"], self.win_path: ["Message 2"]})
        self.assertNotIn("Message 1", self._get_message_window_contents())
        self.assertNotIn("Message 2", self._get_message_window_contents())

    def test_that_log_messages_are_displayed_if_hide_all_scripts_is_selected(self):
        msg = "Notice log message"
        self.display.hide_all_scripts()
        self.display.appendNotice(msg)
        self.assertIn(msg, self._get_message_window_contents())

    def _get_message_window_contents(self):
        return self.display.getTextEdit().toPlainText()

    def _simulate_scripts_printing(self, script_messages):
        for path, messages in script_messages.items():
            self.display.script_executing(path)
            for msg in messages:
                self.display.append_script_notice(msg)

    def _get_menu_action(self, menu, action_text):
        for action in menu.actions():
            if action_text in action.iconText():
                return action
        self.fail("Could not find action with text '{}' in menu '{}'.".format(action_text, menu))

    def _simulate_script_executions(self, paths):
        for path in paths:
            self.display.script_executing(path)


if __name__ == "__main__":
    unittest.main()
