#    This file is part of the mantid workbench.
#
#    Copyright (C) 2017 mantidproject
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
