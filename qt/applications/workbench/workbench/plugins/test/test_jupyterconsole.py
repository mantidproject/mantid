# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
# system imports
import unittest

# third-party library imports
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QMainWindow

# local package imports
from workbench.plugins.jupyterconsole import InProcessJupyterConsole, JupyterConsole


@start_qapplication
class JupyterConsoleTest(unittest.TestCase):
    def test_construction_creates_inprocess_console_widget(self):
        main_window = QMainWindow()
        widget = JupyterConsole(main_window)
        self.assertTrue(hasattr(widget, "console"))
        self.assertTrue(isinstance(widget.console, InProcessJupyterConsole))
        console = widget.console
        console.kernel_manager.shutdown_kernel()
        widget.console = None
        del console


if __name__ == "__main__":
    unittest.main()
