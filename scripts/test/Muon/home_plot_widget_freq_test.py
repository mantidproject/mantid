# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.home_plot_widget.home_plot_widget_presenter import HomePlotWidgetPresenter
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.contexts.fitting_context import FitInformation
from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class HomeTabPlotPresenterFreqTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)
        self.plotting_window_model = mock.MagicMock()
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.workspace_list = ['MUSR62260; Group; bottom; Asymmetry; FD',
                               'MUSR62261; Group; bottom; Asymmetry; FD',
                               'FFT; Re MUSR62260; Group; fwd; Asymmetry; FD; Im MUSR62260; Group; fwd; Asymmetry; FD_Im'
                               'FFT; Re MUSR62260; Group; fwd; Asymmetry; FD; Im MUSR62260; Group; fwd; Asymmetry; FD_Re'
                               'FFT; Re MUSR62260; Group; fwd; Asymmetry; FD; Im MUSR62260; Group; fwd; Asymmetry; FD_mod'
                               'MUSR62260_raw_data FD; MaxEnt']

        self.presenter = HomePlotWidgetPresenter(self.view, self.model, self.context)
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')

    def test_time_plot_in_FDA(self):
        self.presenter.get_workspaces_to_plot = mock.MagicMock(return_value=[self.workspace_list[0],self.workspace_list[1]])
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.plot.assert_called_once_with(['MUSR62260; Group; bottom; Asymmetry; FD',
                                                 'MUSR62261; Group; bottom; Asymmetry; FD'], 'MUSR62260-62261 bottom', 'Time', False, "Frequency Domain Analysis")


    def test_plot_type_changed(self):
        self.view.get_selected.return_value =  "Asymmetry"
        self.assertEquals(self.view.get_selected(),"Asymmetry")

        self.view.get_selected.return_value =  "Frequency Re"
        self.presenter.handle_plot_type_changed()
        self.assertEquals(self.context._frequency_context.plot_type, "Re")

    def test_plot_type_changed_to_time(self):
        self.view.get_selected.return_value =  "Frequency Re"

        self.view.get_selected.return_value =  "Asymmetry"
        self.presenter.handle_plot_type_changed()
        self.assertEquals(self.context._frequency_context.plot_type, "")

    def test_get_workspace_to_plot(self):
        self.view.get_selected.return_value =  "Frequency Re"
        self.presenter.get_freq_workspaces_to_plot = mock.Mock()

        self.presenter.get_workspaces_to_plot("fwd",True, self.view.get_selected() )
        self.assertEquals(self.presenter.get_freq_workspaces_to_plot.call_count, 1)

    def test_get_workspaces_to_plot_freq(self):
        self.view.get_selected.return_value =  "Frequency Re"
        self.context.data_context.current_runs = [[62260]]
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.presenter.get_freq_workspaces_to_plot("fwd", self.view.get_selected() )

        self.assertEquals(self.context.get_names_of_frequency_domain_workspaces_to_fit.call_count, 1)
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with("62260", "fwd", True, "Re")

    def test_get_2_workspaces_to_plot_freq(self):
        self.view.get_selected.return_value =  "Frequency Re"
        self.context.data_context.current_runs = [[62260],[ 62261]]
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.presenter.get_freq_workspaces_to_plot("fwd", self.view.get_selected() )

        self.assertEquals(self.context.get_names_of_frequency_domain_workspaces_to_fit.call_count, 1)
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with("62260, 62261", "fwd", True, "Re")

    def test_get_domain_freq(self):
        self.view.get_selected.return_value =  "Frequency Re"
        self.assertEquals(self.presenter.get_domain(),"Frequency")

    def test_get_domain_time(self):
        self.view.get_selected.return_value =  "Asymmetry"
        self.assertEquals(self.presenter.get_domain(),"Time")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
