# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.plot_widget.model_fit_pane.plot_model_fit_pane_model import PlotModelFitPaneModel
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class MockFitInfo(object):
    def __init__(self, name):
        self.fit = "FlatBackground"
        self.tf_asymmetry_fit = False
        self.input_workspaces = name

    def output_workspace_names(self):
        return self.input_workspaces


@start_qapplication
class PlotModelFitPaneModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotModelFitPaneModel(context=self.context)

    def test_get_fit_ws_and_indicies(self):
        fit = MockFitInfo([self.model.name, "unit"])

        ws, indices = self.model.get_fit_workspace_and_indices(fit, False)

        self.assertEqual(ws, [self.model.name, "unit"])
        self.assertEqual(indices, [1, 1])

    def test_get_fit_ws_and_indicies_with_diff(self):
        fit = MockFitInfo([self.model.name, "unit"])

        ws, indices = self.model.get_fit_workspace_and_indices(fit, True)

        self.assertEqual(ws, [self.model.name, self.model.name, "unit", "unit"])
        self.assertEqual(indices, [1, 2, 1, 2])

    def test_that_get_fit_label_returns_the_expected_labels_for_different_indices(self):
        self.assertEqual(self.model._get_fit_label(0), "")
        self.assertEqual(self.model._get_fit_label(1), ";Calc")
        self.assertEqual(self.model._get_fit_label(2), ";Diff")
        self.assertEqual(self.model._get_fit_label(3), "")

    def test_that_create_workspace_label_returns_the_expected_workspace_label(self):
        workspace_name = "Results Table; Sigma vs A"
        self.assertEqual(self.model._create_workspace_label(workspace_name, 0), workspace_name)
        self.assertEqual(self.model._create_workspace_label(workspace_name, 1), workspace_name + ";Calc")
        self.assertEqual(self.model._create_workspace_label(workspace_name, 2), workspace_name + ";Diff")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
