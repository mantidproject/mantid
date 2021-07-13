# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class MockFitInfo(object):
    def __init__(self, name):
        self.fit = "FlatBackground"
        self.input_workspaces = name
        self.tf_asymmetry_fit = False

    def output_workspace_names(self):
        return self.input_workspaces


@start_qapplication
class PlotTimeFitPaneModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotTimeFitPaneModel(context=self.context)

    def test_get_fit_ws_and_indicies(self):
        fit = MockFitInfo(["test","unit"])
        ws, indices = self.model.get_fit_workspace_and_indices(fit, False)

        self.assertEqual(ws, ["test", "unit"])
        self.assertEqual(indices, [1,1])

    def test_get_fit_ws_and_indicies_with_diff(self):
        fit = MockFitInfo(["test","unit"])

        ws, indices = self.model.get_fit_workspace_and_indices(fit, True)

        self.assertEqual(ws, ["test", "test", "unit", "unit"])
        self.assertEqual(indices, [1,2,1,2])

    def test_get_fit_ws_and_indicies_TF(self):
        fit = MockFitInfo(["test","unit"])
        fit.tf_asymmetry_fit = True

        ws, indices = self.model.get_fit_workspace_and_indices(fit, False)

        self.assertEqual(ws, ["test", "unit"])
        self.assertEqual(indices, [3,3])

    def test_get_fit_ws_and_indicies_with_diff_TF(self):
        fit = MockFitInfo(["test","unit"])
        fit.tf_asymmetry_fit = True

        ws, indices = self.model.get_fit_workspace_and_indices(fit, True)

        self.assertEqual(ws, ["test", "test", "unit", "unit"])
        self.assertEqual(indices, [3,2,3,2])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
