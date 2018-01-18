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
from qtpy.QtWidgets import QApplication, QDialogButtonBox

# local imports
from mantidqt.utils.qt.testing import requires_qapp
from mantidqt.widgets.workspacewidget.plotselectiondialog import PlotSelectionDialog


@requires_qapp
class PlotSelectionDialogTest(unittest.TestCase):

    def test_initial_dialog_setup(self):
        workspaces = [CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)]
        dlg = PlotSelectionDialog(workspaces)
        self.assertFalse(dlg._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_filling_workspace_details_single_workspace(self):
        workspaces = [CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)]
        dlg = PlotSelectionDialog(workspaces)
        self.assertEqual("valid range: 1-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-199", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspaces_of_same_size(self):
        workspaces = [CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False),
                      CreateSampleWorkspace(OutputWorkspace='ws2', StoreInADS=False)]
        dlg = PlotSelectionDialog(workspaces)
        self.assertEqual("valid range: 1-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-199", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspaces_of_different_sizes(self):
        ws1 = CreateSampleWorkspace(OutputWorkspace='ws', NumBanks=1, StoreInADS=False)
        ws1 = CropWorkspace(ws1, StartWorkspaceIndex=50)
        ws2 = CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)

        dlg = PlotSelectionDialog([ws1, ws2])
        self.assertEqual("valid range: 51-100", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-49", dlg._ui.wkspIndices.placeholderText())

    def test_valid_text_in_boxes_activates_ok(self):
        workspaces = [CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)]
        dlg = PlotSelectionDialog(workspaces)

        def do_test(input_box):
            input_box.setText("1")
            self.assertTrue(dlg._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())
            input_box.clear()
            self.assertFalse(dlg._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

        do_test(dlg._ui.wkspIndices)
        do_test(dlg._ui.specNums)

    def test_plot_all_gives_only_workspaces_indices(self):
        ws = CreateSampleWorkspace(OutputWorkspace='ws', StoreInADS=False)
        dlg = PlotSelectionDialog([ws])
        dlg._ui.buttonBox.button(QDialogButtonBox.YesToAll).click()
        self.assertTrue(dlg.user_selection is not None)
        self.assertTrue(dlg.user_selection.spectra is None)
        self.assertEqual(range(200), dlg.user_selection.wksp_indices)


if __name__ == '__main__':
    unittest.main()
