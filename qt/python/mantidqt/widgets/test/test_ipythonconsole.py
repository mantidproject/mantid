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

# local package imports
from mantidqt.widgets.ipythonconsole import InProcessIPythonConsole
from mantidqt.utils.qt.testing import requires_qapp


@requires_qapp
class InProcessIPythonConsoleTest(unittest.TestCase):

    def test_construction_raises_no_errors(self):
        widget = InProcessIPythonConsole()
        self.assertTrue(hasattr(widget, "kernel_manager"))
        self.assertTrue(hasattr(widget, "kernel_client"))


if __name__ == '__main__':
    unittest.main()
