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
from unittest.mock import patch

# third-party library imports

# local package imports
from mantidqt.widgets.jupyterconsole import InProcessJupyterConsole
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class InProcessJupyterConsoleTest(unittest.TestCase):
    def test_construction_raises_no_errors(self):
        widget = InProcessJupyterConsole()
        self.assertTrue(hasattr(widget, "kernel_manager"))
        self.assertTrue(hasattr(widget, "kernel_client"))
        self.assertGreater(len(widget.banner), 0)
        self._pre_delete_console_cleanup(widget)
        widget.kernel_manager.shutdown_kernel()
        del widget

    def test_construction_with_startup_code_adds_to_banner_and_executes(self):
        widget = InProcessJupyterConsole(startup_code="x = 1")
        self.assertEqual(1, widget.kernel_manager.kernel.shell.user_ns["x"])
        self._pre_delete_console_cleanup(widget)
        widget.kernel_manager.shutdown_kernel()
        del widget

    @patch("mantidqt.widgets.jupyterconsole.input_qinputdialog")
    def test_construction_overrides_python_input_with_qinputdialog(self, mock_input):
        widget = InProcessJupyterConsole()
        kernel = widget.kernel_manager.kernel
        kernel.raw_input("prompt")

        mock_input.assert_called_with("prompt")

        self._pre_delete_console_cleanup(widget)
        widget.kernel_manager.shutdown_kernel()
        del widget

    def _pre_delete_console_cleanup(self, console):
        """Certain versions of qtconsole seem to raise an attribute error on
        exit of the process when an event is delivered to an event filter
        during object deletion:

        OK
        Traceback (most recent call last):
          File "/usr/local/lib/python2.7/site-packages/qtconsole/completion_html.py", line 149, in eventFilter
            if obj == self._text_edit:
        AttributeError: 'CompletionHtml' object has no attribute '_text_edit'
        Child aborted

        We workaround this by manually deleting the completion widget
        """
        try:
            console._completion_widget = None
        except AttributeError:
            pass


if __name__ == "__main__":
    unittest.main()
