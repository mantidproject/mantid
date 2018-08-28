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
import unittest

# third-party library imports
from mantidqt.utils.qt.test import requires_qapp
from qtpy.QtWidgets import QMainWindow

# local package imports
from workbench.plugins.jupyterconsole import InProcessJupyterConsole, JupyterConsole


@requires_qapp
class JupyterConsoleTest(unittest.TestCase):

    def test_construction_creates_inprocess_console_widget(self):
        main_window = QMainWindow()
        widget = JupyterConsole(main_window)
        self.assertTrue(hasattr(widget, "console"))
        self.assertTrue(isinstance(widget.console, InProcessJupyterConsole))


if __name__ == '__main__':
    unittest.main()
