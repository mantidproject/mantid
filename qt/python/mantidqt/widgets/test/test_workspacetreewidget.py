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
from __future__ import absolute_import, print_function

import unittest

from mantidqt.widgets.workspacewidget.mantidtreemodel import MantidTreeModel
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from mantidqt.utils.qt.testing import requires_qapp


@requires_qapp
class WorkspaceWidgetTest(unittest.TestCase):
    """Minimal testing as it is exported from C++"""

    def test_widget_creation(self):
        param = MantidTreeModel()
        widget = WorkspaceTreeWidget(param)
        self.assertTrue(widget is not None)


if __name__ == "__main__":
    unittest.main()
