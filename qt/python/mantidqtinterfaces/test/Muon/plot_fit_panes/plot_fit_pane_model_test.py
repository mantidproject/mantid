# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateWorkspace
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_model import PlotFitPaneModel
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


class MockFitInfo(object):
    def __init__(self, name):
        self.fit = "FlatBackground"
        self.tf_asymmetry_fit = False
        self.input_workspaces = name

    def output_workspace_names(self):
        return self.input_workspaces


@start_qapplication
class PlotFitPaneModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotFitPaneModel(context=self.context, name="test")

    def test_get_fit_ws_and_indicies(self):
        fit = MockFitInfo(["test", "unit"])

        ws, indices = self.model.get_fit_workspace_and_indices(fit, False)

        self.assertEqual(ws, ["test", "unit"])
        self.assertEqual(indices, [1,1])

    def test_get_fit_ws_and_indicies_with_diff(self):
        fit = MockFitInfo(["test", "unit"])

        ws, indices = self.model.get_fit_workspace_and_indices(fit, True)

        self.assertEqual(ws, ["test", "test", "unit", "unit"])
        self.assertEqual(indices, [1,2,1,2])

    def test_get_shade_lines(self):
        ws = CreateWorkspace(OutputWorkspace="test",
                             DataX=[0,1,2,3,4,5,6,7,8],
                             DataY=[2,1,0,5,4,3,8,7,6],
                             DataE=[.1,.2,.3,.4,.5,.6,.7,.8,.9],
                             NSpec=3)

        x_data, y1, y2 = self.model.get_shade_lines(ws, 1)
        self.assertEqual(x_data.tolist(), [3, 4, 5])
        # y1 = y_data + e_data
        self.assertEqual(y1.tolist(), [5.4, 4.5, 3.6])
        # y2 = y_data - e_data
        self.assertEqual(y2.tolist(), [4.6, 3.5, 2.4])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
