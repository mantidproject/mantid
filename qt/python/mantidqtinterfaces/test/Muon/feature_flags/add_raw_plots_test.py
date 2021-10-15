# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.features.raw_plots import AddRawPlots
from mantidqtinterfaces.Muon.GUI.MuonAnalysis.load_widget.load_widget import LoadWidget
from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import MuonAnalysisPlotWidget


class AddRawPlotsTest(unittest.TestCase):

    def setUp(self):
        self.GUI = mock.Mock()
        self.GUI.load_widget = mock.MagicMock(autospec=LoadWidget)
        self.GUI.plot_widget = mock.MagicMock(autospec=MuonAnalysisPlotWidget)

        return

    def test_get_features_success(self):
        test = {"raw_plots":1}
        AddRawPlots(self.GUI, test)
        self.GUI.plot_widget.create_raw_pane.assert_called_once_with()

    def test_get_features_fail(self):
        test = {"raw_plots":0}
        AddRawPlots(self.GUI, test)
        self.GUI.plot_widget.create_raw_pane.assert_not_called()

    def test_add_observers_to_feature(self):
        test = {"raw_plots":1}
        add = AddRawPlots(self.GUI, test)
        add.add_observers_to_feature(self.GUI)
        self.GUI.load_widget.load_widget.loadNotifier.add_subscriber.assert_called_once_with(
                self.GUI.plot_widget.raw_mode.new_data_observer)

    def test_add_observers_to_feature_fail(self):
        test = {"raw_plots":0}
        add = AddRawPlots(self.GUI, test)
        add.add_observers_to_feature(self.GUI)
        self.GUI.load_widget.load_widget.loadNotifier.add_subscriber.assert_not_called()


if __name__ == '__main__':
    unittest.main()
