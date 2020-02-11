# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FunctionFactory, MultiDomainFunction
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter import SeqFittingTabPresenter
from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class SeqFittingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context()

        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.presenter = SeqFittingTabPresenter(self.view, self.model, self.context)

    def test_handle_workspaces_changed_correctly_updates_view_from_model(self):
        run_list = ["2224", "2225"]
        group_pair_list = ["fwd", "bwd"]
        self.presenter.model.get_runs_groups_and_pairs_for_fits = mock.MagicMock(
            return_value=[run_list, group_pair_list])

        self.presenter.handle_selected_workspaces_changed()

        self.presenter.model.get_runs_groups_and_pairs_for_fits.assert_called_once()
        self.view.set_fit_table_workspaces.assert_called_with(run_list, group_pair_list)

    def test_handle_fit_function_changed_correctly_updates_fit_table_parameters(self):
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.model.fit_function = fit_function

        self.presenter.handle_fit_function_updated()

        self.view.set_fit_table_function_parameters.assert_called_once_with(['A', 'Sigma', 'Frequency', 'Phi'],
                                                                            [0.2, 0.2, 0.1, 0])

    @mock.patch("functools.partial")
    def test_handle_single_fit_correctly_sets_up_fit(self, mock_fit_function):
        workspace = "EMU20884; Group; fwd; Asymmetry"
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.model.fit_function = fit_function
        self.model.evaluate_single_fit = mock.MagicMock()

        self.presenter.get_workspaces_for_entry_in_fit_table = mock.MagicMock(return_value=workspace)

        self.presenter.handle_single_fit_requested()

        mock_fit_function.assert_called_once_with(self.model.evaluate_single_fit, workspace, False)

    @mock.patch("functools.partial")
    def test_handle_single_fit_does_nothing_if_fit_function_is_none(self, mock_fit_function):
        self.model.fit_function = None

        self.presenter.handle_single_fit_requested()

        mock_fit_function.assert_not_called()

    @mock.patch("functools.partial")
    def test_handle_single_fit_does_nothing_if_no_fit_selected(self, mock_fit_function):
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.model.fit_function = fit_function
        self.view.get_selected_row = mock.MagicMock(return_value=-1)

        self.presenter.handle_single_fit_requested()

        mock_fit_function.assert_not_called()

    def test_handle_single_fit_started_updates_view(self):
        self.presenter.handle_single_fit_started()

        self.view.fit_selected_button.setEnabled.assert_called_once_with(False)

    def test_handle_single_fit_error_informs_view(self):
        error = 'Input workspace not defined'

        self.presenter.handle_single_fit_error(error)

        self.view.warning_popup.assert_called_once_with(error)
        self.view.fit_selected_button.setEnabled.assert_called_once_with(True)

    def test_handle_single_fit_finished_updates_view(self):
        self.presenter.selected_row = 0
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.6,Sigma=0.9,Frequency=0.10,Phi=1')
        self.presenter.fitting_calculation_model = mock.MagicMock()
        self.presenter.fitting_calculation_model.result = (fit_function, 'Success', 1.07)

        self.presenter.handle_single_fit_finished()

        self.view.set_fit_function_parameters.assert_called_once_with(0, [0.6, 0.9, 0.1, 1])
        self.view.set_fit_quality.assert_called_once_with(0, 'Success', 1.07)
        self.view.fit_selected_button.setEnabled.assert_called_once_with(True)

    @mock.patch("functools.partial")
    def test_handle_sequential_fit_correctly_sets_up_fit(self, mock_fit_function):
        workspaces = ["EMU20884; Group; fwd; Asymmetry"]
        number_of_entries = 3
        fit_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        self.model.fit_function = fit_function
        self.model.evaluate_sequential_fit = mock.MagicMock()
        self.view.get_number_of_entries = mock.MagicMock(return_value=number_of_entries)
        self.presenter.get_workspaces_for_entry_in_fit_table = mock.MagicMock(return_value=workspaces)

        self.presenter.handle_sequential_fit_requested()

        self.assertEqual(self.presenter.get_workspaces_for_entry_in_fit_table.call_count, number_of_entries)
        mock_fit_function.assert_called_once_with(self.model.evaluate_sequential_fit, [workspaces] * number_of_entries,
                                                  False)

    @mock.patch("functools.partial")
    def test_handle_sequential_fit_does_nothing_if_fit_function_is_none(self, mock_fit_function):
        self.model.fit_function = None

        self.presenter.handle_sequential_fit_requested()

        mock_fit_function.assert_not_called()

    def test_handle_sequential_fit_started_updates_view(self):
        self.presenter.handle_seq_fit_started()

        self.view.seq_fit_button.setEnabled.assert_called_once_with(False)

    def test_handle_sequential_fit_error_informs_view(self):
        error = 'Input workspace not defined'

        self.presenter.handle_seq_fit_error(error)

        self.view.warning_popup.assert_called_once_with(error)
        self.view.seq_fit_button.setEnabled.assert_called_once_with(True)

    def test_handle_sequential_fit_finished_updates_view(self):
        number_of_entries = 3
        fit_functions = [FunctionFactory.createInitialized('name=GausOsc,A=0.2,'
                                                           'Sigma=0.2,Frequency=0.1,Phi=1')] * number_of_entries
        fit_status = ['Success'] * number_of_entries
        fit_quality = [1.3, 2.4, 1.9]

        self.presenter.fitting_calculation_model = mock.MagicMock()
        self.presenter.fitting_calculation_model.result = (fit_functions, fit_status, fit_quality)

        self.presenter.handle_seq_fit_finished()

        self.assertEqual(self.view.set_fit_function_parameters.call_count, number_of_entries)
        call_list = [mock.call(0, [0.2, 0.2, 0.1, 1]),
                     mock.call(1, [0.2, 0.2, 0.1, 1]),
                     mock.call(2, [0.2, 0.2, 0.1, 1])]
        self.view.set_fit_function_parameters.assert_has_calls(call_list)

        self.assertEqual(self.view.set_fit_quality.call_count, number_of_entries)
        call_list = [mock.call(0, 'Success', fit_quality[0]),
                     mock.call(1, 'Success', fit_quality[1]),
                     mock.call(2, 'Success', fit_quality[2])]
        self.view.set_fit_quality.assert_has_calls(call_list)

        self.view.seq_fit_button.setEnabled.assert_called_once_with(True)

    def test_get_workspaces_for_entry_in_fit_table_calls_model_correctly(self):
        run = "2224"
        groups = "bwd;fwd;top"
        self.view.get_workspace_info_from_fit_table_row = mock.MagicMock(return_value=[run, groups])

        self.presenter.get_workspaces_for_entry_in_fit_table(0)

        self.model.get_fit_workspace_names_from_groups_and_runs.assert_called_once_with([run], ["bwd", "fwd", "top"])

    def test_fit_uses_table_values_for_fit(self):
        self.view.get_entry_fit_parameters = mock.MagicMock(return_value=[0.25, 0.35, 0.6])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
