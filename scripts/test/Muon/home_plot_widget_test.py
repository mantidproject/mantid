# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from Muon.GUI.Common.home_plot_widget.home_plot_widget_presenter import HomePlotWidgetPresenter
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_group import MuonGroup


class HomeTabPlotPresenterTest(GuiTest):
    def setUp(self):
        self.context = mock.MagicMock()
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
                                                'MUSR62261; Group; bottom; Asymmetry; MA'], 'MUSR62260-62261 bottom')

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

        self.model.plot.assert_called_once_with(self.workspace_list, 'MUSR62260-62261 bottom')

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

        self.model.plot.assert_called_once_with(self.workspace_list, 'MUSR62260-62261 bottom')

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

        self.model.plot.assert_called_once_with(self.workspace_list, 'MUSR62260-62261 bottom')

    def test_handle_fit_completed_adds_appropriate_fits_to_plot(self):
        self.model.plotted_workspaces = self.workspace_list
        mock_fit = mock.MagicMock()
        mock_fit.parameter_name = 'MUSR62260; Group; bottom; Asymmetry; MA; Fitted; Parameters'
        self.context.fitting_context.find_fit_for_input_workspace_name.return_value =\
            [mock_fit]

        self.presenter.handle_fit_completed()

        self.assertEqual(self.model.add_workspace_to_plot.call_count, 2)
        self.model.add_workspace_to_plot.assert_called_with('MUSR62260; Group; bottom; Asymmetry; MA; Fitted;', 2)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
