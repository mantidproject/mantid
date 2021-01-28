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
import unittest

from unittest.mock import patch, Mock, MagicMock, call

from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import FrameworkManager

from qtpy.QtWidgets import QMessageBox, QAction, QApplication

from workbench.utils.recentlyclosedscriptsmenu import RecentlyClosedScriptsMenu # noqa

@start_qapplication
class MainWindowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from workbench.app.mainwindow import MainWindow
        cls.main_window = MainWindow()

    @patch("workbench.app.mainwindow.find_window")
    def test_launch_custom_cpp_gui_creates_interface_if_not_already_open(self, mock_find_window):
        mock_find_window.return_value = None
        interface_name = 'ISIS Reflectometry'
        with patch.object(self.main_window, 'interface_manager') as mock_interface_manager:
            self.main_window.launch_custom_cpp_gui(interface_name)
            mock_interface_manager.createSubWindow.assert_called_once_with(interface_name)

    @patch("workbench.app.mainwindow.find_window")
    def test_different_interfaces_simultaneously_created(self, mock_find_window):
        mock_find_window.return_value = None
        interface_name = 'Data Reduction'
        second_interface_name = 'Settings'
        with patch.object(self.main_window, 'interface_manager') as mock_interface_manager:
            self.main_window.launch_custom_cpp_gui(interface_name)
            mock_interface_manager.createSubWindow.assert_called_with(interface_name)
            self.main_window.launch_custom_cpp_gui(second_interface_name)
            mock_interface_manager.createSubWindow.assert_called_with(second_interface_name)

    @patch("workbench.app.mainwindow.FrameworkManager")
    @patch("workbench.app.mainwindow.QMessageBox")
    def test_clear_all_memory_calls_frameworkmanager_when_user_presses_ok(self,mock_msg_box, mock_fm):
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

    def test_menus_exist(self):
        self.main_window.menuBar().addMenu = MagicMock()
        expected_calls = [call("&File"), call("&View"), call("&Interfaces"), call("&Help")]

        self.main_window.create_menus()

        self.main_window.menuBar().addMenu.assert_has_calls(expected_calls, any_order=False)

    @patch('workbench.app.mainwindow.add_actions')
    def test_menus_are_correct(self, mock_add_actions):
        from workbench.app.mainwindow import MainWindow
        main_window = MainWindow()
        main_window.editor = Mock()
        expected_file_menu_items = ['Open Script', 'Open Project', None, 'Save Script', 'Save Script as...', RecentlyClosedScriptsMenu, 'Generate Recovery Script', None, 'Save Project', 'Save Project as...', None, 'Settings', None, 'Manage User Directories', None, 'Script Repository', None, 'Clear All Memory', None, '&Quit']
        expected_view_menu_items = ['Restore Default Layout', None]  # There are no wigets on this instance of MainWindow, so they will not appear on the view menu.
        expected_help_menu_items = ['Mantid Help', 'Mantid Concepts', 'Algorithm Descriptions', None, 'Mantid Homepage', 'Mantid Forum', None, 'About Mantid Workbench']

        convert_actions_to_texts = lambda menu_actions: \
            [a.text() if isinstance(a, QAction) else a if not a else type(a) for a in menu_actions]

        main_window.create_menus()
        main_window.create_actions()
        main_window.populate_menus()

        actual_file_menu_items = convert_actions_to_texts(main_window.file_menu_actions)
        actual_view_menu_items = convert_actions_to_texts(main_window.view_menu_actions)
        actual_help_menu_items = convert_actions_to_texts(main_window.help_menu_actions)

        self.assertEqual(expected_file_menu_items, actual_file_menu_items)
        self.assertEqual(expected_view_menu_items, actual_view_menu_items)
        self.assertEqual(expected_help_menu_items, actual_help_menu_items)
        mock_add_actions.add_actions.assert_has_calls([
            call(main_window.file_menu, main_window.file_menu_actions),
            call(main_window.view_menu, main_window.view_menu_actions),
            call(main_window.help_menu, main_window.help_menu_actions),
        ])

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
        mock_project.saved = False
        mock_project.offer_save = Mock(return_value=False)  # user cancels when save offered
        self.main_window.project = mock_project

        self.main_window.closeEvent(mock_event)

        mock_project.offer_save.assert_called()
        mock_event.ignore.assert_called()

    @patch('workbench.app.mainwindow.ConfigService')
    @patch('workbench.app.mainwindow.QApplication')
    @patch('matplotlib.pyplot.close')
    def test_main_window_close_behavior_correct_when_workbench_able_to_be_closed(
            self,
            mock_plt_close,
            mock_QApplication,
            mock_ConfigService
    ):
        mock_event = Mock()
        mock_project = Mock()
        mock_project.is_saving, mock_project.is_loading, mock_project.saved = False, False, True
        mock_editor = Mock()
        mock_editor.app_closing = Mock(return_value=True)  # Editors can be closed
        mock_project_recovery = Mock()
        mock_project_recovery.stop_recovery_thread, mock_project_recovery.remove_current_pid_folder =\
            Mock(), Mock()
        self.main_window.editor = mock_editor
        self.main_window.writeSettings = Mock()
        self.main_window.project_recovery = mock_project_recovery
        self.main_window.interface_manager = Mock()
        self.main_window.writeSettings = Mock()
        self.main_window.project = mock_project

        self.main_window.closeEvent(mock_event)

        mock_ConfigService.saveConfig.assert_called_with(mock_ConfigService.getUserFilename())
        self.main_window.writeSettings.assert_called()
        mock_plt_close.assert_called_with('all')
        mock_QApplication.instance().closeAllWindows.assert_called()
        self.main_window.project_recovery.assert_has_calls([call.stop_recovery_thread(), call.remove_current_pid_folder()])
        self.assertTrue(self.main_window.project_recovery.closing_workbench)
        self.main_window.interface_manager.closeHelpWindow.assert_called()
        mock_event.accept.assert_called()

    @patch('workbench.app.mainwindow.logger')
    @patch('workbench.app.mainwindow.ConfigService')
    @patch('os.path.exists')
    def test_python_interfaces_are_discovered_correctly(self, mock_os_path_exists, mock_ConfigService, _):
        interfaces = ['Muon/Frequency_Domain_Analysis.py', 'ILL/Drill.py']
        interfaces_str = " ".join(interfaces)  # config service returns them as a whole string.
        mock_os_path_exists.return_value = lambda path: path in interfaces
        mock_ConfigService.__getitem__.side_effect = \
            lambda arg: interfaces_str if arg == 'mantidqt.python_interfaces' else None

        returned_interfaces = self.main_window._discover_python_interfaces('', [])

        expected_interfaces = {'Muon': ['Frequency_Domain_Analysis.py'], 'ILL': ['Drill.py']}
        self.assertDictEqual(expected_interfaces, returned_interfaces)

    @patch('workbench.app.mainwindow.logger')
    @patch('workbench.app.mainwindow.ConfigService')
    @patch('os.path.exists')
    def test_that_non_existent_python_interface_is_ignored_gracefully(self, mock_os_path_exists, mock_ConfigService, mock_logger):
        interface_str = 'fake/interface.py'
        mock_os_path_exists.return_value = False
        mock_ConfigService.__getitem__.side_effect = \
            lambda arg: interface_str if arg == 'mantidqt.python_interfaces' else None

        returned_interfaces = self.main_window._discover_python_interfaces('', [])

        self.assertDictEqual({}, returned_interfaces)
        mock_logger.warning.assert_called()

    @patch('workbench.app.mainwindow.logger')
    @patch('workbench.app.mainwindow.ConfigService')
    @patch('os.path.exists')
    def test_that_non_existent_python_interface_is_ignored_gracefully(self, mock_os_path_exists, mock_ConfigService, mock_logger):
        interface_str = 'blacklisted/interface.py'
        mock_os_path_exists.return_value = True
        mock_ConfigService.__getitem__.side_effect = \
            lambda arg: interface_str if arg == 'mantidqt.python_interfaces' else None

        returned_interfaces = self.main_window._discover_python_interfaces('', 'interface.py')

        self.assertDictEqual({}, returned_interfaces)
        mock_logger.information.assert_called()

    @patch('workbench.app.mainwindow.UserSubWindowFactory')
    def test_cpp_interfaces_are_discovered_correctly(self, mock_UserSubWindowFactory):
        """Assuming we have already found some python interfaces, test that
        cpp interfaces are discovered correctly using the Direct interfaces as an example."""

        cpp_interface_factory = Mock()
        cpp_interface_factory.keys.return_value = ['ALF View', 'TOFCalculator']
        cpp_interface_factory.categories.side_effect = lambda name: ['Direct'] if name == 'ALF View' else []
        mock_UserSubWindowFactory.Instance.return_value = cpp_interface_factory

        all_interfaces = self.main_window._discover_cpp_interfaces(
            {'Direct': ['DGS_Reduction.py', 'DGSPlanner.py', 'PyChop.py', 'MSlice.py']})

        expected_interfaces = {
            'Direct': ['DGS_Reduction.py', 'DGSPlanner.py', 'PyChop.py', 'MSlice.py', 'ALF View'],
            'General': ['TOFCalculator']
        }
        self.assertDictEqual(expected_interfaces, all_interfaces)


if __name__ == '__main__':
    unittest.main()
