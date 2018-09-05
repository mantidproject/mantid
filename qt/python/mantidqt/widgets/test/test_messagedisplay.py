#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest

from mantidqt.widgets.messagedisplay import MessageDisplay
from mantidqt.utils.qt.test import GuiTest


class MessageDisplayTest(GuiTest):
    """Minimal testing as it is exported from C++"""

    def test_widget_creation(self):
        display = MessageDisplay()
        self.assertTrue(display is not None)


if __name__ == "__main__":
    unittest.main()
