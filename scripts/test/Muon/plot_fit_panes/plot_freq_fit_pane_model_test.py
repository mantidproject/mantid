# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_model import PlotFreqFitPaneModel
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
class PlotFreqFitPaneModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotFreqFitPaneModel(context=self.context)

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

    def test_get_fft_label(self):
        ws_name = 'FFT; Re MUSR62260; Pair Asym; long; FD_Re'
        self.assertEqual(';FFT;Re', self.model._get_freq_label(ws_name))

    def test_get_maxent_label(self):
        ws_name = 'MUSR62260_raw_data FD; MaxEnt'
        self.assertEqual(';MaxEnt', self.model._get_freq_label(ws_name))

    def test_get_label_when_not_fft_or_maxent(self):
        ws_name = 'test'
        self.assertEqual('', self.model._get_freq_label(ws_name))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
