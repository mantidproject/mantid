# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.Common.plotting_widget.plotting_widget_presenter import PlotWidgetPresenter
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.contexts.fitting_context import FitInformation
from mantid.simpleapi import CreateWorkspace


@start_qapplication
class PlottingWidgetPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()
        self.context.fitting_context.number_of_fits = 1
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA',
                               'MUSR62261; Group; bottom; Asymmetry; MA']
        self.context.data_context.instrument = "MUSR"
        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.presenter = PlotWidgetPresenter(self.view, self.model, self.context)
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')
        self.view.is_tiled_plot = mock.MagicMock(return_value=False)
        self.view.plot_options.get_errors = mock.MagicMock(return_value=True)
        self.presenter.get_x_limits = mock.MagicMock(return_value=[0, 15])
        self.presenter.get_x_lim_from_subplot = mock.MagicMock(return_value=[0, 15])
        self.presenter.get_y_lim_from_subplot = mock.MagicMock(return_value=[-1, 1])

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    def test_handle_workspace_deleted_from_ads_calls_delete_workspace_correctly(self):
        self.model.plotted_workspaces = ['fwd', 'bwd']
        ws = self.create_workspace('bwd')
        self.presenter.handle_workspace_deleted_from_ads(ws)

        self.model.workspace_deleted_from_ads.assert_called_once_with(ws, self.view.get_axes())

    def test_handle_workspace_replaced_in_ads_calls_update_workspace_correctly(self):
        self.model.plotted_workspaces = ['fwd', 'bwd']
        self.presenter.handle_workspace_replaced_in_ads('bwd')

        self.model.replace_workspace_plot.assert_called_once_with('bwd', self.view.get_axes()[0])

    def test_use_rebin_changed_resets_use_raw_to_true_if_no_rebin_specified(self):
        self.view.if_raw.return_value = False
        self.context._do_rebin.return_value = False

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.add_workspace_to_plot.assert_not_called()
        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_use_rebin_changed_replots_figure_with_appropriate_data(self):
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        workspace_indices = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}
        self.presenter.handle_use_raw_workspaces_changed()

        self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[0],
                                                         self.workspace_list[0],
                                                         workspace_indices,
                                                         errors=errors,
                                                         plot_kwargs=plot_kwargs)
        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[0],
                                                            self.workspace_list[1],
                                                            workspace_indices,
                                                            errors=errors,
                                                            plot_kwargs=plot_kwargs)

    def test_get_legend_label_for_maxent_returns(self):
        workspace_name = "EMU20884 MaXEnt"
        self.presenter._view.get_selected = mock.MagicMock(return_value="Frequency MaxEnt")

        label = self.presenter.get_workspace_legend_label(workspace_name)

        self.assertEqual(label, workspace_name)

    def test_handle_data_updated_does_nothing_if_workspace_list_has_not_changed(self):
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.model.plotted_workspaces = self.workspace_list

        self.presenter.handle_data_updated()

        self.model.add_workspace_to_plot.assert_not_called()

    def test_add_workspace_to_plot_called_by_handle_data_updated_if_run_list_changed(self):
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.model.plotted_workspaces = []
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        workspace_indices = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}

        self.presenter.handle_data_updated()

        self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[0],
                                                         self.workspace_list[0],
                                                         workspace_indices,
                                                         errors=errors,
                                                         plot_kwargs=plot_kwargs)
        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[0],
                                                            self.workspace_list[1],
                                                            workspace_indices,
                                                            errors=errors,
                                                            plot_kwargs=plot_kwargs)

    def test_handle_plot_type_changed_displays_a_warning_if_trying_to_plot_counts_on_a_pair(self):
        self.context.group_pair_context.selected_pairs = ['long']
        self.view.get_selected.return_value = 'Counts'

        self.presenter.handle_plot_type_changed()

        self.model.add_workspace_to_plot.assert_not_called()
        self.view.warning_popup.assert_called_once_with(
            'Pair workspaces have no counts workspace, remove pairs from analysis and retry')

    def test_handle_plot_type_changed_calls_add_workspace_to_plot(self):
        self.context.group_pair_context.__getitem__.return_value = MuonGroup('bottom', [])
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=self.workspace_list)
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        self.view.get_selected.return_value = 'Counts'
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False,
                       'label': 'label'}

        self.presenter.handle_plot_type_changed()

        self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[0], self.workspace_list[0], [0],
                                                         errors=True, plot_kwargs=plot_kwargs)
        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[0], self.workspace_list[1], [0],
                                                            errors=True, plot_kwargs=plot_kwargs)

    def test_handle_added_group_or_pair_to_plot_does_nothing_if_selected_groups_or_pairs_not_changed(self):
        self.model.plotted_workspaces = ['bottom']
        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=True)

        self.model.add_workspace_to_plot.assert_not_called()

    def test_handle_added_group_or_pair_to_plot_calls_add_workspace_to_plot(self):
        self.model.plotted_workspaces = []
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=[self.workspace_list[0]])
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False,
                       'label': 'label'}
        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=True)

        self.model.add_workspace_to_plot.assert_called_once_with(self.view.get_axes()[0], self.workspace_list[0], [0],
                                                                 errors=True, plot_kwargs=plot_kwargs)

    def test_handle_remove_group_or_pair_to_plot_calls_remove_workspace_to_plot(self):
        self.model.plotted_workspaces = ['bottom', 'fwd']

        # handle fwd group being removed from the workspaces to plot
        self.presenter.workspace_finder.get_workspaces_to_plot = mock.MagicMock(return_value=['bottom'])
        self.presenter.handle_added_or_removed_group_or_pair_to_plot(is_added=False)

        self.model.remove_workspace_from_plot.assert_called_once_with('fwd', self.view.get_axes())

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

        # check fit and diff workspaces plotted
        self.model.add_workspace_to_plot.assert_any_call(self.view.get_axes()[0],
                                                         'MUSR62260; Group; bottom; Asymmetry; MA; Fitted;', [1],
                                                         errors=False, plot_kwargs=mock.ANY)
        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[0],
                                                            'MUSR62260; Group; bottom; Asymmetry; MA; Fitted;', [2],
                                                            errors=False, plot_kwargs=mock.ANY)

    def test_handle_plot_guess_changed_removes_all_guesses_if_workspace_is_none(self):
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = None
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4']
        call_list = [mock.call('ws2_guess', self.view.get_axes()), mock.call('ws3_guess', self.view.get_axes())]
        self.presenter.handle_plot_guess_changed()

        self.assertEqual(0, self.model.add_workspace_to_plot.call_count)
        self.assertEqual(2, self.model.remove_workspace_from_plot.call_count)
        self.model.remove_workspace_from_plot.assert_has_calls(call_list)

    def test_handle_plot_guess_changed_adds_guess_to_plot_and_removes_previous_guess_if_present(self):
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = 'ws_guess'
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4', 'ws_guess']
        self.model.number_of_axes = 1
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'Fit Function Guess'}

        self.presenter.handle_plot_guess_changed()

        self.assertEqual(3, self.model.remove_workspace_from_plot.call_count)
        self.assertEqual(1, self.model.add_workspace_to_plot.call_count)
        self.model.remove_workspace_from_plot.assert_called_with('ws_guess', self.view.get_axes())
        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[0], 'ws_guess',
                                                            workspace_indices=[1], errors=False,
                                                            plot_kwargs=plot_kwargs)

    def test_handle_plot_guess_changed_adds_guess_to_plot(self):
        self.context.fitting_context.plot_guess = True
        self.context.fitting_context.guess_ws = 'ws_guess'
        self.model.plotted_fit_workspaces = ['ws1', 'ws2_guess', 'ws3_guess', 'ws4']
        self.model.number_of_axes = 1
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'Fit Function Guess'}

        self.presenter.handle_plot_guess_changed()

        self.assertEqual(2, self.model.remove_workspace_from_plot.call_count)
        self.assertEqual(1, self.model.add_workspace_to_plot.call_count)
        self.model.add_workspace_to_plot.assert_called_with(self.view.get_axes()[0], 'ws_guess',
                                                            workspace_indices=[1], errors=False,
                                                            plot_kwargs=plot_kwargs)

    def test_handle_subplot_changes_correctly_retrieves_axis_limits(self):
        subplot = '1'
        self.view.plot_options.get_selection.return_value = [subplot]

        self.presenter.handle_subplot_changed_in_options()

        self.presenter.get_x_lim_from_subplot.assert_called_with(int(subplot) - 1)
        self.presenter.get_y_lim_from_subplot.assert_called_with(int(subplot) - 1)

    def test_handle_subplot_changes_correctly_updates_options(self):
        subplot = '1'
        self.view.plot_options.get_selection.return_value = [subplot]

        self.presenter.handle_subplot_changed_in_options()

        self.view.plot_options.set_plot_x_range.assert_called_with(self.presenter.get_x_lim_from_subplot())
        self.view.plot_options.set_plot_y_range.assert_called_with(self.presenter.get_y_lim_from_subplot())

    def test_handle_subplot_changes_does_not_update_options_if_all_subplots_selected(self):
        subplots = ['1', '2', '3', '4']
        self.view.plot_options.get_selection.return_value = subplots

        self.presenter.handle_subplot_changed_in_options()

        self.view.plot_options.set_plot_x_range.assert_not_called()
        self.view.plot_options.set_plot_y_range.assert_not_called()

    def test_handle_autoscale_correctly_updates_axis(self):
        subplot = '1'
        xlims = [0, 10]
        ylims = [-3, 3]
        self.view.plot_options.get_selection.return_value = [subplot]
        self.view.plot_options.get_plot_x_range.return_value = xlims
        self.view.plot_options.set_plot_y_range = mock.MagicMock()
        self.model.autoscale_axes = mock.MagicMock(return_value=ylims)

        self.presenter.handle_autoscale_requested_in_view()

        self.model.autoscale_axes.assert_called_with(mock.ANY, xlims)
        self.view.plot_options.set_plot_y_range.assert_called_once_with(ylims)

    def test_handle_x_axis_limits_changed_in_options_updates_plot_correctly(self):
        subplot = '1'
        self.view.plot_options.get_selection.return_value = [subplot]
        self.model.set_axis_xlim = mock.MagicMock()
        xlims = [0, 10]

        self.presenter.handle_xlim_changed_in_options_view(xlims)

        self.model.set_axis_xlim.assert_called_once_with(mock.ANY, xlims)

    def test_handle_y_axis_limits_changed_in_options_updates_plot_correctly(self):
        subplot = '1'
        self.view.plot_options.get_selection.return_value = [subplot]
        self.model.set_axis_ylim = mock.MagicMock()
        ylims = [-1, 1]

        self.presenter.handle_ylim_changed_in_options_view(ylims)

        self.model.set_axis_ylim.assert_called_once_with(mock.ANY, ylims)

    def test_handle_x_axis_limits_changed_in_figure_updates_plot_correctly(self):
        subplot = '1'
        xlims = [0, 15]
        self.view.plot_options.get_selection.return_value = [subplot]
        self.view.plot_options.set_plot_x_range = mock.MagicMock()
        axis = self.view.get_axes()[0]
        axis.get_xlim.return_value = xlims

        self.presenter.handle_x_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_x_range.assert_called_once_with(xlims)

    def test_handle_x_axis_limits_changed_in_figure_does_not_update_if_all_subplots_chosen(self):
        subplots = ['1,2,3,4']
        self.view.plot_options.get_selection.return_value = subplots
        axis = self.view.get_axes()[0]
        self.view.plot_options.set_plot_x_range = mock.MagicMock()

        self.presenter.handle_x_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_x_range.assert_not_called()

    def test_handle_y_axis_limits_changed_in_figure_updates_plot_correctly(self):
        subplot = '1'
        ylims = [-1, 1]
        self.view.plot_options.get_selection.return_value = [subplot]
        self.view.plot_options.set_plot_y_range = mock.MagicMock()
        axis = self.view.get_axes()[0]
        axis.get_ylim.return_value = ylims

        self.presenter.handle_y_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_y_range.assert_called_once_with(ylims)

    def test_handle_y_axis_limits_changed_in_figure_does_not_update_if_all_subplots_chosen(self):
        subplots = ['1,2,3,4']
        self.view.plot_options.get_selection.return_value = subplots
        axis = self.view.get_axes()[0]
        self.view.plot_options.set_plot_x_range = mock.MagicMock()

        self.presenter.handle_y_axis_limits_changed_in_figure_view(axis)

        self.view.plot_options.set_plot_y_range.assert_not_called()

    def test_handle_error_selection_changed_correctly_replots_workspace_data(self):
        errors = True
        self.model.replot_workspace = mock.MagicMock()
        workspaces = ["fwd", "bwd"]
        self.model.plotted_workspaces = workspaces
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='label')

        self.presenter.handle_error_selection_changed(errors)

        self.assertEqual(self.model.replot_workspace.call_count, len(workspaces))
        self.model.replot_workspace.assert_any_call(workspaces[0], self.view.get_axes()[0], errors, mock.ANY)
        self.model.replot_workspace.assert_called_with(workspaces[1], self.view.get_axes()[0], errors, mock.ANY)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
