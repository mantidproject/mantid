# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import)

# system imports
import unittest

# third-party library imports
from mantidqt.utils.qt.testing import GuiTest
from qtpy.QtWidgets import QMainWindow

# local package imports
from workbench.plugins.jupyterconsole import InProcessJupyterConsole, JupyterConsole


class JupyterConsoleTest(GuiTest):

    def test_construction_creates_inprocess_console_widget(self):
        main_window = QMainWindow()
        widget = JupyterConsole(main_window)
        self.assertTrue(hasattr(widget, "console"))
        self.assertTrue(isinstance(widget.console, InProcessJupyterConsole))


if __name__ == '__main__':
    unittest.main()
