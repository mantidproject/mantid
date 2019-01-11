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
import sys
import unittest

# third-party library imports
import IPython
from qtpy import QT_VERSION

# local package imports
from mantidqt.widgets.jupyterconsole import InProcessJupyterConsole
from mantidqt.utils.qt.test import GuiTest


PRE_IPY5_PY3_QT5 = (sys.version_info.major == 3 and IPython.version_info[0] < 5 and int(QT_VERSION[0]) == 5)
SKIP_REASON = "Segfault within readline for IPython < 5 and Python 3"


class InProcessJupyterConsoleTest(GuiTest):

    @unittest.skipIf(PRE_IPY5_PY3_QT5, SKIP_REASON)
    def test_construction_raises_no_errors(self):
        widget = InProcessJupyterConsole()
        self.assertTrue(hasattr(widget, "kernel_manager"))
        self.assertTrue(hasattr(widget, "kernel_client"))
        self.assertTrue(len(widget.banner) > 0)
        del widget

    @unittest.skipIf(PRE_IPY5_PY3_QT5, SKIP_REASON)
    def test_construction_with_banner_replaces_default(self):
        widget = InProcessJupyterConsole(banner="Hello!")
        self.assertEquals("Hello!", widget.banner)
        del widget

    @unittest.skipIf(PRE_IPY5_PY3_QT5, SKIP_REASON)
    def test_construction_with_startup_code_adds_to_banner_and_executes(self):
        widget = InProcessJupyterConsole(startup_code="x = 1")
        self.assertTrue("x = 1" in widget.banner)
        self.assertEquals(1, widget.kernel_manager.kernel.shell.user_ns['x'])
        del widget


if __name__ == '__main__':
    unittest.main()
