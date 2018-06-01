
from __future__ import (absolute_import, division, print_function)

import unittest
import sys

from mantid.kernel import config
from mantid.kernel import PropertyManagerDataService

from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from sans.common.enums import (SANSFacility, ReductionDimensionality, SaveType, ISISReductionMode,
                               RangeStepType, FitType)
from sans.test_helper.user_file_test_helper import (create_user_file, sample_user_file, sample_user_file_gravity_OFF)
from sans.test_helper.mock_objects import (create_mock_view)
from sans.test_helper.common import (remove_file)
from sans.common.enums import BatchReductionEntry


if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

BATCH_FILE_TEST_CONTENT_1 = [{BatchReductionEntry.SampleScatter: 1, BatchReductionEntry.SampleTransmission: 2,
                              BatchReductionEntry.SampleDirect: 3, BatchReductionEntry.Output: 'test_file',
                              BatchReductionEntry.UserFile: 'user_test_file'},
                             {BatchReductionEntry.SampleScatter: 1, BatchReductionEntry.CanScatter: 2,
                              BatchReductionEntry.Output: 'test_file2'}]

BATCH_FILE_TEST_CONTENT_2 = [{BatchReductionEntry.SampleScatter: 'SANS2D00022024',
                              BatchReductionEntry.SampleTransmission: 'SANS2D00022048',
                              BatchReductionEntry.SampleDirect: 'SANS2D00022048',
                              BatchReductionEntry.Output: 'test_file', BatchReductionEntry.UserFile: 'user_test_file'},
                             {BatchReductionEntry.SampleScatter: 'SANS2D00022024', BatchReductionEntry.Output: 'test_file2'}]

BATCH_FILE_TEST_CONTENT_3 = [{BatchReductionEntry.SampleScatter: 'SANS2D00022024',
                              BatchReductionEntry.SampleScatterPeriod: '3',
                              BatchReductionEntry.Output: 'test_file'}]


class MultiPeriodMock(object):
    def __init__(self, call_pattern):
        self._counter = 0
        self._call_pattern = call_pattern

    def __call__(self):
        if self._counter < len(self._call_pattern):
            return_value = self._call_pattern[self._counter]
            self._counter += 1
            return return_value
        raise RuntimeError("Issue with multi-period mocking.")


class RunTabPresenterTest(unittest.TestCase):
    def setUp(self):
        config.setFacility("ISIS")
        config.setString("default.instrument", "SANS2D")
        patcher = mock.patch('sans.gui_logic.presenter.run_tab_presenter.BatchCsvParser')
        self.addCleanup(patcher.stop)
        self.BatchCsvParserMock = patcher.start()

        self.os_patcher = mock.patch('sans.gui_logic.presenter.run_tab_presenter.os')
        self.addCleanup(self.os_patcher.stop)
        self.osMock = self.os_patcher.start()

    def test_that_will_load_user_file(self):
        # Setup presenter and mock view
        user_file_path = create_user_file(sample_user_file)
        view, settings_diagnostic_tab, _ = create_mock_view(user_file_path)
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(view)

        # Act
        presenter.on_user_file_load()

        # Assert
        # Note that the event slices are not set in the user file
        self.assertFalse(view.event_slices)
        self.assertTrue(view.reduction_dimensionality is ReductionDimensionality.OneDim)
        self.assertTrue(view.save_types[0] is SaveType.NXcanSAS)
        self.assertTrue(view.zero_error_free)
        self.assertTrue(view.use_optimizations)
        self.assertTrue(view.reduction_mode is ISISReductionMode.LAB)
        self.assertTrue(view.merge_scale == 1.)
        self.assertTrue(view.merge_shift == 0.)
        self.assertFalse(view.merge_scale_fit)
        self.assertFalse(view.merge_shift_fit)
        self.assertTrue(view.event_binning == "7000.0,500.0,60000.0")
        self.assertTrue(view.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(view.wavelength_min == 1.5)
        self.assertTrue(view.wavelength_max == 12.5)
        self.assertTrue(view.wavelength_step == 0.125)
        self.assertTrue(view.absolute_scale == 0.074)
        self.assertTrue(view.z_offset == 53.)
        self.assertTrue(view.normalization_incident_monitor == 1)
        self.assertTrue(view.normalization_interpolate)
        self.assertTrue(view.transmission_incident_monitor == 1)
        self.assertTrue(view.transmission_interpolate)
        self.assertTrue(view.transmission_roi_files == "test2.xml")
        self.assertTrue(view.transmission_mask_files == "test4.xml")
        self.assertTrue(view.transmission_radius == 7.)
        self.assertTrue(view.transmission_monitor == 4)
        self.assertTrue(view.transmission_mn_shift == -70)
        self.assertTrue(view.transmission_sample_use_fit)
        self.assertTrue(view.transmission_sample_fit_type is FitType.Logarithmic)
        self.assertTrue(view.transmission_sample_polynomial_order == 2)
        self.assertTrue(view.transmission_sample_wavelength_min == 1.5)
        self.assertTrue(view.transmission_sample_wavelength_max == 12.5)
        self.assertTrue(view.transmission_sample_use_wavelength)
        self.assertFalse(view.pixel_adjustment_det_1)
        self.assertFalse(view.pixel_adjustment_det_2)
        self.assertFalse(view.wavelength_adjustment_det_1)
        self.assertFalse(view.wavelength_adjustment_det_2)
        self.assertTrue(view.q_1d_min_or_rebin_string == "0.001,0.001,0.0126,-0.08,0.2")
        self.assertTrue(view.q_xy_max == 0.05)
        self.assertTrue(view.q_xy_step == 0.001)
        self.assertTrue(view.q_xy_step_type == RangeStepType.Lin)
        self.assertTrue(view.gravity_on_off)
        self.assertTrue(view.use_q_resolution)
        self.assertTrue(view.q_resolution_sample_a == 14.)
        self.assertTrue(view.q_resolution_source_a == 13.)
        self.assertTrue(view.q_resolution_delta_r == 11.)
        self.assertTrue(view.q_resolution_collimation_length == 12.)
        self.assertTrue(view.q_resolution_moderator_file == "moderator_rkh_file.txt")
        self.assertFalse(view.phi_limit_use_mirror)
        self.assertTrue(view.radius_limit_min == 12.)
        self.assertTrue(view.radius_limit_min == 12.)
        self.assertTrue(view.radius_limit_max == 15.)
        self.assertFalse(view.compatibility_mode)
        self.assertTrue(view.show_transmission)

        # Assert that Beam Centre View is updated correctly
        self.assertEqual(view.beam_centre.lab_pos_1, 155.45)
        self.assertEqual(view.beam_centre.lab_pos_2, -169.6)
        self.assertEqual(view.beam_centre.hab_pos_1, 155.45)
        self.assertEqual(view.beam_centre.hab_pos_2, -169.6)

        # Assert certain function calls
        self.assertTrue(view.get_user_file_path.call_count == 3)
        self.assertTrue(view.get_batch_file_path.call_count == 2)  # called twice for the sub presenter updates (masking table and settings diagnostic tab)  # noqa
        self.assertEqual(view.get_cell.call_count, 68)

        self.assertTrue(view.get_number_of_rows.call_count == 6)

        # clean up
        remove_file(user_file_path)

    def test_fails_silently_when_user_file_does_not_exist(self):
        self.os_patcher.stop()
        view, _, _ = create_mock_view("non_existent_user_file")

        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(view)

        try:
            presenter.on_user_file_load()
            has_raised = False
        except:  # noqa
            has_raised = True
        self.assertFalse(has_raised)
        self.os_patcher.start()

    def do_test_that_loads_batch_file_and_places_it_into_table(self, use_multi_period):
        # Arrange
        batch_file_path, user_file_path, presenter, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2,
                                                                                              is_multi_period=use_multi_period)  # noqa

        # Act
        presenter.on_batch_file_load()

        # Assert
        self.assertTrue(view.add_row.call_count == 2)
        if use_multi_period:
            expected_first_row = "SampleScatter:SANS2D00022024,ssp:,SampleTrans:SANS2D00022048,stp:,SampleDirect:SANS2D00022048,sdp:," \
                                 "CanScatter:,csp:,CanTrans:,ctp:,CanDirect:,cdp:,OutputName:test_file,Sample Thickness:1.0"
            expected_second_row = "SampleScatter:SANS2D00022024,ssp:,SampleTrans:,stp:,SampleDirect:,sdp:," \
                                  "CanScatter:,csp:,CanTrans:,ctp:,CanDirect:,cdp:,OutputName:test_file2,Sample Thickness:1.0"
        else:
            expected_first_row = "SampleScatter:SANS2D00022024,SampleTrans:SANS2D00022048,SampleDirect:SANS2D00022048," \
                                 "CanScatter:,CanTrans:,CanDirect:,OutputName:test_file,Sample Thickness:1.0"
            expected_second_row = "SampleScatter:SANS2D00022024,SampleTrans:,SampleDirect:," \
                                  "CanScatter:,CanTrans:,CanDirect:,OutputName:test_file2,Sample Thickness:1.0"

        calls = [mock.call(expected_first_row), mock.call(expected_second_row)]
        view.add_row.assert_has_calls(calls)

        # Clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

    def test_that_loads_batch_file_and_places_it_into_table(self):
        self.do_test_that_loads_batch_file_and_places_it_into_table(use_multi_period=True)

    def test_that_loads_batch_file_and_places_it_into_table_when_not_multi_period_enabled(self):
        self.do_test_that_loads_batch_file_and_places_it_into_table(use_multi_period=False)

    def test_that_loads_batch_file_with_multi_period_settings(self):
        # Arrange
        batch_file_path, user_file_path, presenter, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_3,
                                                                                              is_multi_period=False)
        # The call pattern is
        # False -- we are in single mode
        # True -- we switch to multi-period mode
        # True -- Getting table model
        # True -- Getting table model
        multi_period_mock = MultiPeriodMock(call_pattern=[False, True, True, True])
        view.is_multi_period_view = mock.MagicMock(side_effect=multi_period_mock)

        # Act
        presenter.on_batch_file_load()

        # Assert
        self.assertEqual(view.add_row.call_count, 1)
        self.assertEqual(view.set_multi_period_view_mode.call_count, 1)

        expected_row = "SampleScatter:SANS2D00022024,ssp:3,SampleTrans:,stp:,SampleDirect:,sdp:," \
                       "CanScatter:,csp:,CanTrans:,ctp:,CanDirect:,cdp:,OutputName:test_file,Sample Thickness:1.0"

        calls = [mock.call(expected_row)]
        view.add_row.assert_has_calls(calls)

    def test_fails_silently_when_batch_file_does_not_exist(self):
        self.os_patcher.stop()
        presenter = RunTabPresenter(SANSFacility.ISIS)
        user_file_path = create_user_file(sample_user_file)
        view, settings_diagnostic_tab, masking_table = create_mock_view(user_file_path, "non_existent_batch_file")
        presenter.set_view(view)

        try:
            presenter.on_batch_file_load()
            has_raised = False
        except:  # noqa
            has_raised = True
        self.assertFalse(has_raised)

        # Clean up
        self._remove_files(user_file_path=user_file_path)
        self.os_patcher.start()

    def test_that_gets_states_from_view(self):
        # Arrange
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)
        presenter.on_user_file_load()
        presenter.on_batch_file_load()

        # Act
        states = presenter.get_states()

        # Assert
        self.assertTrue(len(states) == 2)
        for _, state in states.items():
            try:
                state.validate()
                has_raised = False
            except:  # noqa
                has_raised = True
            self.assertFalse(has_raised)

        # Check state 0
        state0 = states[0]
        self.assertTrue(state0.data.sample_scatter == "SANS2D00022024")
        self.assertTrue(state0.data.sample_transmission == "SANS2D00022048")
        self.assertTrue(state0.data.sample_direct == "SANS2D00022048")
        self.assertTrue(state0.data.can_scatter is None)
        self.assertTrue(state0.data.can_transmission is None)
        self.assertTrue(state0.data.can_direct is None)

        # Check state 1
        state1 = states[1]
        self.assertTrue(state1.data.sample_scatter == "SANS2D00022024")
        self.assertTrue(state1.data.sample_transmission is None)
        self.assertTrue(state1.data.sample_direct is None)
        self.assertTrue(state1.data.can_scatter is None)
        self.assertTrue(state1.data.can_transmission is None)
        self.assertTrue(state1.data.can_direct is None)

        # Check some entries
        self.assertTrue(state0.slice.start_time is None)
        self.assertTrue(state0.slice.end_time is None)

        self.assertTrue(state0.reduction.reduction_dimensionality is ReductionDimensionality.OneDim)
        self.assertEqual(state0.move.detectors['LAB'].sample_centre_pos1, 0.15544999999999998)

        # Clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

    def test_that_can_get_state_for_index_if_index_exists(self):
        # Arrange
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_user_file_load()
        presenter.on_batch_file_load()

        # Act
        state = presenter.get_state_for_row(1)

        # Assert
        self.assertTrue(state.data.sample_scatter == "SANS2D00022024")
        self.assertTrue(state.data.sample_transmission is None)
        self.assertTrue(state.data.sample_direct is None)
        self.assertTrue(state.data.can_scatter is None)
        self.assertTrue(state.data.can_transmission is None)
        self.assertTrue(state.data.can_direct is None)

        # Clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

    def test_that_can_get_states_from_row_user_file(self):
        # Arrange
        row_user_file_path = create_user_file(sample_user_file_gravity_OFF)
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2,
                                                                                           row_user_file_path = row_user_file_path)

        presenter.on_user_file_load()
        presenter.on_batch_file_load()

        # Act
        state = presenter.get_state_for_row(1)
        state0 = presenter.get_state_for_row(0)

        # Assert
        self.assertTrue(state.convert_to_q.use_gravity is False)
        self.assertTrue(state0.convert_to_q.use_gravity is True)

    def test_that_returns_none_when_index_does_not_exist(self):
        # Arrange
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)
        view, _, _ = create_mock_view(user_file_path, batch_file_path)
        presenter.set_view(view)
        presenter.on_user_file_load()
        presenter.on_batch_file_load()

        # Act
        state = presenter.get_state_for_row(3)

        # Assert
        self.assertTrue(state is None)

        # Clean up
        remove_file(batch_file_path)
        remove_file(user_file_path)

    def test_that_populates_the_property_manager_data_service_when_processing_is_called(self):
        # Arrange
        self._clear_property_manager_data_service()
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        # This is not the nicest of tests, but better to test this functionality than not
        presenter.on_user_file_load()
        presenter.on_batch_file_load()

        # Act
        presenter.on_processed_clicked()

        # Assert
        # We should have two states in the PropertyManagerDataService
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()), 2)

        # clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

        self._clear_property_manager_data_service()

    def test_that_calls_halt_process_flag_if_user_file_has_not_been_loaded_and_process_is_run(self):
        # Arrange
        self._clear_property_manager_data_service()
        batch_file_path, user_file_path, presenter, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_processed_clicked()

        # Assert
        # We should have printed an error message to the logs and called halt process flag
        self.assertTrue(view.halt_process_flag.call_count == 1)
        # clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

        self._clear_property_manager_data_service()

    def test_that_calls_halt_process_flag_if_state_are_invalid_and_process_is_run(self):
        # Arrange
        self._clear_property_manager_data_service()
        batch_file_path, user_file_path, presenter, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_batch_file_load()
        presenter.on_user_file_load()

        # Set invalid state
        presenter._state_model.event_slices = 'Hello'

        presenter.on_processed_clicked()

        # Assert
        # We should have printed an error to logs and called halt process flag
        self.assertTrue(view.halt_process_flag.call_count == 1)

        # clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

        self._clear_property_manager_data_service()

    def test_that_calls_halt_process_flag_if_states_are_not_retrievable_and_process_is_run(self):
        # Arrange
        self._clear_property_manager_data_service()
        batch_file_path, user_file_path, presenter, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_batch_file_load()
        presenter.on_user_file_load()

        presenter.get_states = mock.MagicMock(return_value='')

        presenter.on_processed_clicked()

        # Assert
        # We should have printed an error to logs and called halt process flag
        self.assertTrue(view.halt_process_flag.call_count == 1)

        # clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

        self._clear_property_manager_data_service()

    def test_that_can_add_new_masks(self):
        # Arrange
        self._clear_property_manager_data_service()
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_user_file_load()
        presenter.on_batch_file_load()

        # Act
        presenter.on_mask_file_add()

        # Assert
        state = presenter.get_state_for_row(0)
        mask_info = state.mask
        mask_files = mask_info.mask_files
        self.assertTrue(mask_files == [user_file_path])

        # clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

    def test_that_get_processing_options_returns_correct_value(self):
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_1)
        expected_result = {'UseOptimizations':'1','OutputMode':'PublishToADS','PlotResults':'1','OutputGraph':'SANS-Latest'}

        result = presenter.get_processing_options()

        self.assertEqual(expected_result, result)

    @staticmethod
    def _clear_property_manager_data_service():
        for element in PropertyManagerDataService.getObjectNames():
            if PropertyManagerDataService.doesExist(element):
                PropertyManagerDataService.remove(element)

    def _get_files_and_mock_presenter(self, content, is_multi_period=True, row_user_file_path = ""):
        batch_parser = mock.MagicMock()
        batch_parser.parse_batch_file = mock.MagicMock(return_value=content)
        self.BatchCsvParserMock.return_value = batch_parser
        batch_file_path = 'batch_file_path'
        user_file_path = create_user_file(sample_user_file)
        view, _, _ = create_mock_view(user_file_path, batch_file_path, row_user_file_path)
        # We just use the sample_user_file since it exists.
        view.get_mask_file = mock.MagicMock(return_value=user_file_path)
        view.is_multi_period_view = mock.MagicMock(return_value=is_multi_period)
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(view)
        return batch_file_path, user_file_path, presenter, view

    @staticmethod
    def _remove_files(user_file_path=None, batch_file_path=None):
        if user_file_path:
            remove_file(user_file_path)
        if batch_file_path:
            remove_file(batch_file_path)


if __name__ == '__main__':
    unittest.main()
