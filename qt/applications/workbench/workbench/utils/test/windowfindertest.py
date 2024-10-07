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

from workbench.app.mainwindow import MainWindow
from workbench.utils.windowfinder import find_window
from qtpy.QtWidgets import QMainWindow


class WindowFinderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main_window = MainWindow()

    def test_find_window_finds_top_level_widget(self):
        top_level_widget = QMainWindow()
        interface_name = "ISIS Reflectometry"
        top_level_widget.setObjectName(interface_name)
        window = find_window(interface_name, QMainWindow)
        self.assertIs(top_level_widget, window, "Window found was not the widget supplied")

    def test_find_window_returns_none_if_no_widget_exists(self):
        interface_name = "ISIS Reflectometry"
        window = find_window(interface_name, QMainWindow)
        self.assertIsNone(window, "Found a widget when none expected")
