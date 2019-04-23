# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.phase_table_widget.phase_table_presenter import PhaseTablePresenter
from Muon.GUI.Common.phase_table_widget.phase_table_view import PhaseTableView
from Muon.GUI.Common.muon_group import MuonGroup
from mantid.py3compat import mock
from Muon.GUI.Common.contexts.context_setup import setup_context
from Muon.GUI.Common import mock_widget
from qtpy import QtCore


class PhaseTablePresenterTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            self._qapp.processEvents()

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.view = PhaseTableView()
        self.context = setup_context()
        self.context.data_context.instrument = 'MUSR'

        self.presenter = PhaseTablePresenter(self.view, self.context)

        forward_group = MuonGroup(group_name="fwd", detector_ids=[1,3,5,7,9])
        backward_group = MuonGroup(group_name="bwd", detector_ids=[2,4,6,8,10])

        self.context.group_pair_context.add_group(forward_group)
        self.context.group_pair_context.add_group(backward_group)
        self.presenter.update_current_groups_list()

        self.view.warning_popup = mock.MagicMock()

    def test_update_view_from_model_updates_view_to_have_correct_values(self):
        self.presenter.update_view_from_model()

        for key, item in self.context.phase_context.options_dict.items():
            self.assertEquals(getattr(self.view, key), item)

    def test_update_model_from_view_updates_model_to_have_correct_values_if_view_changed(self):
        self.view.set_input_combo_box(['new_workspace_name'])
        self.view.input_workspace = 'new_workspace_name'

        self.presenter.update_model_from_view()

        self.assertEquals(self.context.phase_context.options_dict['input_workspace'], 'new_workspace_name')

    def test_create_parameters_for_cal_muon_phase_returns_correct_parameter_dict(self):
        self.context.phase_context.options_dict['input_workspace'] = 'input_workspace_name'

        result = self.presenter.create_parameters_for_cal_muon_phase_algorithm()

        self.assertEquals(result, {'BackwardSpectra': [2, 4, 6, 8, 10], 'FirstGoodData': 0.1, 'ForwardSpectra': [1, 3, 5, 7, 9],
                                   'InputWorkspace': 'input_workspace_name', 'LastGoodData': 15,
                                   'DetectorTable': 'input_workspace_name_phase_table'})

    def test_correctly_retrieves_workspace_names_associsated_to_current_runs(self):
        self.view.set_input_combo_box = mock.MagicMock()
        self.context.getGroupedWorkspaceNames = mock.MagicMock(return_value=['MUSR22222', 'MUSR44444'])

        self.presenter.update_current_run_list()

        self.view.set_input_combo_box.assert_called_once_with(['MUSR22222', 'MUSR44444'])

    def test_correctly_retrieves_names_of_current_groups(self):
        self.view.set_group_combo_boxes = mock.MagicMock()

        self.presenter.update_current_groups_list()

        self.view.set_group_combo_boxes.assert_called_once_with(['fwd', 'bwd'])

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.AnalysisDataService')
    def test_that_phase_table_added_to_ADS_with_correct_name_and_group(self, mock_data_service):
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        parameters = self.presenter.create_parameters_for_cal_muon_phase_algorithm()
        mock_phase_table = mock.MagicMock()

        self.presenter.add_phase_table_to_ADS('MUSR22222_period_1_phase_table', 'MUSR22222 PhaseTable', mock_phase_table)

        mock_data_service.addOrReplace.assert_called_once_with(parameters['DetectorTable'], mock_phase_table)
        mock_data_service.addToGroup.assert_called_once_with('MUSR22222 PhaseTable', parameters['DetectorTable'])

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_CalMuonDetectorPhases')
    def test_handle_calculate_phase_table_clicked_behaves_correctly_for_succesful_calculation(self, run_algorith_mock):
        detector_table_mock = mock.MagicMock()
        self.view.set_input_combo_box(['MUSR22222_raw_data_period_1'])
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        self.presenter.update_view_from_model()
        run_algorith_mock.return_value = (detector_table_mock, mock.MagicMock())
        self.presenter.add_phase_table_to_ADS = mock.MagicMock()
        self.presenter.calculate_base_name_and_group = mock.MagicMock(
            return_value=('MUSR22222_raw_data_period_1', 'MUSR22222 PhaseTable'))

        self.presenter.handle_calulate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.presenter.add_phase_table_to_ADS.assert_called_once_with('MUSR22222_raw_data_period_1', 'MUSR22222 PhaseTable', detector_table_mock)
        self.assertTrue(self.view.isEnabled())

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_CalMuonDetectorPhases')
    def test_handle_calculate_phase_table_clicked_behaves_correctly_for_error_calculation(self, run_algorith_mock):
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        self.presenter.update_view_from_model()
        run_algorith_mock.side_effect = RuntimeError('CalMuonDetectorPhases has failed')
        self.presenter.add_phase_table_to_ADS = mock.MagicMock()
        self.presenter.calculate_base_name_and_group = mock.MagicMock(return_value=('MUSR22222_raw_data_period_1', 'MUSR22222 PhaseTable'))

        self.presenter.handle_calulate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.assertTrue(self.view.isEnabled())
        self.view.warning_popup.assert_called_once_with('CalMuonDetectorPhases has failed')

    def test_get_parameters_for_phase_quad(self):
        self.view.set_input_combo_box(['MUSR22222_raw_data_period_1'])
        self.view.set_phase_table_combo_box(['MUSR22222_period_1_phase_table'])

        self.presenter.update_model_from_view()

        parameters = self.presenter.get_parameters_for_phase_quad()

        self.assertEquals(parameters, {'InputWorkspace': 'MUSR22222_raw_data_period_1', 'PhaseTable': 'MUSR22222_period_1_phase_table'})

    def test_that_new_phase_table_calculated_if_construct_selected(self):
        self.view.set_input_combo_box(['MUSR22222_raw_data_period_1'])
        self.view.set_phase_table_combo_box(['Construct', 'MUSR22222_period_1_phase_table'])
        self.presenter.calculate_phase_table = mock.MagicMock(return_value='created_phase_table')

        self.presenter.update_model_from_view()

        parameters = self.presenter.get_parameters_for_phase_quad()

        self.assertEquals(parameters, {'InputWorkspace': 'MUSR22222_raw_data_period_1',
                                       'PhaseTable': 'created_phase_table'})

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.AnalysisDataService')
    def test_add_phase_quad_to_ADS_does_so_in_correct_location_with_correct_name(self, mock_data_service):
        phase_quad = mock.MagicMock()

        self.presenter.add_phase_quad_to_ADS('MUSR22222_PhaseQuad_phase_table_MUSR22222', 'MUSR22222 PhaseTable',
                                             phase_quad)

        mock_data_service.addOrReplace.assert_called_once_with('MUSR22222_PhaseQuad_phase_table_MUSR22222', phase_quad)
        mock_data_service.addToGroup.assert_called_once_with('MUSR22222 PhaseTable', 'MUSR22222_PhaseQuad_phase_table_MUSR22222')

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_PhaseQuad')
    def test_handle_calcuate_phase_quad_behaves_correctly_for_succesful_calculation(self, run_algorithm_mock):
        phase_quad_mock = mock.MagicMock()
        run_algorithm_mock.return_value = phase_quad_mock
        self.presenter.add_phase_quad_to_ADS = mock.MagicMock()
        self.presenter.calculate_base_name_and_group = mock.MagicMock(return_value=('MUSR22222_period_1_phase_table',
                                                                                    'MUSR22222 PhaseTable'))
        self.view.set_input_combo_box(['MUSR22222_raw_data_period_1'])
        self.view.set_phase_table_combo_box(['MUSR22222_period_1_phase_table'])
        self.presenter.update_model_from_view()

        self.presenter.handle_calculate_phase_quad_button_clicked()
        self.wait_for_thread(self.presenter.phasequad_calculation_thread)

        self.assertTrue(self.view.isEnabled())
        run_algorithm_mock.assert_called_once_with({'PhaseTable': 'MUSR22222_period_1_phase_table',
                                                    'InputWorkspace': 'MUSR22222_raw_data_period_1'})
        self.presenter.add_phase_quad_to_ADS.assert_called_once_with('MUSR22222_period_1_phase_table',
                                                                     'MUSR22222 PhaseTable',
                                                                      phase_quad_mock)

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_PhaseQuad')
    def test_handle_phase_quad_calculation_behaves_correctly_on_error(self, run_algorithm_mock):
        run_algorithm_mock.side_effect = RuntimeError('PhaseQuad algorithm returned error')
        self.view.set_phase_table_combo_box(['MUSR22222_period_1_phase_table'])

        self.presenter.handle_calculate_phase_quad_button_clicked()
        self.wait_for_thread(self.presenter.phasequad_calculation_thread)

        self.assertTrue(self.view.isEnabled())
        self.view.warning_popup.assert_called_once_with('PhaseQuad algorithm returned error')

    def test_update_current_phase_table_list_retrieves_all_correct_tables(self):
        self.view.set_phase_table_combo_box = mock.MagicMock()
        self.context.phase_context.add_phase_table('MUSR22222_phase_table')

        self.presenter.update_current_phase_tables()

        self.view.set_phase_table_combo_box.assert_called_once_with(['MUSR22222_phase_table', 'Construct'])

    def test_handle_calculation_started_and_handle_calculation_ended_called_correctly(self):
        self.presenter.handle_calculation_started = mock.MagicMock()
        self.presenter.handle_calculation_success = mock.MagicMock()
        self.presenter.handle_calculation_error = mock.MagicMock()
        self.presenter.calculate_phase_table = mock.MagicMock()

        self.presenter.handle_calulate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.presenter.handle_calculation_started.assert_called_once_with()
        self.presenter.handle_calculation_success.assert_called_once_with()
        self.presenter.handle_calculation_error.assert_not_called()
        self.presenter.calculate_phase_table.assert_called_once_with()

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.AnalysisDataService')
    def test_add_fitting_info_to_ADS_does_nothing_if_output_fitting_info_is_false(self, mock_data_service):
        self.presenter.add_fitting_info_to_ADS_if_required('MUSR22222_PhaseTable', 'MUSR22222 PhaseTable', mock.MagicMock())

        mock_data_service.addOrReplace.assert_not_called()
        mock_data_service.addToGroup.assert_not_called()

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.AnalysisDataService')
    def test_add_fitting_info_to_ADS_adds_fitting_info_to_ADS_if_option_selected(self, mock_data_service):
        self.view.output_fit_info_box.setCheckState(QtCore.Qt.Checked)
        fit_information = mock.MagicMock()

        self.presenter.add_fitting_info_to_ADS_if_required('MUSR22222_PhaseTable', 'MUSR22222 PhaseTable',
                                                           fit_information)

        mock_data_service.addOrReplace.assert_called_once_with('MUSR22222_PhaseTable_fit_information', fit_information)
        mock_data_service.addToGroup.assert_called_once_with('MUSR22222 PhaseTable', 'MUSR22222_PhaseTable_fit_information')

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)