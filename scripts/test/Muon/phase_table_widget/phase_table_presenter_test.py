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
from Muon.GUI.Common.contexts.context_setup import setup_context_for_tests
from Muon.GUI.Common import mock_widget


class PhaseTablePresenterTest(unittest.TestCase):
    def wait_for_thread(self, thread_model):
        if thread_model:
            thread_model._thread.wait()
            self._qapp.processEvents()

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.view = PhaseTableView()
        setup_context_for_tests(self)

        self.presenter = PhaseTablePresenter(self.view, self.context)

        forward_group = MuonGroup(group_name="fwd", detector_ids=[1,3,5,7,9])
        backward_group = MuonGroup(group_name="bwd", detector_ids=[2,4,6,8,10])

        self.context.group_pair_context.add_group(forward_group)
        self.context.group_pair_context.add_group(backward_group)

        self.view.warning_popup = mock.MagicMock()

    def test_update_view_from_model_updates_view_to_have_correct_values(self):
        self.presenter.update_view_from_model()

        for key, item in self.context.phase_context.options_dict.items():
            self.assertEquals(getattr(self.view, key), item)

    def test_update_model_from_view_updates_model_to_have_correct_values_if_view_changed(self):
        self.view.input_workspace = 'new_workspace_name'

        self.presenter.update_model_from_view()

        self.assertEquals(self.context.phase_context.options_dict['input_workspace'], 'new_workspace_name')

    def test_create_parameters_for_cal_muon_phase_returns_correct_parameter_dict(self):
        self.context.phase_context.options_dict['input_workspace'] = 'input_workspace_name'

        result = self.presenter.create_parameters_for_cal_muon_phase_algorithm()

        self.assertEquals(result, {'BackwardSpectra': [8, 2, 4, 10, 6], 'FirstGoodData': 0.1, 'ForwardSpectra': [1, 3, 9, 5, 7],
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

        self.presenter.add_phase_table_to_ADS(parameters, mock_phase_table)

        mock_data_service.addOrReplace.assert_called_once_with(parameters['DetectorTable'], mock_phase_table)
        mock_data_service.addToGroup.assert_called_once_with('Muon Data/MUSR22222/', parameters['DetectorTable'])

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_CalMuonDetectorPhases')
    def test_handle_calculate_phase_table_clicked_behaves_correctly_for_succesful_calculation(self, run_algorith_mock):
        detector_table_mock = mock.MagicMock()
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        self.presenter.update_view_from_model()
        parameters = self.presenter.create_parameters_for_cal_muon_phase_algorithm()
        run_algorith_mock.return_value = detector_table_mock
        self.presenter.add_phase_table_to_ADS = mock.MagicMock()

        self.presenter.handle_calulate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.presenter.add_phase_table_to_ADS.assert_called_once_with(parameters, detector_table_mock)
        self.assertTrue(self.view.isEnabled())

    @mock.patch('Muon.GUI.Common.phase_table_widget.phase_table_presenter.run_CalMuonDetectorPhases')
    def test_handle_calculate_phase_table_clicked_behaves_correctly_for_error_calculation(self, run_algorith_mock):
        self.context.phase_context.options_dict['input_workspace'] = 'MUSR22222_raw_data_period_1'
        self.presenter.update_view_from_model()
        run_algorith_mock.side_effect = RuntimeError('CalMuonDetectorPhases has failed')
        self.presenter.add_phase_table_to_ADS = mock.MagicMock()

        self.presenter.handle_calulate_phase_table_clicked()
        self.wait_for_thread(self.presenter.calculation_thread)

        self.assertTrue(self.view.isEnabled())
        self.view.warning_popup.assert_called_once_with('CalMuonDetectorPhases has failed')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)