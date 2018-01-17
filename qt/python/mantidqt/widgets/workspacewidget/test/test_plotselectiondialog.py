#  This file is part of the mantidqt package
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
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# std imports
import unittest

# 3rdparty imports
from mantid.simpleapi import CreateSampleWorkspace, CropWorkspace

# local imports
from mantidqt.utils.qt.testing import requires_qapp
from mantidqt.widgets.workspacewidget.workspacetreewidget import PlotSelectionDialog


@requires_qapp
class PlotSelectionDialogTest(unittest.TestCase):

    def test_filling_workspace_details_single_workspace(self):
        workspaces = [CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)]
        dlg = PlotSelectionDialog(workspaces)
        self.assertEqual("1-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("0-199", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspaces_of_same_size(self):
        workspaces = [CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False),
                      CreateSampleWorkspace(OutputWorkspace='ws2', StoreInADS=False)]
        dlg = PlotSelectionDialog(workspaces)
        self.assertEqual("1-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("0-199", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspaces_of_different_sizes(self):
        ws1 = CreateSampleWorkspace(OutputWorkspace='ws', NumBanks=1, StoreInADS=False)
        ws1 = CropWorkspace(ws1, StartWorkspaceIndex=50)
        ws2 = CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)

        dlg = PlotSelectionDialog([ws1, ws2])
        self.assertEqual("51-100", dlg._ui.specNums.placeholderText())
        self.assertEqual("0-49", dlg._ui.wkspIndices.placeholderText())


if __name__ == '__main__':
    unittest.main()
