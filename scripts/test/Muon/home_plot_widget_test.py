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


@start_qapplication
class HomeTabPlotPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()
        self.context.fitting_context.number_of_fits = 1
        self.plotting_window_model = mock.MagicMock()
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA',
                               'MUSR62261; Group; bottom; Asymmetry; MA']

        self.presenter = HomePlotWidgetPresenter(self.view, self.model, self.context)
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')

    def test_use_rebin_changed_resets_use_raw_to_true_if_no_rebin_specified(self):
        self.view.if_raw.return_value = False
        self.context._do_rebin.return_value = False

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.plot.assert_not_called()
        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_use_rebin_changed_replots_figure_with_appropriate_data(self):
        self.presenter.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.plot.assert_called_once_with(['MUSR62260; Group; bottom; Asymmetry; MA',
                                                 'MUSR62261; Group; bottom; Asymmetry; MA'], 'MUSR62260-62261 bottom', 'Time', False, mock.ANY)

    def test_handle_data_updated_does_nothing_if_workspace_list_has_not_changed(self):
        self.presenter.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.model.plotted_workspaces = self.workspace_list

        self.presenter.handle_data_updated()

        self.model.plot.assert_not_called()

    def test_plot_called_by_handle_data_updated_if_run_list_changed(self):
        self.presenter.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.model.plotted_workspaces = []
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')

        self.presenter.handle_data_updated()

        self.model.plot.assert_called_once_with(self.workspace_list, 'MUSR62260-62261 bottom', 'Time', False, mock.ANY)

    def test_handle_plot_type_changed_displays_a_warning_if_trying_to_plot_counts_on_a_pair(self):
        self.context.group_pair_context.__getitem__.return_value = MuonPair('long', 'bwd', 'fwd')
        self.view.get_selected.return_value = 'Counts'

        self.presenter.handle_plot_type_changed()

        self.model.plot.assert_not_called()
        self.view.warning_popup.assert_called_once_with('Pair workspaces have no counts workspace')

    def test_handle_plot_type_changed_calls_plot(self):
        self.context.group_pair_context.__getitem__.return_value = MuonGroup('bottom', [])
        self.presenter.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.view.get_selected.return_value = 'Counts'

        self.presenter.handle_plot_type_changed()

        self.model.plot.assert_called_once_with(self.workspace_list, 'MUSR62260-62261 bottom', 'Time', True, mock.ANY)

    def test_handle_group_pair_to_plot_changed_does_nothing_if_group_not_changed(self):
        self.model.plotted_group = 'bottom'
        self.context.group_pair_context.selected = 'bottom'

        self.presenter.handle_group_pair_to_plot_changed()

        self.model.plot.assert_not_called()

    def test_handle_group_pair_to_plot_changed_does_nothing_if_plot_window_does_not_exist(self):
        self.model.plotted_group = 'top'
        self.context.group_pair_context.selected = 'bottom'
        self.model.plot_figure = None

        self.presenter.handle_group_pair_to_plot_changed()

        self.model.plot.assert_not_called()

    def test_handle_group_pair_to_plot_calls_plot(self):
        self.model.plotted_group = 'top'
        self.context.group_pair_context.selected = 'bottom'
        self.presenter.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)

        self.presenter.handle_group_pair_to_plot_changed()

        self.model.plot.assert_called_once_with(self.workspace_list, 'MUSR62260-62261 bottom', 'Time', False, mock.ANY)

    def test_handle_fit_completed_adds_appropriate_fits_to_plot(self):
        self.model.plotted_workspaces = self.workspace_list
        self.model.plotted_workspaces_inverse_binning = {}
        fit_information = FitInformation(mock.MagicMock(),
                                         'GaussOsc',
                                         ['MUSR62260; Group; bottom; Asymmetry; MA'],
                                         ['MUSR62260; Group; bottom; Asymmetry; MA; Fitted;'])
        self.context.fitting_context.fit_list.__getitem__.return_value = fit_information
        self.context.fitting_context.number_of_fits = 1

        self.presenter.handle_fit_completed()

        self.assertEqual(self.model.add_workspace_to_plot.call_count, 2)
        self.model.add_workspace_to_plot.assert_any_call('MUSR62260; Group; bottom; Asymmetry; MA; Fitted;', 1,
                                                         'MUSR62260; Group; bottom; Asymmetry; MA; Fitted;: Fit')
        self.model.add_workspace_to_plot.assert_called_with('MUSR62260; Group; bottom; Asymmetry; MA; Fitted;', 2,
                                                            'MUSR62260; Group; bottom; Asymmetry; MA; Fitted;: Diff')

    def test_handle_plot_guess_changed_returns_if_no_plot_is_open(self):
        self.model.plot_figure = None
        self.presenter.handle_plot_guess_changed()

        self.assertEqual(0, self.context.fitting_context.plot_guess.call_count)

    def test_handle_plot_guess_changed_removes_all_guesses_if_workspace_is_none(self):
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = None
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4']
        call_list = [mock.call('ws2_guess'), mock.call('ws3_guess')]
        self.presenter.handle_plot_guess_changed()

        self.assertEqual(0, self.model.add_workspace_to_plot.call_count)
        self.assertEqual(2, self.model.remove_workpace_from_plot.call_count)
        self.model.remove_workpace_from_plot.assert_has_calls(call_list)
        self.assertEqual(1, self.model.force_redraw.call_count)

    def test_handle_plot_guess_changed_adds_guess_to_plot_and_removes_previous_guess_if_present(self):
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = 'ws_guess'
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4', 'ws_guess']
        self.presenter.handle_plot_guess_changed()

        self.assertEqual(3, self.model.remove_workpace_from_plot.call_count)
        self.assertEqual(1, self.model.add_workspace_to_plot.call_count)
        self.model.remove_workpace_from_plot.assert_called_with('ws_guess')
        self.model.add_workspace_to_plot.assert_called_with('ws_guess',
                                                            workspace_index=1,
                                                            label='Fit Function Guess')

    def test_handle_plot_guess_changed_adds_guess_to_plot(self):
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = 'ws_guess'
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4']
        self.presenter.handle_plot_guess_changed()

        self.assertEqual(2, self.model.remove_workpace_from_plot.call_count)
        self.assertEqual(1, self.model.add_workspace_to_plot.call_count)
        self.model.add_workspace_to_plot.assert_called_with('ws_guess',
                                                            workspace_index=1,
                                                            label='Fit Function Guess')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
