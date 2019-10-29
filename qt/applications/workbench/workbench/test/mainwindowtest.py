# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Defines the QMainWindow of the application and the main() entry point.
"""
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest

from mantid.py3compat.mock import Mock, patch
from workbench.app.mainwindow import MainWindow

class MainWindowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
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


if __name__ == '__main__':
    unittest.main()
