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

from unittest.mock import patch, MagicMock

from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import FrameworkManager

from qtpy.QtWidgets import QMessageBox


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


if __name__ == '__main__':
    unittest.main()
