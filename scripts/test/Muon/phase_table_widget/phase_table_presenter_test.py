# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.observer_pattern import GenericObservable
from qtpy.QtWidgets import QApplication
from qtpy import QtCore
from Muon.GUI.Common.phase_table_widget.phase_table_presenter import PhaseTablePresenter
from Muon.GUI.Common.phase_table_widget.phase_table_view import PhaseTableView
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_phasequad import MuonPhasequad
from Muon.GUI.Common.test_helpers.context_setup import setup_context


def phase_table_name_side_effect():
    return ""


@start_qapplication
class PhaseTablePresenterTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            QApplication.sendPostedEvents()

    def setUp(self):
        self.view = PhaseTableView()
        self.context = setup_context()
        self.context.data_context.instrument = 'MUSR'

        self.presenter = PhaseTablePresenter(self.view, self.context)

        forward_group = MuonGroup(group_name="fwd", detector_ids=[1, 3, 5, 7, 9])
        backward_group = MuonGroup(group_name="bwd", detector_ids=[2, 4, 6, 8, 10])

        self.context.group_pair_context.add_group(forward_group)
        self.context.group_pair_context.add_group(backward_group)
        self.presenter.update_current_groups_list()

        self.view.warning_popup = mock.MagicMock()
        self.view.enter_phase_table_name = mock.Mock()
        self.view.enter_phase_table_name.side_effect = phase_table_name_side_effect

    def test_update_view_from_model_updates_view_to_have_correct_values(self):
        self.presenter.update_view_from_model()

        for key, item in self.context.phase_context.options_dict.items():
            self.assertEqual(getattr(self.view, key), item)

    def test_update_model_from_view_updates_model_to_have_correct_values_if_view_changed(self):
        workspace_name = 'new_workspace_name'
        self.view.set_input_combo_box([workspace_name])
        self.view.input_workspace = workspace_name

        self.presenter.update_model_from_view()

        self.assertEqual(self.context.phase_context.options_dict['input_workspace'], workspace_name)

    def test_create_parameters_for_cal_muon_phase_returns_correct_parameter_dict(self):
        workspace_name = 'input_workspace_name_raw_data'
        self.context.phase_context.options_dict['input_workspace'] = workspace_name

        result = self.presenter.create_parameters_for_cal_muon_phase_algorithm()

        self.assertEqual(result,
                         {'BackwardSpectra': [2, 4, 6, 8, 10], 'FirstGoodData': 0.1, 'ForwardSpectra': [1, 3, 5, 7, 9],
                          'InputWorkspace': workspace_name, 'LastGoodData': 15,
                          'DetectorTable': 'input_workspace_name; PhaseTable; fwd; bwd'})

    def test_correctly_retrieves_workspace_names_associsated_to_current_runs(self):
        self.view.set_input_combo_box = mock.MagicMock()
        self.context.getGroupedWorkspaceNames = mock.MagicMock(return_value=['MUSR22222', 'MUSR44444'])

        self.presenter.update_current_run_list()

        self.view.set_input_combo_box.assert_called_once_with(['MUSR22222', 'MUSR44444'])

    def test_correctly_retrieves_names_of_current_groups(self):
        self.view.set_group_combo_boxes = mock.MagicMock()

        self.presenter.update_current_groups_list()

        self.view.set_group_combo_boxes.assert_called_once_with(['fwd', 'bwd'])

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.MuonWorkspaceWrapper')
    def test_that_phase_table_added_to_ADS_with_correct_name_and_group(self, mock_workspace_wrapper):
        workspace_wrapper = mock.MagicMock()
        mock_workspace_wrapper.return_value = workspace_wrapper

        self.presenter.add_phase_table_to_ADS('MUSR22222_period_1; PhaseTable')

        mock_workspace_wrapper.assert_called_once_with('MUSR22222 MA/MUSR22222_period_1; PhaseTable')
        workspace_wrapper.show.assert_called_once_with()

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_CalMuonDetectorPhases')
    def test_handle_calculate_phase_table_clicked_behaves_correctly_for_succesful_calculation(self, run_algorithm_mock):
        detector_table_mock = mock.MagicMock()
        self.view.set_input_combo_box(['MUSR22222_raw_data_period_1'])
        self.context.getGroupedWorkspaceNames = mock.MagicMock(return_value=['MUSR22222_raw_data_period_1'])
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        self.presenter.update_view_from_model()
        run_algorithm_mock.return_value = (detector_table_mock, mock.MagicMock())
        self.presenter.add_phase_table_to_ADS = mock.MagicMock()

        self.presenter.update_current_run_list()
        self.presenter.handle_calculate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.presenter.add_phase_table_to_ADS.assert_called_once_with(detector_table_mock)
        self.assertTrue(self.view.isEnabled())

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_CalMuonDetectorPhases')
    def test_handle_calculate_phase_table_clicked_behaves_correctly_for_error_in_calculation(self, run_algorithm_mock):
        self.context.getGroupedWorkspaceNames = mock.MagicMock(return_value=['MUSR22222_raw_data_period_1'])
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        self.presenter.update_view_from_model()
        run_algorithm_mock.side_effect = RuntimeError('CalMuonDetectorPhases has failed')
        self.presenter.add_phase_table_to_ADS = mock.MagicMock()
        self.presenter.calculate_base_name_and_group = mock.MagicMock(
            return_value=('MUSR22222_raw_data_period_1', 'MUSR22222 PhaseTable'))

        self.presenter.update_current_run_list()
        self.presenter.handle_calculate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.assertTrue(self.view.isEnabled())
        self.view.warning_popup.assert_called_once_with('CalMuonDetectorPhases has failed')

    def test_update_current_phase_table_list_retrieves_all_correct_tables(self):
        self.view.set_phase_table_combo_box = mock.MagicMock()
        workspace_wrapper = mock.MagicMock()
        workspace_wrapper.workspace_name = 'MUSR22222_phase_table'
        self.context.phase_context.add_phase_table(workspace_wrapper)

        self.presenter.update_current_phase_tables()

        self.view.set_phase_table_combo_box.assert_called_once_with(['MUSR22222_phase_table'])

    def test_handle_calculation_started_and_handle_calculation_ended_called_correctly(self):
        self.presenter.handle_phase_table_calculation_started = mock.MagicMock()
        self.presenter.handle_calculation_success = mock.MagicMock()
        self.presenter.handle_calculation_error = mock.MagicMock()
        self.presenter.calculate_phase_table = mock.MagicMock()

        self.presenter.handle_calculate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.presenter.handle_phase_table_calculation_started.assert_called_once_with()
        self.presenter.handle_calculation_success.assert_called_once_with()
        self.presenter.handle_calculation_error.assert_not_called()
        self.presenter.calculate_phase_table.assert_called_once_with()

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.MuonWorkspaceWrapper')
    def test_add_fitting_info_to_ADS_does_nothing_if_output_fitting_info_is_false(self, workspace_wrapper_mock):
        self.presenter.add_fitting_info_to_ADS_if_required('MUSR22222_PhaseTable', mock.MagicMock())

        workspace_wrapper_mock.assert_not_called()

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.MuonWorkspaceWrapper')
    def test_add_fitting_info_to_ADS_adds_fitting_info_to_ADS_if_option_selected(self, workspace_wrapper_mock):
        self.view.output_fit_info_box.setCheckState(QtCore.Qt.Checked)

        self.presenter.add_fitting_info_to_ADS_if_required('MUSR22222_PhaseTable',
                                                           'MUSR22222_PhaseTable; fit_information')

        workspace_wrapper_mock.assert_called_once_with('MUSR22222_PhaseTable; fit_information')
        workspace_wrapper_mock.return_value.show.assert_called_once_with()

    def test_that_disable_observer_calls_on_view_when_triggered(self):
        self.view.setEnabled(True)
        self.view.enable_widget()

        for widget in self.view.children():
            if str(widget.objectName()) in ['cancel_calculate_phase_table_button']:
                continue
            self.assertTrue(widget.isEnabled())

        disable_notifier = GenericObservable()
        disable_notifier.add_subscriber(self.presenter.disable_tab_observer)

        disable_notifier.notify_subscribers()
        for widget in self.view.children():
            if str(widget.objectName()) in ['cancel_calculate_phase_table_button']:
                continue
            self.assertFalse(widget.isEnabled())

    def test_that_enable_observer_calls_on_view_when_triggered(self):
        self.view.setEnabled(True)
        self.view.disable_widget()

        for widget in self.view.children():
            if str(widget.objectName()) in ['cancel_calculate_phase_table_button']:
                continue
            self.assertFalse(widget.isEnabled())

        enable_notifier = GenericObservable()
        enable_notifier.add_subscriber(self.presenter.enable_tab_observer)

        enable_notifier.notify_subscribers()
        for widget in self.view.children():
            if str(widget.objectName()) in ['cancel_calculate_phase_table_button']:
                continue
            self.assertTrue(widget.isEnabled())

    def test_handle_add_phasequad_button_no_table(self):
        self.presenter.handle_add_phasequad_button_clicked()
        self.view.warning_popup.assert_called_once_with('Please generate a phase table first.')

    def test_handle_add_phasequad_button_no_name(self):
        self.view.set_phase_table_combo_box(["Table"])
        self.view.enter_pair_name = mock.Mock(return_value = None)
        self.presenter.create_phase_quad_calculation_thread = mock.MagicMock()

        self.presenter.handle_add_phasequad_button_clicked()
        self.assertEqual(self.presenter.create_phase_quad_calculation_thread.call_count, 0)

    def test_handle_add_phasequad_button(self):
        self.view.set_phase_table_combo_box(["Table"])
        self.presenter.validate_pair_name = mock.Mock(return_value=True)
        self.view.enter_pair_name = mock.Mock(return_value = "test")
        self.presenter.create_phase_quad_calculation_thread = mock.MagicMock()

        self.presenter.handle_add_phasequad_button_clicked()
        self.assertEqual(self.presenter.create_phase_quad_calculation_thread.call_count, 1)

    def test_phasequad_success(self):
        self.context.group_pair_context.add_pair_to_selected_pairs = mock.Mock()
        self.presenter.selected_phasequad_changed_notifier.notify_subscribers = mock.Mock()
        self.presenter._phasequad_obj = MuonPhasequad("test", "table")
        self.presenter.handle_phasequad_calculation_success()

        self.context.group_pair_context.add_pair_to_selected_pairs.assert_any_call("test_Re_")
        self.context.group_pair_context.add_pair_to_selected_pairs.assert_any_call("test_Im_")
        self.presenter.selected_phasequad_changed_notifier.notify_subscribers.assert_any_call({"is_added":True, "name":"test_Re_"})
        self.presenter.selected_phasequad_changed_notifier.notify_subscribers.assert_any_call({"is_added":True, "name":"test_Im_"})

    def test_handle_first_good_data_too_small(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.view.first_good_time = 0.0
        self.context.first_good_data = mock.Mock(return_value=0.102)
        self.presenter.handle_first_good_data_changed()

        self.view.warning_popup.assert_called_once_with('First Good Data cannot be smaller than 0.102')

    def test_handle_first_good_data_too_big(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.view.first_good_time = 40.0
        self.view.last_good_time = 41.0
        self.context.last_good_data = mock.Mock(return_value=32.29)
        self.presenter.handle_first_good_data_changed()

        self.view.warning_popup.assert_called_once_with('First Good Data cannot be greater than 32.29')

    def test_handle_last_good_data_too_small(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.view.first_good_time = -1.0
        self.view.last_good_time = 0.0
        self.context.first_good_data = mock.Mock(return_value=0.102)
        self.presenter.handle_last_good_data_changed()

        self.view.warning_popup.assert_called_once_with('Last Good Data cannot be smaller than 0.102')

    def test_handle_last_good_data_too_big(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.view.last_good_time = 41.0
        self.context.last_good_data = mock.Mock(return_value=32.29)
        self.presenter.handle_last_good_data_changed()

        self.view.warning_popup.assert_called_once_with('Last Good Data cannot be greater than 32.29')

    def test_handle_first_good_greater_than_last_good(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.view.first_good_time = 20.0
        self.view.last_good_time = 10.0
        self.presenter.handle_first_good_data_changed()

        self.view.warning_popup.assert_called_once_with('First Good Data cannot be greater than Last Good Data')

    def test_handle_last_good_less_than_first_good(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.view.first_good_time = 20.0
        self.view.last_good_time = 10.0
        self.presenter.handle_last_good_data_changed()

        self.view.warning_popup.assert_called_once_with('First Good Data cannot be greater than Last Good Data')

    def test_handle_first_good_and_last_good_pass_validation(self):
        self.view.input_workspace_combo_box.currentText = mock.Mock(return_value="MUSR62260_raw_data MA")
        self.context.data_context.instrument = 'MUSR'
        self.context.first_good_data = mock.Mock(return_value=0.102)
        self.context.last_good_data = mock.Mock(return_value=32.29)
        self.view.first_good_time = 10.0
        self.view.last_good_time = 20.0
        self.presenter.handle_first_good_data_changed()
        self.presenter.handle_last_good_data_changed()

        self.view.warning_popup.assert_not_called()

    def test_remove_phase_quad_button(self):
        phasequad = MuonPhasequad("test", "table")
        self.view.num_rows = mock.Mock(return_value=1)
        self.view.get_table_item_text = mock.Mock(return_value="test")
        self.presenter.context.group_pair_context._phasequad = [phasequad]
        self.presenter.handle_remove_phasequad_button_clicked()

        self.assertEqual(0, len(self.presenter.context.group_pair_context.phasequads))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
