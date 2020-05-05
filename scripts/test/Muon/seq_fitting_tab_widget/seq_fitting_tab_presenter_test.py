# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FunctionFactory, MultiDomainFunction
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter import SeqFittingTabPresenter
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from qtpy import QtWidgets


def wait_for_thread(thread_model):
    if thread_model:
        thread_model._thread.wait()
        QtWidgets.QApplication.instance().processEvents()


@start_qapplication
class SeqFittingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context()
        self.view = mock.MagicMock()
        self.view.fit_table = mock.MagicMock()
        self.model = mock.MagicMock()
        self.presenter = SeqFittingTabPresenter(self.view, self.model, self.context)

        self.view.is_plotting_checked.return_value = False
        self.view.use_initial_values_for_fits.return_value = False
        self.presenter.create_thread = mock.MagicMock()

        self.view.fit_table.get_workspace_info_from_row = mock.MagicMock(return_value=["2224;2225", "bwd;fwd;top"])

    def _setup_test_fit_function(self, values):
        self.presenter.fitting_calculation_model = mock.MagicMock()
        self.model.get_fit_function_parameter_values = mock.MagicMock(return_value=values)
        fit_function = FunctionFactory.createInitialized(f"name=GausOsc,A={values[0]},Sigma={values[1]},"
                                                         f"Frequency={values[2]},Phi={values[3]}")
        self.model.fit_function = fit_function
        self.model.get_ws_fit_function = mock.MagicMock(return_value=fit_function)
        self.model.get_fit_function_parameter_values = mock.MagicMock(return_value=values)

        return fit_function

    def test_handle_workspaces_changed_correctly_updates_view_from_model(self):
        run_list = ["2224", "2225"]
        group_pair_list = ["fwd", "bwd"]
        self.presenter.model.get_runs_groups_and_pairs_for_fits = mock.MagicMock(
            return_value=[run_list, group_pair_list])

        self.presenter.handle_selected_workspaces_changed()

        self.presenter.model.get_runs_groups_and_pairs_for_fits.assert_called_once()
        self.view.fit_table.set_fit_workspaces.assert_called_with(run_list, group_pair_list)

    def test_handle_fit_function_changed_correctly_updates_fit_table_parameters(self):
        fit_values = [0.2, 0.2, 0.1, 0]
        fit_function = self._setup_test_fit_function(fit_values)
        self.view.fit_table.get_number_of_fits.return_value = 2

        self.presenter.handle_fit_function_updated()

        self.view.fit_table.set_parameters_and_values.assert_called_once_with(['A', 'Sigma', 'Frequency', 'Phi'],
                                                                              [fit_values, fit_values])

    def test_handle_fit_started_updates_view(self):
        self.presenter.handle_fit_started()

        self.view.seq_fit_button.setEnabled.assert_called_once_with(False)
        self.view.fit_selected_button.setEnabled.assert_called_once_with(False)

    def test_handle_fit_error_informs_view(self):
        error = 'Input workspace not defined'

        self.presenter.handle_fit_error(error)

        self.view.warning_popup.assert_called_once_with(error)
        self.view.seq_fit_button.setEnabled.assert_called_once_with(True)
        self.view.fit_selected_button.setEnabled.assert_called_once_with(True)

    @mock.patch('Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter.functools')
    def test_handle_fit_selected_correctly_sets_up_fit(self, mock_function_tools):
        workspace = "EMU20884; Group; fwd; Asymmetry"
        self.view.fit_table.get_selected_rows = mock.MagicMock(return_value=[0])
        fit_function = self._setup_test_fit_function([0.2, 0.1, 0, 0])
        self.presenter.get_workspaces_for_row_in_fit_table = mock.MagicMock(return_value=workspace)

        self.presenter.handle_fit_selected_pressed()

        mock_function_tools.partial.assert_called_once_with(self.model.evaluate_sequential_fit, [workspace], False)

    @mock.patch('Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter.functools')
    def test_handle_fit_selected_does_nothing_if_fit_function_is_none(self, mock_function_tools):
        self.model.fit_function = None
        self.view.fit_table.get_selected_rows = mock.MagicMock(return_value=[0, 1])

        self.presenter.handle_fit_selected_pressed()

        mock_function_tools.partial.assert_not_called()

    @mock.patch('Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter.functools')
    def test_handle_fit_selected_pressed_does_nothing_if_no_fit_selected(self, mock_function_tools):
        fit_function = self._setup_test_fit_function([0.2, 0.1, 0, 0])
        self.view.fit_table.get_selected_rows = mock.MagicMock(return_value=[])

        self.presenter.handle_fit_selected_pressed()

        mock_function_tools.partial.assert_not_called()

    def test_handle_seq_fit_finished_updates_view_for_single_fit(self):
        selected_row = 2
        fit_values = [0.6, 0.9, 0.1, 1]
        fit_function = self._setup_test_fit_function(fit_values)
        self.presenter.fitting_calculation_model.result = ([fit_function], ['Success'], [1.07])
        self.presenter.selected_rows = [selected_row]

        self.presenter.handle_seq_fit_finished()

        self.view.fit_table.set_parameter_values_for_row.assert_called_once_with(2, [0.6, 0.9, 0.1, 1])
        self.view.fit_table.set_fit_quality.assert_called_once_with(2, 'Success', 1.07)
        self.view.fit_selected_button.setEnabled.assert_called_once_with(True)

    @mock.patch('Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter.functools')
    def test_handle_sequential_fit_correctly_sets_up_fit(self, mock_function_tools):
        workspaces = ["EMU20884; Group; fwd; Asymmetry"]
        number_of_entries = 3
        fit_function = self._setup_test_fit_function([0.2, 0.2, 0.1, 0])
        self.view.fit_table.get_number_of_fits = mock.MagicMock(return_value=number_of_entries)
        self.presenter.get_workspaces_for_row_in_fit_table = mock.MagicMock(return_value=workspaces)

        self.presenter.handle_sequential_fit_pressed()

        self.assertEqual(self.presenter.get_workspaces_for_row_in_fit_table.call_count, number_of_entries)
        mock_function_tools.partial.assert_called_once_with(self.model.evaluate_sequential_fit,
                                                            [workspaces] * number_of_entries, False)

    @mock.patch('Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter.functools')
    def test_handle_sequential_fit_does_nothing_if_fit_function_is_none(self, mock_function_tools):
        self.model.fit_function = None

        self.presenter.handle_sequential_fit_requested()

        mock_function_tools.partial.assert_not_called()

    def test_handle_sequential_fit_finished_updates_view_for_multiple_fits(self):
        num_fits = 3
        self.presenter.selected_rows = [0, 1, 2]
        values = [0.2, 0.2, 0.1, 1]
        fit_functions = [self._setup_test_fit_function(values)] * num_fits
        fit_status = ['Success'] * num_fits
        fit_quality = [1.3, 2.4, 1.9]
        self.presenter.fitting_calculation_model.result = (fit_functions, fit_status, fit_quality)

        self.presenter.handle_seq_fit_finished()

        self.assertEqual(self.view.fit_table.set_parameter_values_for_row.call_count, num_fits)
        call_list = [mock.call(i, values) for i in range(num_fits)]
        self.view.fit_table.set_parameter_values_for_row.assert_has_calls(call_list)

        self.assertEqual(self.view.fit_table.set_fit_quality.call_count, num_fits)
        call_list = [mock.call(i, 'Success', fit_quality[i]) for i in range(num_fits)]
        self.view.fit_table.set_fit_quality.assert_has_calls(call_list)

        self.view.seq_fit_button.setEnabled.assert_called_once_with(True)

    def test_get_workspaces_for_row_in_fit_table_calls_model_correctly(self):
        run = "2224"
        groups = "bwd;fwd;top"
        self.view.fit_table.get_workspace_info_from_row = mock.MagicMock(return_value=[run, groups])

        self.presenter.get_workspaces_for_row_in_fit_table(0)

        self.model.get_fit_workspace_names_from_groups_and_runs.assert_called_once_with([run], ["bwd", "fwd", "top"])

    def test_handle_fit_selected_in_table_retrieves_correct_workspaces(self):
        selected_rows = [0, 2, 5]
        self.presenter.get_workspaces_for_row_in_fit_table = mock.MagicMock(return_value=["test"])
        self.view.fit_table.get_selected_rows.return_value = selected_rows

        self.presenter.handle_fit_selected_in_table()

        self.assertEqual(self.presenter.get_workspaces_for_row_in_fit_table.call_count, len(selected_rows))
        self.presenter.get_workspaces_for_row_in_fit_table.assert_has_calls([mock.call(0),
                                                                             mock.call(2),
                                                                             mock.call(5)])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
