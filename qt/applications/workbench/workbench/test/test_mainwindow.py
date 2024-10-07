# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Defines the QMainWindow of the application and the main() entry point.
"""

import os
import unittest
import sys

from unittest.mock import patch, Mock, MagicMock, call

import matplotlib

from mantidqt.utils.qt.testing import start_qapplication
from mantid.api import FrameworkManager
from qtpy.QtWidgets import QMessageBox, QAction, QMenu
from workbench.utils.recentlyclosedscriptsmenu import RecentlyClosedScriptsMenu
from io import StringIO


@start_qapplication
class MainWindowTest(unittest.TestCase):
    def setUp(self):
        from workbench.app.mainwindow import MainWindow

        self.main_window = MainWindow()

    def tearDown(self):
        self.main_window.close()

    @patch("workbench.app.mainwindow.find_window")
    def test_launch_custom_cpp_gui_creates_interface_if_not_already_open(self, mock_find_window):
        mock_find_window.return_value = None
        interface_name = "ISIS Reflectometry"
        with patch.object(self.main_window, "interface_manager") as mock_interface_manager:
            self.main_window.launch_custom_cpp_gui(interface_name)
            mock_interface_manager.createSubWindow.assert_called_once_with(interface_name)

    @patch("workbench.app.mainwindow.find_window")
    def test_different_interfaces_simultaneously_created(self, mock_find_window):
        mock_find_window.return_value = None
        interface_name = "Data Reduction"
        second_interface_name = "Settings"
        with patch.object(self.main_window, "interface_manager") as mock_interface_manager:
            self.main_window.launch_custom_cpp_gui(interface_name)
            mock_interface_manager.createSubWindow.assert_called_with(interface_name)
            self.main_window.launch_custom_cpp_gui(second_interface_name)
            mock_interface_manager.createSubWindow.assert_called_with(second_interface_name)

    @patch("workbench.app.mainwindow.FrameworkManager")
    @patch("workbench.app.mainwindow.QMessageBox")
    def test_clear_all_memory_calls_frameworkmanager_when_user_presses_ok(self, mock_msg_box, mock_fm):
        mock_msg_box_instance = MagicMock(spec=QMessageBox)
        mock_msg_box.return_value = mock_msg_box_instance
        mock_msg_box_instance.exec.return_value = mock_msg_box.Ok
        mock_fm_instance = MagicMock(spec=FrameworkManager)
        mock_fm.Instance.return_value = mock_fm_instance

        self.main_window.clear_all_memory_action()

        mock_fm_instance.clear.assert_called_once()

    @patch("workbench.app.mainwindow.FrameworkManager")
    @patch("workbench.app.mainwindow.QMessageBox")
    def test_clear_all_memory_does_not_call_frameworkmanager_when_user_presses_cancel(self, mock_msg_box, mock_fm):
        mock_msg_box_instance = MagicMock(spec=QMessageBox)
        mock_msg_box.return_value = mock_msg_box_instance
        mock_msg_box_instance.exec.return_value = mock_msg_box.Cancel
        mock_fm_instance = MagicMock(spec=FrameworkManager)
        mock_fm.Instance.return_value = mock_fm_instance

        self.main_window.clear_all_memory_action()

        mock_fm_instance.clear.assert_not_called()

    @patch("workbench.plugins.logmessagedisplay.ORIGINAL_STDOUT", new=StringIO())
    @patch("workbench.plugins.logmessagedisplay.ORIGINAL_STDERR", new=StringIO())
    @patch("workbench.plugins.logmessagedisplay.MessageDisplay.append_script_notice")
    @patch("workbench.plugins.logmessagedisplay.MessageDisplay.append_script_error")
    def test_after_setup_stdout_and_stderr_are_captured(self, append_script_notice, append_script_error):
        original_stdout = sys.stdout
        original_stderr = sys.stderr

        try:
            matplotlib.use("agg")
            self.main_window.setup()
            print("test stdout")
            print("test stderr", file=sys.stderr)
        finally:
            # whatever happened, we need to reset these so unittest can report it!
            sys.stdout = original_stdout
            sys.stderr = original_stderr

        append_script_notice.assert_called()
        append_script_error.assert_called()

    def test_menus_exist(self):
        self.main_window.menuBar().addMenu = MagicMock()
        expected_calls = [call("&File"), call("&View"), call("&Interfaces"), call("&Help")]

        self.main_window.create_menus()

        self.main_window.menuBar().addMenu.assert_has_calls(expected_calls, any_order=False)

    @patch("workbench.app.mainwindow.add_actions")
    def test_file_view_and_help_menus_are_correct(self, mock_add_actions):
        def convert_action_to_text(menu_item):
            """Takes an item on a mainwindow menu, and returns a representation of the item
            so we can assert whether menus look right to the user."""
            if isinstance(menu_item, QAction):
                return menu_item.text()
            if not menu_item:
                return None
            return type(menu_item)

        self.main_window.editor = Mock()
        self.main_window.populate_interfaces_menu = Mock()
        expected_file_menu_items = [
            "Open Script",
            "Open Project",
            None,
            "Save Script",
            "Save Script as...",
            RecentlyClosedScriptsMenu,
            "Generate Recovery Script",
            None,
            "Save Project",
            "Save Project as...",
            None,
            "Settings",
            None,
            "Manage User Directories",
            None,
            "Script Repository",
            None,
            "Clear All Memory",
            None,
            "&Quit",
        ]
        # There are no wigets on this instance of MainWindow, so they will not appear on the view menu.
        expected_view_menu_items = ["Restore Default Layout", None]
        expected_help_menu_items = [
            "Mantid Help",
            "Mantid Concepts",
            "Algorithm Descriptions",
            None,
            "Mantid Homepage",
            "Mantid Forum",
            None,
            "About Mantid Workbench",
        ]

        self.main_window.create_menus()
        self.main_window.create_actions()
        self.main_window.populate_menus()

        actual_file_menu_items = list(map(convert_action_to_text, self.main_window.file_menu_actions))
        actual_view_menu_items = list(map(convert_action_to_text, self.main_window.view_menu_actions))
        actual_help_menu_items = list(map(convert_action_to_text, self.main_window.help_menu_actions))
        self.assertEqual(expected_file_menu_items, actual_file_menu_items)
        self.assertEqual(expected_view_menu_items, actual_view_menu_items)
        self.assertEqual(expected_help_menu_items, actual_help_menu_items)
        mock_add_actions.assert_has_calls(
            [
                call(self.main_window.file_menu, self.main_window.file_menu_actions),
                call(self.main_window.view_menu, self.main_window.view_menu_actions),
                call(self.main_window.help_menu, self.main_window.help_menu_actions),
            ]
        )

    @patch("workbench.app.mainwindow.add_actions")
    def test_interfaces_menu_texts_are_correct(self, _):
        example_interfaces = {
            "General": ["TOFCalculator"],
            "Direct": ["DGS_Reduction.py", "DGS_Planner.py", "PyChop.py", "MSlice.py", "ALFView"],
        }

        with (
            patch("workbench.app.mainwindow.ConfigService", new={"interfaces.categories.hidden": ""}),
            patch("workbench.app.mainwindow.gather_interface_names") as mock_gather_interfaces,
            patch("workbench.app.mainwindow.get_registers_to_run") as mock_registers_to_run,
        ):
            mock_gather_interfaces.return_value = example_interfaces
            mock_registers_to_run.return_value = {}
            self.main_window.create_menus()
            self.main_window.populate_interfaces_menu()

        expected_menu_texts = ["Direct", "General"]  # Alphabetical order
        actual_menu_texts = [action.text() for action in self.main_window.interfaces_menu.actions()]
        self.assertEqual(expected_menu_texts, actual_menu_texts)
        expected_direct_texts = ["ALFView", "DGS Planner", "DGS Reduction", "MSlice", "PyChop"]
        expected_general_texts = ["TOFCalculator"]
        submenus = list(filter(lambda child: isinstance(child, QMenu), self.main_window.interfaces_menu.children()))
        actual_direct_texts = [action.text() for action in submenus[0].actions()]
        actual_general_texts = [action.text() for action in submenus[1].actions()]
        self.assertEqual(expected_direct_texts, actual_direct_texts)
        self.assertEqual(expected_general_texts, actual_general_texts)

    @patch("workbench.app.mainwindow.add_actions")
    def test_that_populate_interfaces_menu_discovers_interfaces(self, _):
        import mantidqtinterfaces

        interface_dir = os.path.dirname(mantidqtinterfaces.__file__)
        interfaces = {"category": ["interface.py"]}

        with (
            patch("workbench.app.mainwindow.ConfigService", new={"interfaces.categories.hidden": ""}),
            patch("workbench.app.mainwindow.gather_interface_names") as mock_gather_interfaces,
            patch("workbench.app.mainwindow.get_registers_to_run") as mock_registers_to_run,
        ):
            mock_gather_interfaces.return_value = interfaces
            mock_registers_to_run.return_value = {}
            self.main_window.create_menus()
            self.main_window.populate_interfaces_menu()

            mock_gather_interfaces.assert_called_with(interface_dir)

    def test_that_populate_interfaces_menu_ignores_hidden_interfaces(self):
        self.main_window.interfaces_menu = Mock()
        ConfigService_dict = {"interfaces.categories.hidden": "category1;category2"}

        with (
            patch.object(self.main_window, "interfaces_menu") as mock_interfaces_menu,
            patch("workbench.app.mainwindow.ConfigService", new=ConfigService_dict),
            patch("workbench.app.mainwindow.gather_interface_names") as mock_gather_interfaces,
            patch("workbench.app.mainwindow.get_registers_to_run") as mock_registers_to_run,
        ):
            mock_gather_interfaces.return_value = {"category1": ["interface1.py"], "category2": ["interface2.py"]}
            mock_registers_to_run.return_value = {}
            self.main_window.create_menus()
            self.main_window.populate_interfaces_menu()

            mock_interfaces_menu.addMenu.assert_not_called()

    def test_main_window_does_not_close_when_project_is_saving(self):
        mock_event = Mock()
        mock_project = Mock()
        mock_project.is_saving = True
        self.main_window.project = mock_project

        self.main_window.closeEvent(mock_event)

        mock_event.ignore.assert_called()
        mock_project.inform_user_not_possible.assert_called()

    def test_main_window_does_not_close_when_project_is_loading(self):
        mock_event = Mock()
        mock_project = Mock()
        mock_project.is_loading = True
        self.main_window.project = mock_project

        self.main_window.closeEvent(mock_event)

        mock_event.ignore.assert_called()
        mock_project.inform_user_not_possible.assert_called()

    def test_main_window_does_not_close_if_project_not_saved_and_user_cancels_project_save(self):
        mock_event = Mock()
        mock_project = Mock()
        mock_project.is_saving = False
        mock_project.is_loading = False
        mock_project.saved = False
        mock_project.offer_save = Mock(return_value=True)  # user cancels when save offered
        self.main_window.project = mock_project

        self.main_window.closeEvent(mock_event)

        mock_project.offer_save.assert_called()
        mock_event.ignore.assert_called()

    @patch("workbench.app.mainwindow.ConfigService")
    @patch("workbench.app.mainwindow.QApplication")
    @patch("matplotlib.pyplot.close")
    def test_main_window_close_behavior_correct_when_workbench_able_to_be_closed(
        self, mock_plt_close, mock_QApplication, mock_ConfigService
    ):
        mock_event = Mock()
        mock_project = Mock()
        mock_project.is_saving, mock_project.is_loading, mock_project.saved = False, False, True
        mock_editor = Mock()
        mock_editor.app_closing = Mock(return_value=True)  # Editors can be closed
        mock_project_recovery = Mock()
        mock_project_recovery.stop_recovery_thread, mock_project_recovery.remove_current_pid_folder = Mock(), Mock()
        self.main_window.editor = mock_editor
        self.main_window.writeSettings = Mock()
        self.main_window.project_recovery = mock_project_recovery
        self.main_window.interface_manager = Mock()
        self.main_window.writeSettings = Mock()
        self.main_window.project = mock_project

        self.main_window.closeEvent(mock_event)

        mock_ConfigService.saveConfig.assert_called_with(mock_ConfigService.getUserFilename())
        self.main_window.writeSettings.assert_called()
        mock_plt_close.assert_called_with("all")
        mock_QApplication.instance().closeAllWindows.assert_called()
        self.main_window.project_recovery.assert_has_calls(
            [call.stop_recovery_thread(), call.remove_current_pid_folder(ignore_errors=True)]
        )
        self.assertTrue(self.main_window.project_recovery.closing_workbench)
        self.main_window.interface_manager.closeHelpWindow.assert_called()
        mock_event.accept.assert_called()

    @patch("workbench.app.mainwindow.input_qinputdialog")
    def test_override_python_input_replaces_input_with_qinputdialog(self, mock_input):
        self.main_window.override_python_input()

        input("prompt")

        mock_input.assert_called_with("prompt")


if __name__ == "__main__":
    unittest.main()
