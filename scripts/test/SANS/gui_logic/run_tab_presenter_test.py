# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import config
from mantid.kernel import PropertyManagerDataService
from mantid.py3compat import mock

from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from sans.common.enums import (SANSFacility, ReductionDimensionality, SaveType, ISISReductionMode,
                               RangeStepType, FitType, SANSInstrument, RowState)
from sans.test_helper.user_file_test_helper import (create_user_file, sample_user_file, sample_user_file_gravity_OFF,
                                                    sample_user_file_with_instrument)
from sans.test_helper.mock_objects import (create_mock_view)
from sans.test_helper.common import (remove_file)
from sans.common.enums import BatchReductionEntry, SANSInstrument
from sans.gui_logic.models.table_model import TableModel, TableIndexModel
from sans.test_helper.file_information_mock import SANSFileInformationMock


BATCH_FILE_TEST_CONTENT_1 = [{BatchReductionEntry.SampleScatter: 1, BatchReductionEntry.SampleTransmission: 2,
                              BatchReductionEntry.SampleDirect: 3, BatchReductionEntry.Output: 'test_file',
                              BatchReductionEntry.UserFile: 'user_test_file'},
                             {BatchReductionEntry.SampleScatter: 1, BatchReductionEntry.CanScatter: 2,
                              BatchReductionEntry.Output: 'test_file2'}]

BATCH_FILE_TEST_CONTENT_2 = [{BatchReductionEntry.SampleScatter: 'SANS2D00022024',
                              BatchReductionEntry.SampleTransmission: 'SANS2D00022048',
                              BatchReductionEntry.SampleDirect: 'SANS2D00022048',
                              BatchReductionEntry.Output: 'test_file'},
                             {BatchReductionEntry.SampleScatter: 'SANS2D00022024', BatchReductionEntry.Output: 'test_file2'}]

BATCH_FILE_TEST_CONTENT_3 = [{BatchReductionEntry.SampleScatter: 'SANS2D00022024',
                              BatchReductionEntry.SampleScatterPeriod: '3',
                              BatchReductionEntry.Output: 'test_file'}]

BATCH_FILE_TEST_CONTENT_4 = [{BatchReductionEntry.SampleScatter: 'SANS2D00022024',
                              BatchReductionEntry.SampleTransmission: 'SANS2D00022048',
                              BatchReductionEntry.SampleDirect: 'SANS2D00022048',
                              BatchReductionEntry.Output: 'test_file'},
                             {BatchReductionEntry.SampleScatter: 'SANS2D00022024', BatchReductionEntry.Output: 'test_file2'}]


def get_non_empty_row_mock(value):
    return value


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

        patcher = mock.patch('sans.gui_logic.presenter.run_tab_presenter.BatchCsvParser')
        self.addCleanup(patcher.stop)
        self.BatchCsvParserMock = patcher.start()

        self.os_patcher = mock.patch('sans.gui_logic.presenter.run_tab_presenter.os')
        self.addCleanup(self.os_patcher.stop)
        self.osMock = self.os_patcher.start()

        self.thickness_patcher = mock.patch('sans.gui_logic.models.table_model.create_file_information')
        self.addCleanup(self.thickness_patcher.stop)
        self.thickness_patcher.start()

    def test_that_will_load_user_file(self):
        # Setup presenter and mock view
        user_file_path = create_user_file(sample_user_file)
        view, settings_diagnostic_tab, _ = create_mock_view(user_file_path)
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(view)

        # Act
        try:
            presenter.on_user_file_load()
        except RuntimeError:
            # Assert that RuntimeError from no instrument is caught
            self.fail("on_user_file_load raises a RuntimeError which should be caught")

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
        self.assertTrue(view.phi_limit_use_mirror)
        self.assertTrue(view.radius_limit_min == 12.)
        self.assertTrue(view.radius_limit_min == 12.)
        self.assertTrue(view.radius_limit_max == 15.)
        self.assertTrue(view.compatibility_mode)

        # Assert that Beam Centre View is updated correctly
        self.assertEqual(view.beam_centre.lab_pos_1, 155.45)
        self.assertEqual(view.beam_centre.lab_pos_2, -169.6)
        self.assertEqual(view.beam_centre.hab_pos_1, 155.45)
        self.assertEqual(view.beam_centre.hab_pos_2, -169.6)

        # clean up
        remove_file(user_file_path)

    def test_that_checks_default_user_file(self):
        # Setup presenter and mock view
        view, settings_diagnostic_tab, _ = create_mock_view("")
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(view)

        self.assertEqual(
            presenter._view.set_out_default_user_file.call_count, 1,
            "Expected mock to have been called once. Called {} times.".format(
                presenter._view.set_out_default_user_file.call_count))

        self.assertEqual(
            presenter._view._call_settings_listeners.call_count, 0,
            "Expected mock to not have been called. Called {} times.".format(
                presenter._view._call_settings_listeners.call_count))

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
        self.assertEqual(view.add_row.call_count, 8)
        if use_multi_period:
            expected_first_row = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '',
                                  '', 'test_file', '', '', '', '', '', '']
            expected_second_row = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '',
                                   '', '', '', '', '']
        else:
            expected_first_row = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '',
                                  '', 'test_file', '', '', '', '', '', '']
            expected_second_row = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', ''
                                   , '', '', '', '']

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
        self.assertEqual(view.add_row.call_count, 4)
        self.assertEqual(view.show_period_columns.call_count, 2)

        expected_row = ['SANS2D00022024', '3', '', '', '', '', '', '', '', '', '', '', 'test_file', '', '',
                        '', '', '', '']

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

    def test_batch_file_dir_not_added_to_config_if_batch_file_load_fails(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        user_file_path = create_user_file(sample_user_file)
        view, settings_diagnostic_tab, masking_table = create_mock_view(user_file_path, "A/Path/batch_file.csv")
        presenter.set_view(view)

        presenter.on_batch_file_load()
        config_dirs = config["datasearch.directories"]
        result = "A/Path/" in config_dirs

        self.assertFalse(result, "We do not expect A/Path/ to be added to config, "
                                 "datasearch.directories is now {}".format(config_dirs))

    def test_that_gets_states_from_view(self):
        # Arrange
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)
        presenter.on_user_file_load()
        presenter.on_batch_file_load()
        presenter._table_model.update_thickness_from_file_information(1, self.get_file_information_mock())
        presenter._table_model.update_thickness_from_file_information(2, self.get_file_information_mock())

        # Act
        states, errors = presenter.get_states(row_index=[0, 1])

        # Assert
        self.assertEqual(len(states), 2)
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

    def test_that_can_get_states_from_row_user_file(self):
        # Arrange
        row_user_file_path = create_user_file(sample_user_file_gravity_OFF)
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_4,
                                                                                           row_user_file_path = row_user_file_path)
        presenter.on_user_file_load()
        presenter.on_batch_file_load()
        presenter._table_model.update_thickness_from_file_information(1, self.get_file_information_mock())
        presenter._table_model.update_thickness_from_file_information(2, self.get_file_information_mock())


        # Act
        state = presenter.get_state_for_row(1)
        state0 = presenter.get_state_for_row(0)
        # Assert
        self.assertFalse(state.convert_to_q.use_gravity)
        self.assertTrue(state0.convert_to_q.use_gravity)

    def test_that_can_get_state_for_index_if_index_exists(self):
        # Arrange
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_user_file_load()
        presenter.on_batch_file_load()
        presenter._table_model.update_thickness_from_file_information(1, self.get_file_information_mock())
        presenter._table_model.update_thickness_from_file_information(2, self.get_file_information_mock())

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

    def test_that_can_add_new_masks(self):
        # Arrange
        self._clear_property_manager_data_service()
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)

        presenter.on_user_file_load()
        presenter.on_batch_file_load()
        presenter._table_model.update_thickness_from_file_information(1, self.get_file_information_mock())
        presenter._table_model.update_thickness_from_file_information(2, self.get_file_information_mock())

        # Act
        presenter.on_mask_file_add()

        # Assert
        state = presenter.get_state_for_row(0)
        mask_info = state.mask
        mask_files = mask_info.mask_files
        self.assertTrue(mask_files == [user_file_path])

        # clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

    def test_on_data_changed_calls_update_rows(self):
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_1)
        presenter._masking_table_presenter = mock.MagicMock()
        presenter._table_model.subscribe_to_model_changes(presenter._masking_table_presenter)
        presenter._beam_centre_presenter = mock.MagicMock()
        presenter._table_model.subscribe_to_model_changes(presenter._beam_centre_presenter)
        row_entry = [''] * 16
        presenter.on_row_inserted(0, row_entry)

        presenter.on_data_changed(0, 0, '12335', '00000')

        presenter._masking_table_presenter.on_update_rows.assert_called_with()
        presenter._beam_centre_presenter.on_update_rows.assert_called_with()

    def test_on_save_dir_changed_calls_set_out_file_directory(self):
        batch_file_path, user_file_path, presenter, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_3,
                                                                                              is_multi_period=False)
        config["defaultsave.directory"] = "test/path"
        presenter._view.set_out_file_directory.assert_called_with("test/path")

    def test_table_model_is_initialised_upon_presenter_creation(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        expected_table_model = TableModel()
        expected_table_model.subscribe_to_model_changes(presenter)
        expected_table_model.subscribe_to_model_changes(presenter._masking_table_presenter)
        expected_table_model.subscribe_to_model_changes(presenter._beam_centre_presenter)
        self.maxDiff = None
        self.assertEqual(presenter._table_model, expected_table_model)

    def test_on_insert_row_updates_table_model(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())
        row = ['74044', '', '74044', '', '74044', '', '74044', '', '74044', '', '74044', '', 'test_reduction'
               , 'user_file', '1.2', '']
        index = 0
        expected_table_index_model = TableIndexModel(*row)
        expected_table_index_model.id = 0
        expected_table_index_model.file_finding = True

        presenter.on_row_inserted(index, row)

        self.assertEqual(presenter._table_model.get_number_of_rows(), 1)
        model_row = presenter._table_model.get_table_entry(0)
        self.assertEqual(model_row, expected_table_index_model)

    def test_that_all_columns_shown_when_multi_period_is_true(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())

        presenter.on_multiperiod_changed(True)

        presenter._view.show_period_columns.assert_called_once_with()

    def test_that_period_columns_hidden_when_multi_period_is_false(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())

        presenter.on_multiperiod_changed(False)

        presenter._view.hide_period_columns.assert_called_once_with()

    def test_on_data_changed_updates_table_model(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())
        row = ['74044', '', '74044', '', '74044', '', '74044', '', '74044', '', '74044', '', 'test_reduction'
               , 'user_file', '1.2', '']
        expected_row = ['74044', '', '74040', '', '74044', '', '74044', '', '74044', '', '74044', '', 'test_reduction'
                        , 'user_file', '1.2', '']
        presenter._table_model.add_table_entry(0, TableIndexModel(*row))
        row = 0
        column = 2
        value = '74040'
        expected_table_index_model = TableIndexModel(*expected_row)
        expected_table_index_model.id = 0
        expected_table_index_model.file_finding = True

        presenter.on_data_changed(row, column, value, '')

        self.assertEqual(presenter._table_model.get_number_of_rows(), 1)
        model_row = presenter._table_model.get_table_entry(0)
        self.assertEqual(model_row, expected_table_index_model)

    def test_on_row_removed_removes_correct_row(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())
        row_0 = ['74040', '', '74040', '', '74040', '', '74040', '', '74040', '', '74040', '', 'test_reduction'
                 , 'user_file', '1.2', '']
        row_1 = ['74041', '', '74041', '', '74041', '', '74041', '', '74041', '', '74041', '', 'test_reduction'
                 , 'user_file', '1.2', '']
        row_2 = ['74042', '', '74042', '', '74042', '', '74042', '', '74042', '', '74042', '', 'test_reduction'
                 , 'user_file', '1.2', '']
        row_3 = ['74043', '', '74043', '', '74043', '', '74043', '', '74043', '', '74043', '', 'test_reduction'
                 , 'user_file', '1.2', '']
        presenter._table_model.add_table_entry(0, TableIndexModel(*row_0))
        presenter._table_model.add_table_entry(1, TableIndexModel(*row_1))
        presenter._table_model.add_table_entry(2, TableIndexModel(*row_2))
        presenter._table_model.add_table_entry(3, TableIndexModel(*row_3))
        rows = [0, 2]
        expected_row_0 = TableIndexModel(*row_1)
        expected_row_0.id = 1
        expected_row_0.file_finding = True
        expected_row_1 = TableIndexModel(*row_3)
        expected_row_1.id = 3
        expected_row_1.file_finding = True


        presenter.on_rows_removed(rows)

        self.assertEqual(presenter._table_model.get_number_of_rows(), 2)
        model_row_0 = presenter._table_model.get_table_entry(0)
        self.assertEqual(model_row_0, expected_row_0)
        model_row_1 = presenter._table_model.get_table_entry(1)
        self.assertEqual(model_row_1, expected_row_1)

    def test_on_rows_removed_updates_view(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())
        row_0 = ['74040', '', '74040', '', '74040', '', '74040', '', '74040', '', '74040', '', 'test_reduction'
            , 'user_file', '1.2', '']
        row_1 = ['74041', '', '74041', '', '74041', '', '74041', '', '74041', '', '74041', '', 'test_reduction'
            , 'user_file', '1.2', '']
        row_2 = ['74042', '', '74042', '', '74042', '', '74042', '', '74042', '', '74042', '', 'test_reduction'
            , 'user_file', '1.2', '']
        row_3 = ['74043', '', '74043', '', '74043', '', '74043', '', '74043', '', '74043', '', 'test_reduction'
            , 'user_file', '1.2', '']
        presenter._table_model.add_table_entry(0, TableIndexModel(*row_0))
        presenter._table_model.add_table_entry(1, TableIndexModel(*row_1))
        presenter._table_model.add_table_entry(2, TableIndexModel(*row_2))
        presenter._table_model.add_table_entry(3, TableIndexModel(*row_3))
        presenter.update_view_from_table_model = mock.MagicMock()
        rows = [0, 2]

        presenter.on_rows_removed(rows)

        presenter.update_view_from_table_model.assert_called_once_with()

    def test_add_row_to_table_model_adds_row_to_table_model(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())
        parsed_data = BATCH_FILE_TEST_CONTENT_2

        for item, row in enumerate(parsed_data):
            presenter._add_row_to_table_model(row, item)

        table_entry_0 = presenter._table_model.get_table_entry(0)
        self.assertEqual(table_entry_0.output_name, 'test_file')
        self.assertEqual(table_entry_0.sample_scatter, 'SANS2D00022024')
        self.assertEqual(table_entry_0.sample_thickness, '')

        table_entry_1 = presenter._table_model.get_table_entry(1)
        self.assertEqual(table_entry_1.output_name, 'test_file2')

    def test_update_view_from_table_model_updated_view_based_on_model(self):
        batch_file_path, user_file_path, presenter, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)
        presenter.set_view(mock.MagicMock())
        parsed_data = BATCH_FILE_TEST_CONTENT_2
        for item, row in enumerate(parsed_data):
            presenter._add_row_to_table_model(row, item)
        expected_call_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                           'test_file', '', '', '', '', '', '']
        expected_call_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '', '',
                           '', '', '']

        presenter.update_view_from_table_model()

        self.assertEqual(presenter._view.add_row.call_count, 5)

        presenter._view.add_row.assert_any_call(expected_call_0)
        presenter._view.add_row.assert_any_call(expected_call_1)

    def test_setup_instrument_specific_settings(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        presenter.set_view(mock.MagicMock())
        presenter._beam_centre_presenter = mock.MagicMock()
        presenter._workspace_diagnostic_presenter = mock.MagicMock()
        instrument = SANSInstrument.LOQ

        presenter._setup_instrument_specific_settings(SANSInstrument.LOQ)

        presenter._view.set_instrument_settings.called_once_with(instrument)
        presenter._beam_centre_presenter.on_update_instrument.called_once_with(instrument)
        presenter._workspace_diagnostic_presenter.called_once_with(instrument)

    def test_on_copy_rows_requested_adds_correct_rows_to_clipboard(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0])
        presenter.set_view(view)
        test_row = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                    'test_file', '', '1.0', '', '', '', '']

        presenter.on_row_inserted(0, test_row)

        presenter.on_copy_rows_requested()

        self.assertEqual(presenter._clipboard, [test_row])

    def test_on_paste_rows_requested_appends_new_row_if_no_row_selected(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0', '',
                      '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)
        presenter._clipboard = [test_row_0]

        presenter.on_paste_rows_requested()

        self.assertEqual(presenter._table_model.get_number_of_rows(), 3)
        self.assertEqual(presenter._table_model.get_table_entry(2).to_list(), test_row_0)

    def test_on_paste_rows_requested_replaces_row_if_one_row_is_selected(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[1])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0', '',
                      '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)
        presenter.on_row_inserted(2, test_row_0)
        presenter._clipboard = [test_row_0]

        presenter.on_paste_rows_requested()

        self.assertEqual(presenter._table_model.get_number_of_rows(), 3)
        self.assertEqual(presenter._table_model.get_table_entry(1).to_list(), test_row_0)

    def test_on_paste_rows_requested_replaces_first_row_and_removes_rest_if_multiple_rows_selected(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 2])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0', '',
                      '', '', '']
        test_row_2 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file3', '', '1.0', '',
                      '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)
        presenter.on_row_inserted(2, test_row_2)
        presenter._clipboard = [test_row_2]

        presenter.on_paste_rows_requested()

        self.assertEqual(presenter._table_model.get_number_of_rows(), 2)
        self.assertEqual(presenter._table_model.get_table_entry(0).to_list(), test_row_2)
        self.assertEqual(presenter._table_model.get_table_entry(1).to_list(), test_row_1)

    def test_on_paste_rows_updates_table_in_view(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)
        presenter._clipboard = [test_row_0]
        presenter.update_view_from_table_model = mock.MagicMock()

        presenter.on_paste_rows_requested()

        presenter.update_view_from_table_model.assert_called_with()

    def test_on_insert_row_adds_row_to_table_model_after_selected_row(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0', '',
                      '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)

        presenter.on_insert_row()

        self.assertEqual(presenter._table_model.get_number_of_rows(), 3)
        self.assertEqual(presenter._table_model.get_table_entry(1).to_list(), [''] * 19)
        self.assertEqual(presenter._table_model.get_table_entry(0).to_list(), test_row_0)
        self.assertEqual(presenter._table_model.get_table_entry(2).to_list(), test_row_1)

    def test_on_insert_row_updates_view(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0',
                      '', '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)
        presenter.update_view_from_table_model = mock.MagicMock()

        presenter.on_insert_row()

        presenter.update_view_from_table_model.assert_called_once_with()

    def test_on_erase_rows_clears_rows_from_table_model(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[1, 2])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        empty_row = TableModel.create_empty_row()

        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_0)
        presenter.on_row_inserted(2, test_row_0)

        presenter.on_erase_rows()

        self.assertEqual(presenter._table_model.get_number_of_rows(), 3)
        self.assertEqual(presenter._table_model.get_table_entry(0).to_list(), test_row_0)
        empty_row.id = 3
        self.assertEqual(presenter._table_model.get_table_entry(1).__dict__, empty_row.__dict__)
        empty_row.id = 4
        self.assertEqual(presenter._table_model.get_table_entry(2).__dict__, empty_row.__dict__)

    def test_on_erase_rows_updates_view(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[1, 2])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_0)
        presenter.on_row_inserted(2, test_row_0)
        presenter.update_view_from_table_model = mock.MagicMock()

        presenter.on_erase_rows()

        self.assertEqual(presenter._table_model._table_entries[0].to_list(),
                         ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                          'test_file', '', '1.0', '', '', '', ''])
        self.assertEqual(presenter._table_model._table_entries[1].to_list(),
                         ['', '', '', '', '', '', '', '', '', '', '', '',
                          '', '', '', '', '', '', ''])
        self.assertEqual(presenter._table_model._table_entries[2].to_list(),
                         ['', '', '', '', '', '', '', '', '', '', '', '',
                          '', '', '', '', '', '', ''])

    def test_on_cut_rows_requested_updates_clipboard(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0])
        presenter.set_view(view)
        test_row = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                    'test_file', '', '1.0', '', '', '', '']

        presenter.on_row_inserted(0, test_row)

        presenter.on_cut_rows_requested()

        self.assertEqual(presenter._clipboard, [test_row])

    def test_on_cut_rows_requested_removes_selected_rows(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0])
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '', '', '', '']
        test_row_1 = ['SANS2D00022024', '', '', '', '', '', '', '', '', '', '', '', 'test_file2', '', '1.0',
                      '', '', '', '']
        presenter.on_row_inserted(0, test_row_0)
        presenter.on_row_inserted(1, test_row_1)

        presenter.on_cut_rows_requested()

        self.assertEqual(presenter._table_model.get_number_of_rows(), 1)
        self.assertEqual(presenter._table_model.get_table_entry(0).to_list(), test_row_1)

    def test_notify_progress_increments_progress(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '']
        presenter.on_row_inserted(0, test_row_0)

        presenter.notify_progress(0, [0.0], [1.0])

        self.assertEqual(presenter.progress, 1)
        self.assertEqual(presenter._view.progress_bar_value, 1)

    def test_that_notify_progress_updates_state_and_tooltip_of_row_for_scale_and_shift(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '']
        presenter.on_row_inserted(0, test_row_0)

        presenter.notify_progress(0, [0.0], [1.0])

        self.assertEqual(presenter._table_model.get_table_entry(0).row_state, RowState.Processed)
        self.assertEqual(presenter._table_model.get_table_entry(0).options_column_model.get_options_string(),
                         'MergeScale=1.0, MergeShift=0.0')

        self.assertEqual(presenter.progress, 1)
        self.assertEqual(presenter._view.progress_bar_value, 1)

    def test_that_notify_progress_updates_state_and_tooltip_of_row(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)
        test_row_0 = ['SANS2D00022024', '', 'SANS2D00022048', '', 'SANS2D00022048', '', '', '', '', '', '', '',
                      'test_file', '', '1.0', '']
        presenter.on_row_inserted(0, test_row_0)

        presenter.notify_progress(0, [], [])

        self.assertEqual(presenter._table_model.get_table_entry(0).row_state, RowState.Processed)
        self.assertEqual(presenter._table_model.get_table_entry(0).tool_tip, '')

    def test_that_process_selected_does_nothing_if_no_states_selected(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[])
        presenter.set_view(view)
        presenter._process_rows = mock.MagicMock()

        presenter.on_process_selected_clicked()
        self.assertEqual(
            presenter._process_rows.call_count, 0,
            "Expected presenter._process_rows to not have been called. Called {} times.".format(
                presenter._process_rows.call_count))

    def test_that_process_selected_only_processes_selected_rows(self):
        # Naive test. Doesn't check that we are processing the correct processed rows,
        # just that we are processing the same number of rows as we have selected.
        # This would only really fail if on_process_selected_clicked and on_process_all_clicked 
        # get muddled-up
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 3, 4])
        
        presenter.set_view(view)
        presenter._table_model.reset_row_state = mock.MagicMock()
        presenter._table_model.get_non_empty_rows = mock.MagicMock(side_effect=get_non_empty_row_mock)

        presenter.on_process_selected_clicked()
        self.assertEqual(
            presenter._table_model.reset_row_state.call_count, 3,
            "Expected reset_row_state to have been called 3 times. Called {} times.".format(
                presenter._table_model.reset_row_state.call_count))

    def test_that_process_selected_ignores_all_empty_rows(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 1])
        presenter.set_view(view)

        table_model = TableModel()
        row_entry0 = [''] * 16
        row_entry1 = ['74040', '', '74040', '', '74040', '', '74040', '', '74040', '', '74040', '', 'test_reduction',
                      'user_file', '1.2', '']
        table_model.add_table_entry(0, TableIndexModel(*row_entry0))
        table_model.add_table_entry(1, TableIndexModel(*row_entry1))

        presenter._table_model = table_model
        presenter._process_rows = mock.MagicMock()

        presenter.on_process_selected_clicked()
        presenter._process_rows.assert_called_with([1])
        
    def test_that_process_all_ignores_selected_rows(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 3, 4])
        
        presenter._table_model.get_number_of_rows = mock.MagicMock(return_value=7)
        presenter.set_view(view)
        presenter._table_model.reset_row_state = mock.MagicMock()
        presenter._table_model.get_non_empty_rows = mock.MagicMock(side_effect=get_non_empty_row_mock)
        
        presenter.on_process_all_clicked()
        self.assertEqual(
            presenter._table_model.reset_row_state.call_count, 7,
            "Expected reset_row_state to have been called 7 times. Called {} times.".format(
                presenter._table_model.reset_row_state.call_count))

    def test_that_process_all_ignores_empty_rows(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)

        table_model = TableModel()
        row_entry0 = [''] * 16
        row_entry1 = ['74040', '', '74040', '', '74040', '', '74040', '', '74040', '', '74040', '', 'test_reduction',
                      'user_file', '1.2', '']
        table_model.add_table_entry(0, TableIndexModel(*row_entry0))
        table_model.add_table_entry(1, TableIndexModel(*row_entry1))

        presenter._table_model = table_model
        presenter._process_rows = mock.MagicMock()

        presenter.on_process_all_clicked()
        presenter._process_rows.assert_called_with([1])

    def test_that_table_not_exported_if_table_is_empty(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)

        presenter._export_table = mock.MagicMock()

        presenter.on_export_table_clicked()
        self.assertEqual(presenter._export_table.call_count, 0,
                         "_export table should not have been called."
                         " It was called {} times.".format(presenter._export_table.call_count))

    def test_row_created_for_batch_file_correctly(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)

        test_row = ["SANS2D00022025", "SANS2D00022052", "SANS2D00022022",
                    "", "", "", "another_file", "a_user_file.txt"]

        expected_list = ["sample_sans", "SANS2D00022025", "sample_trans", "SANS2D00022052",
                         "sample_direct_beam", "SANS2D00022022", "can_sans", "", "can_trans", "", "can_direct_beam", "",
                         "output_as", "another_file", "user_file", "a_user_file.txt"]

        actual_list = presenter._create_batch_entry_from_row(test_row)

        self.assertEqual(actual_list, expected_list)

    def test_buttons_enabled_after_export_table_fails(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)

        presenter.get_row_indices = mock.MagicMock(return_value=[0, 1, 2])
        presenter._view.enable_buttons = mock.MagicMock()
        # Mock error throw on disable buttons so export fails
        presenter._view.disable_buttons = mock.MagicMock(side_effect=RuntimeError("A test exception"))
        try:
            presenter.on_export_table_clicked()
        except Exception as e:
            self.fail("Exceptions should have been caught in the method. "
                      "Exception thrown is {}".format(str(e)))
        else:
            self.assertEqual(presenter._view.enable_buttons.call_count, 1,
                             "Expected enable buttons to be called once, "
                             "was called {} times.".format(presenter._view.enable_buttons.call_count))

    def test_that_verify_output_types_disables_canSAS_if_2D_reduction(self):
        """This test checks that if you are running a 2D reduction and have canSAS output mode checked,
        the GUI will automatically uncheck canSAS to avoid data dimension errors."""
        presenter = RunTabPresenter(SANSFacility.ISIS)

        view = mock.MagicMock()
        view.can_sas_checkbox.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setChecked = mock.Mock()
        view.can_sas_checkbox.setEnabled = mock.Mock()

        presenter.set_view(view)
        presenter.verify_output_modes(False)

        setchecked_calls = presenter._view.can_sas_checkbox.setChecked.call_args_list
        self.assertEqual(len(setchecked_calls), 1, "We expected canSAS setChecked to only be called once, was called "
                                                   "{} times instead.".format(len(setchecked_calls)))

        args, _ = setchecked_calls[-1]  # The last call to can_sas_checkbox.setEnabled is from _verify_output_types
        self.assertFalse(args[0], "Can SAS checkbox should have been turned off, since we were in 2D reduction mode.")

        setenabled_calls = presenter._view.can_sas_checkbox.setEnabled.call_args_list
        self.assertEqual(len(setenabled_calls), 1, "We expected canSAS setEnabled to only be called once, was called "
                                                   "{} times instead.".format(len(setenabled_calls)))

        args, _ = setenabled_calls[-1]  # The last call to can_sas_checkbox.setEnabled is from _verify_output_types
        self.assertFalse(args[0], "Can SAS checkbox should have been disabled, since we were in 2D reduction mode.")

    def test_that_verify_output_types_does_not_disable_canSAS_if_1D_reduction(self):
        """This test checks that you can still run a 1D reduction with canSAS output."""
        presenter = RunTabPresenter(SANSFacility.ISIS)

        view = mock.MagicMock()
        view.can_sas_checkbox.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setChecked = mock.Mock()
        view.can_sas_checkbox.setEnabled = mock.Mock()

        presenter.set_view(view)
        presenter.verify_output_modes(True)

        self.assertEqual(presenter._view.can_sas_checkbox.setChecked.call_count, 0,
                         "Did not expect can_sas_checkbox.setChecked to be called. "
                         "It was called {} times".format(presenter._view.can_sas_checkbox.setChecked.call_count))

        args, _ = presenter._view.can_sas_checkbox.setEnabled.call_args_list[-1]
        self.assertTrue(args[0], "Can SAS checkbox should have been enabled, since we switched to 1D reduction mode.")

    def test_that_updating_default_save_directory_also_updates_add_runs_save_directory(self):
        """This test checks that add runs presenter's save directory update method is called
        when the defaultsave directory is updated."""
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = mock.MagicMock()
        presenter.set_view(view)

        presenter._handle_output_directory_changed("a_new_directory")
        calls = presenter._view.add_runs_presenter.handle_new_save_directory.call_args_list
        self.assertEqual(len(calls), 1)

        args = calls[0][0]
        self.assertEqual(args, ("a_new_directory",))

    @staticmethod
    def _clear_property_manager_data_service():
        for element in PropertyManagerDataService.getObjectNames():
            if PropertyManagerDataService.doesExist(element):
                PropertyManagerDataService.remove(element)

    def _get_files_and_mock_presenter(self, content, is_multi_period=True, row_user_file_path = ""):
        if row_user_file_path:
            content[1].update({BatchReductionEntry.UserFile : row_user_file_path})

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

    @staticmethod
    def get_file_information_mock():
        return SANSFileInformationMock(instrument=SANSInstrument.SANS2D)


if __name__ == '__main__':
    unittest.main()
