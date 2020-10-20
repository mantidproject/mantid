# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.kernel import PropertyManagerDataService
from mantid.kernel import config
from sans.command_interface.batch_csv_parser import BatchCsvParser
from sans.common.enums import (SANSFacility, ReductionDimensionality, SaveType, RowState)
from sans.common.enums import SANSInstrument
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel
from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from sans.state.AllStates import AllStates
from sans.test_helper.common import (remove_file)
from sans.test_helper.mock_objects import (create_mock_view)
from sans.test_helper.user_file_test_helper import (create_user_file, sample_user_file)

BATCH_FILE_TEST_CONTENT_1 = [RowEntries(sample_scatter=1, sample_transmission=2,
                                        sample_direct=3, output_name='test_file',
                                        user_file='user_test_file'),
                             RowEntries(sample_scatter=1, can_scatter=2, output_name="test_file2")]

BATCH_FILE_TEST_CONTENT_2 = [RowEntries(sample_scatter='SANS2D00022024', sample_transmission='SANS2D00022048',
                                        sample_direct='SANS2D00022048', output_name="test_file"),
                             RowEntries(sample_scatter='SANS2D00022024', output_name='test_file2')]

BATCH_FILE_TEST_CONTENT_3 = [RowEntries(sample_scatter='SANS2D00022024', sample_scatter_period=3,
                                        output_name='test_file')]

BATCH_FILE_TEST_CONTENT_4 = [RowEntries(sample_scatter='SANS2D00022024', sample_transmission='SANS2D00022048',
                                        sample_direct='SANS2D00022048', output_name='test_file'),
                             RowEntries(sample_scatter='SANS2D00022024', output_name='test_file2')]

BATCH_FILE_TEST_CONTENT_5 = [RowEntries(sample_scatter='SANS2D00022024', sample_transmission='SANS2D00022048',
                                        sample_direct='SANS2D00022048', output_name='test_file',
                                        sample_thickness=5, sample_height=2, sample_width=8)]


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
        # Backup properties that the tests or GUI may change
        self._backup_facility = config["default.facility"]
        self._backup_instrument = config["default.instrument"]
        self._backup_datasearch_dirs = config["datasearch.directories"]
        self._backup_save_dir = config["defaultsave.directory"]

        config["default.facility"] = "ISIS"

        self._mock_model = mock.create_autospec(StateGuiModel, spec_set=True)
        self._mock_table = mock.create_autospec(TableModel, spec_set=True)
        self._mock_csv_parser = mock.create_autospec(BatchCsvParser, spec_set=True)
        self._mock_view = mock.Mock()

        self._mock_model.instrument = SANSInstrument.SANS2D

        # TODO, this top level presenter should not be creating sub presenters, instead we should use
        # TODO  an observer pattern and common interface to exchange messages. However for the moment
        # TODO  we will skip patching each
        self.presenter = RunTabPresenter(SANSFacility.ISIS,
                                         model=self._mock_model, table_model=self._mock_table, view=self._mock_view)

        # The beam centre presenter will run QThreads which leads to flaky tests, so mock out
        self.presenter._beam_centre_presenter = mock.Mock()
        self.presenter._masking_table_presenter = mock.Mock()
        self.presenter._workspace_diagnostic_presenter = mock.Mock()

        self.presenter._csv_parser = self._mock_csv_parser
        # Allows us to use mock objs as the method tries to directly use int/floats
        self.presenter.update_view_from_table_model = mock.Mock()

    def tearDown(self):
        config["default.facility"] = self._backup_facility
        config["default.instrument"] = self._backup_instrument
        config["datasearch.directories"] = self._backup_datasearch_dirs
        config["defaultsave.directory2"] = self._backup_save_dir

    def test_that_will_load_user_file(self):
        # Setup self.presenter.and mock view
        user_file_path = create_user_file(sample_user_file)
        view, settings_diagnostic_tab, _ = create_mock_view(user_file_path)

        self.presenter.set_view(view)

        # Mock out methods which should be called
        self.presenter._beam_centre_presenter = mock.Mock()
        self.presenter._masking_table_presenter = mock.Mock()
        self.presenter.update_view_from_model = mock.Mock()
        self.presenter._workspace_diagnostic_presenter = mock.Mock()

        with mock.patch("sans.gui_logic.presenter.run_tab_presenter.FileLoading") as mocked_loader:
            mocked_loader.load_user_file.return_value = AllStates()
            self.presenter.on_user_file_load()
            mocked_loader.load_user_file.assert_called_once_with(file_path=user_file_path, file_information=mock.ANY)

        self.presenter._beam_centre_presenter.update_centre_positions.assert_called()
        self.presenter._beam_centre_presenter.on_update_rows.assert_called()
        self.presenter._masking_table_presenter.on_update_rows.assert_called()
        self.presenter._workspace_diagnostic_presenter.on_user_file_loadassert_called()

        self.presenter.update_view_from_model.assert_called()

        # Assert
        # Note that the event slices are not set in the user file

        # clean up
        remove_file(user_file_path)

    def test_that_checks_default_user_file(self):
        # Setup self.presenter.and mock view
        view, settings_diagnostic_tab, _ = create_mock_view("")

        self.presenter.set_view(view)

        self.assertEqual(
            self.presenter._view.set_out_default_user_file.call_count, 1,
            "Expected mock to have been called once. Called {} times.".format(
                self.presenter._view.set_out_default_user_file.call_count))

        self.assertEqual(
            self.presenter._view._call_settings_listeners.call_count, 0,
            "Expected mock to not have been called. Called {} times.".format(
                self.presenter._view._call_settings_listeners.call_count))

    def test_fails_silently_when_user_file_does_not_exist(self):
        view, _, _ = create_mock_view("non_existent_user_file")
        self.presenter.set_view(view)

        self.assertIsNone(self.presenter.on_user_file_load())

    def test_that_gets_states_from_view(self):
        # Arrange
        batch_file_path, user_file_path, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_2)
        self.presenter.on_user_file_load()
        self.presenter.on_batch_file_load()

        # Act
        states, errors = self.presenter.get_states(row_entries=BATCH_FILE_TEST_CONTENT_2)

        # Assert
        self.assertEqual(len(states), 2)

        # Check state 0
        state0 = states[BATCH_FILE_TEST_CONTENT_2[0]]
        self.assertEqual(state0.all_states.data.sample_scatter, "SANS2D00022024")
        self.assertEqual(state0.all_states.data.sample_transmission, "SANS2D00022048")
        self.assertEqual(state0.all_states.data.sample_direct, "SANS2D00022048")
        self.assertEqual(state0.all_states.data.can_scatter, None)
        self.assertEqual(state0.all_states.data.can_transmission, None)
        self.assertEqual(state0.all_states.data.can_direct, None)

        # Check state 1
        state1 = states[BATCH_FILE_TEST_CONTENT_2[1]]
        self.assertEqual(state1.all_states.data.sample_scatter, "SANS2D00022024")
        self.assertEqual(state1.all_states.data.sample_transmission, None)
        self.assertEqual(state1.all_states.data.sample_direct, None)
        self.assertEqual(state1.all_states.data.can_scatter, None)
        self.assertEqual(state1.all_states.data.can_transmission, None)
        self.assertEqual(state1.all_states.data.can_direct, None)

        # Check some entries
        self.assertEqual(state0.all_states.slice.start_time, None)
        self.assertEqual(state0.all_states.slice.end_time, None)

        self.assertEqual(state0.all_states.reduction.reduction_dimensionality, ReductionDimensionality.ONE_DIM)
        self.assertEqual(state0.all_states.move.detectors['LAB'].sample_centre_pos1, 0.15544999999999998)

        # Clean up
        self._remove_files(user_file_path=user_file_path, batch_file_path=batch_file_path)

    def test_that_can_get_state_for_index_if_index_exists(self):
        state_key = mock.NonCallableMock()
        self._mock_table.get_row.return_value = state_key
        expected_states, expected_errs = {state_key: mock.NonCallableMock(spec=StateGuiModel)}, None
        self.presenter.get_states = mock.Mock(return_value=(expected_states, expected_errs))
        self.presenter.sans_logger = mock.Mock()

        state = self.presenter.get_state_for_row(0)

        self.assertEqual(expected_states[state_key].all_states, state)

    def test_get_state_for_row_returns_empty_for_empty(self):
        self.presenter.get_states = mock.Mock(return_value=({}, None))
        self.presenter.sans_logger = mock.Mock()
        state = self.presenter.get_state_for_row(0)

        self.assertIsNone(state)
        self.presenter.sans_logger.warning.assert_called_once()

    def test_switching_dimensionality_updates_model(self):
        expected_dim = mock.NonCallableMock()
        self._mock_view.reduction_dimensionality = expected_dim
        self.presenter.on_reduction_dimensionality_changed(is_1d=False)

        self.assertEqual(expected_dim, self._mock_model.reduction_dimensionality)

    def test_on_data_changed_calls_update_rows(self):
        batch_file_path, user_file_path, _ = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_1)
        self.presenter._masking_table_presenter = mock.MagicMock()
        self.presenter._table_model.subscribe_to_model_changes(self.presenter._masking_table_presenter)
        self.presenter._beam_centre_presenter = mock.MagicMock()
        self.presenter._table_model.subscribe_to_model_changes(self.presenter._beam_centre_presenter)

        updated_entries = mock.NonCallableMock()
        self.presenter.on_data_changed(0, updated_entries)

        self.presenter._masking_table_presenter.on_update_rows.assert_called_with()
        self.presenter._beam_centre_presenter.on_update_rows.assert_called_with()
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_on_save_dir_changed_calls_set_out_file_directory(self):
        batch_file_path, user_file_path, view = self._get_files_and_mock_presenter(BATCH_FILE_TEST_CONTENT_3,
                                                                                   is_multi_period=False)
        config["defaultsave.directory"] = "test/path"
        self.presenter._view.set_out_file_directory.assert_called_with("test/path")

    def test_on_insert_row_updates_table_model(self):
        self.presenter.on_row_appended()

        self._mock_table.append_table_entry.assert_called_with(mock.ANY)
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_that_all_columns_shown_when_multi_period_is_true(self):

        self.presenter.set_view(mock.MagicMock())

        self.presenter.on_multiperiod_changed(True)

        self.presenter._view.show_period_columns.assert_called_once_with()

    def test_that_period_columns_hidden_when_multi_period_is_false(self):

        self.presenter.set_view(mock.MagicMock())

        self.presenter.on_multiperiod_changed(False)

        self.presenter._view.hide_period_columns.assert_called_once_with()

    def test_on_data_changed_updates_table_model(self):
        self.presenter._beam_centre_presenter = mock.Mock()
        self.presenter._masking_table_presenter = mock.Mock()

        expected_new_entry = mock.NonCallableMock()
        self._mock_table.get_all_rows.return_value = [expected_new_entry]

        self.presenter.on_data_changed(0, expected_new_entry)
        self._mock_table.replace_table_entry.assert_called_with(0, expected_new_entry)

        self.presenter._beam_centre_presenter.on_update_rows.assert_called_with()
        self.presenter._masking_table_presenter.on_update_rows.assert_called_with()
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_on_row_removed_removes_correct_row(self):
        rows = [0]
        self.presenter.on_rows_removed(rows)

        self._mock_table.remove_table_entries.assert_called_with(rows)
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_setup_instrument_specific_settings(self):

        self.presenter.set_view(mock.MagicMock())
        self.presenter._beam_centre_presenter = mock.MagicMock()
        self.presenter._workspace_diagnostic_presenter = mock.MagicMock()
        instrument = SANSInstrument.LOQ

        self.presenter._setup_instrument_specific_settings(SANSInstrument.LOQ)

        self.presenter._view.set_instrument_settings.called_once_with(instrument)
        self.presenter._beam_centre_presenter.on_update_instrument.called_once_with(instrument)
        self.presenter._workspace_diagnostic_presenter.called_once_with(instrument)

    def test_setup_instrument_specific_settings_sets_facility_in_config(self):
        config.setFacility('TEST_LIVE')

        self.presenter.set_view(mock.MagicMock())
        self.presenter._beam_centre_presenter = mock.MagicMock()
        self.presenter._workspace_diagnostic_presenter = mock.MagicMock()
        instrument = SANSInstrument.LOQ

        self.presenter._setup_instrument_specific_settings(instrument)
        self.assertEqual(config.getFacility().name(), 'ISIS')

    def test_setup_instrument_specific_settings_sets_instrument_in_config(self):
        config['default.instrument'] = 'ALF'

        self.presenter.set_view(mock.MagicMock())
        self.presenter._beam_centre_presenter = mock.MagicMock()
        self.presenter._workspace_diagnostic_presenter = mock.MagicMock()
        instrument = SANSInstrument.LOQ

        self.presenter._setup_instrument_specific_settings(instrument)
        self.assertEqual(config['default.instrument'], 'LOQ')

    def test_on_copy_rows_requested_adds_correct_rows_to_clipboard(self):
        self._mock_view.get_selected_rows.return_value = [0]
        expected_clipboard = mock.NonCallableMock()
        self._mock_table.get_row.return_value = expected_clipboard

        self.presenter.on_copy_rows_requested()

        self.assertTrue(expected_clipboard in self.presenter._clipboard)

    def test_on_paste_rows_requested_appends_new_row_if_no_row_selected(self):
        self._mock_view.get_selected_rows.return_value = []
        pos = 101
        self._mock_table.get_number_of_rows.return_value = pos

        expected = mock.NonCallableMock()
        self.presenter._clipboard = [expected]
        self.presenter.update_view_from_table_model = mock.MagicMock()

        self.presenter.on_paste_rows_requested()

        self.presenter.update_view_from_table_model.assert_called_with()
        self._mock_table.replace_table_entries.assert_called_with([pos], [expected])

    def test_on_paste_rows_requested_replaces_row_if_one_row_is_selected(self):
        pos = [1]
        self._mock_view.get_selected_rows.return_value = pos

        test_row_0 = mock.NonCallableMock()
        self.presenter._clipboard = [test_row_0]
        self.presenter.update_view_from_table_model = mock.MagicMock()

        self.presenter.on_paste_rows_requested()

        self.presenter.update_view_from_table_model.assert_called_with()
        self._mock_table.replace_table_entries.assert_called_with(pos, [test_row_0])

    def test_on_paste_rows_requested_replaces_first_row_and_removes_rest_if_multiple_rows_selected(self):
        expected_pos = [0, 2]
        self._mock_view.get_selected_rows.return_value = expected_pos

        expected = mock.NonCallableMock()
        self.presenter._clipboard = [expected]
        self.presenter.update_view_from_table_model = mock.MagicMock()

        self.presenter.on_paste_rows_requested()

        self.presenter.update_view_from_table_model.assert_called_with()
        self._mock_table.replace_table_entries.assert_called_with(expected_pos, [expected])

    def test_on_insert_row_adds_row_to_table_model_after_selected_row(self):
        self._mock_view.get_selected_rows.return_value = [100]

        self.presenter.on_insert_row()

        self._mock_table.insert_row_at(101, mock.ANY)
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_on_insert_row_updates_view(self):
        self._mock_view.get_selected_rows.return_value = [0]

        self.presenter.on_insert_row()
        self.presenter.update_view_from_table_model.assert_called_once_with()

    def test_on_erase_rows_clears_specific_rows_from_table_model(self):
        selected_rows = [1, 2]
        self._mock_table.get_number_of_rows.return_value = 3
        self._mock_view.get_selected_rows.return_value = selected_rows

        self.presenter.on_erase_rows()
        self.assertEqual(len(selected_rows), self._mock_table.replace_table_entries.call_count)
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_on_erase_rows_clears_table(self):
        selected_rows = [0, 1, 2]
        self._mock_table.get_number_of_rows.return_value = 3
        self._mock_view.get_selected_rows.return_value = selected_rows

        self.presenter.on_erase_rows()

        self._mock_table.clear_table_entries.assert_called_with()

        self._mock_view.get_selected_rows.return_value = []
        self._mock_table.clear_table_entries.reset_mock()

        self.presenter.on_erase_rows()
        self._mock_table.clear_table_entries.assert_called_with()
        self.presenter.update_view_from_table_model.assert_called_with()

    def test_on_cut_rows_requested_updates_clipboard(self):
        self._mock_view.get_selected_rows.return_value = [0]
        expected_clipboard = mock.NonCallableMock()
        self._mock_table.get_row.return_value = expected_clipboard

        self.presenter.on_cut_rows_requested()

        self.assertTrue(expected_clipboard in self.presenter._clipboard)

    def test_on_cut_rows_requested_removes_selected_rows(self):
        selected_rows = [0]
        self._mock_view.get_selected_rows.return_value = selected_rows

        expected = "expected_clipboard_val"
        self._mock_table.get_row.return_value = expected

        self.presenter.on_cut_rows_requested()

        self._mock_table.remove_table_entries.assert_called_with(selected_rows)
        self.assertTrue(expected in self.presenter._clipboard)

    def test_notify_progress_increments_progress(self):
        self.presenter.notify_progress(0, [0.0], [1.0])

        self.assertEqual(self.presenter.progress, 1)
        self.assertEqual(self.presenter._view.progress_bar_value, 1)

    def test_that_notify_progress_updates_state_and_tooltip_of_row_for_scale_and_shift(self):
        mocked_row = mock.Mock()

        self._mock_table.get_row.return_value = mocked_row

        self.presenter.notify_progress(0, [0.0], [1.0])

        self.assertEqual(RowState.PROCESSED, mocked_row.state)
        self.assertIsNone(mocked_row.tool_tip)
        self.presenter.update_view_from_table_model.assert_called_with()

        mocked_row.options.set_developer_option.assert_called_with('MergeShift', 0.0)

        self.assertEqual(self.presenter.progress, 1)
        self.assertEqual(self.presenter._view.progress_bar_value, 1)

    def test_that_update_progress_sets_correctly(self):

        view = mock.MagicMock()
        self.presenter.set_view(view)

        self.presenter._set_progress_bar(current=100, number_steps=200)
        self.assertEqual(self.presenter.progress, 100)
        self.assertEqual(view.progress_bar_value, 100)
        self.assertEqual(view.progress_bar_maximum, 200)

    def test_that_notify_progress_updates_state_and_tooltip_of_row(self):

        view = mock.MagicMock()
        self.presenter.set_view(view)
        self.presenter.on_row_appended()

        self.presenter.notify_progress(0, [], [])

        self.assertEqual(self.presenter._table_model.get_row(0).state, RowState.PROCESSED)
        self.assertEqual(self.presenter._table_model.get_row(0).tool_tip, None)

    def test_that_process_selected_does_nothing_if_no_states_selected(self):

        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[])
        self.presenter.set_view(view)
        self.presenter._process_rows = mock.MagicMock()

        self.presenter.on_process_selected_clicked()
        self.assertEqual(
            self.presenter._process_rows.call_count, 0,
            "Expected self.presenter._process_rows to not have been called. Called {} times.".format(
                self.presenter._process_rows.call_count))

    def test_that_process_selected_only_processes_selected_rows(self):
        # Naive test. Doesn't check that we are processing the correct processed rows,
        # just that we are processing the same number of rows as we have selected.
        # This would only really fail if on_process_selected_clicked and on_process_all_clicked
        # get muddled-up

        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 2, 3])

        self.presenter.set_view(view)
        self.presenter._process_rows = mock.Mock()

        table_model = TableModel()

        for i in range(5):
            table_model.append_table_entry(RowEntries(sample_scatter='74040'))
        self.presenter._table_model = table_model

        self.presenter.on_process_selected_clicked()

        expected = [table_model.get_row(0), table_model.get_row(2), table_model.get_row(3)]

        self.presenter._process_rows.assert_called_with(expected)

    def test_that_process_selected_ignores_all_empty_rows(self):

        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 1])
        self.presenter.set_view(view)

        table_model = TableModel()
        empty_row = RowEntries()
        populated_row = RowEntries(sample_scatter='74040')

        table_model.replace_table_entry(0, empty_row)
        table_model.replace_table_entry(1, populated_row)

        self.presenter._table_model = table_model
        self.presenter._process_rows = mock.MagicMock()

        self.presenter.on_process_selected_clicked()
        self.presenter._process_rows.assert_called_with([populated_row])

    def test_that_process_all_ignores_empty_rows(self):

        view = mock.MagicMock()
        view.get_selected_rows = mock.MagicMock(return_value=[0, 1])
        self.presenter.set_view(view)

        table_model = TableModel()
        populated_row = RowEntries(sample_scatter=1)
        table_model.append_table_entry(populated_row)
        table_model.append_table_entry(RowEntries())

        self.presenter._table_model = table_model
        self.presenter._process_rows = mock.MagicMock()

        self.presenter.on_process_selected_clicked()
        self.presenter._process_rows.assert_called_with([populated_row])

    def test_that_csv_file_is_saved_when_export_table_button_is_clicked(self):
        self.presenter._export_table = mock.MagicMock()
        self._mock_table.get_non_empty_rows.return_value = BATCH_FILE_TEST_CONTENT_1
        self._mock_model.batch_file = ""
        self.presenter.display_save_file_box = mock.Mock(return_value="mocked_file_path")
        self.presenter.on_export_table_clicked()
        self.assertEqual(self._mock_csv_parser.save_batch_file.call_count, 1,
                         "_save_batch_file should have been called but was not")

    def test_that_table_not_exported_if_table_is_empty(self):
        self.presenter._export_table = mock.MagicMock()
        self._mock_table.get_non_empty_rows.return_value = []

        self.presenter.on_export_table_clicked()
        self.assertEqual(self._mock_csv_parser.save_batch_file.call_count, 0,
                         "_save_batch_file should not have been called."
                         " It was called {} times.".format(self.presenter._export_table.call_count))

    def test_buttons_enabled_after_export_table_fails(self):
        self._mock_table.get_non_empty_rows.return_value = [RowEntries()]
        self._mock_model.batch_file = ""

        self.presenter.display_save_file_box = mock.Mock(return_value="mocked_file_path")

        self.presenter._view.enable_buttons = mock.Mock()
        self.presenter._view.disable_buttons = mock.Mock()

        # Mock error throw on disable buttons so export fails
        self.presenter._csv_parser = mock.Mock()
        self.presenter._csv_parser.save_batch_file = mock.Mock(side_effect=RuntimeError(""))

        with self.assertRaises(RuntimeError):
            self.presenter.on_export_table_clicked()

        self.assertEqual(self.presenter._view.enable_buttons.call_count, 1)

    def test_that_default_name_is_used_when_export_table(self):
        self.presenter._export_table = mock.MagicMock()
        self._mock_table.get_non_empty_rows.return_value = BATCH_FILE_TEST_CONTENT_1
        self._mock_model.batch_file = 'test_filename'
        self.presenter.display_save_file_box = mock.Mock(return_value="mocked_file_path")
        self.presenter.on_export_table_clicked()
        self.assertEqual(self.presenter.display_save_file_box.call_count, 1,
                         "display_save_file_box should have been called but was not")
        expected_args = ['Save table as', 'test_filename', '*.csv']
        args = [arg for arg in self.presenter.display_save_file_box.call_args[0]]
        self.assertEqual(args, expected_args)

    def test_that_canSAS_is_disabled_if_2D_reduction(self):
        """This test checks that if you are running a 2D reduction and have canSAS output mode checked,
        the GUI will automatically uncheck canSAS to avoid data dimension errors."""

        view = mock.MagicMock()
        view.can_sas_checkbox.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setChecked = mock.Mock()
        view.can_sas_checkbox.setEnabled = mock.Mock()
        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=False)

        self.presenter.set_view(view)
        self.presenter.on_reduction_dimensionality_changed(False)
        self.presenter._view.can_sas_checkbox.setEnabled.assert_called_once_with(False)

    def test_that_canSAS_is_unchecked_if_2D_reduction(self):
        """This tests that the canSAS checkbox is unchecked when switching from 1D to 2D reduction"""

        view = mock.MagicMock()
        view.can_sas_checkbox.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setChecked = mock.Mock()
        view.can_sas_checkbox.setEnabled = mock.Mock()
        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=False)

        self.presenter.set_view(view)
        self.presenter.on_reduction_dimensionality_changed(False)
        self.presenter._view.can_sas_checkbox.setChecked.assert_called_once_with(False)

    def test_that_canSAS_is_enabled_if_1D_reduction_and_not_in_memory_mode(self):
        """This test checks that if you are not in memory mode and switch to 1D reduction, then
        can sas file type is enabled."""

        view = mock.MagicMock()
        view.can_sas_checkbox.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setChecked = mock.Mock()
        view.can_sas_checkbox.setEnabled = mock.Mock()
        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=False)

        self.presenter.set_view(view)
        self.presenter.on_reduction_dimensionality_changed(True)

        self.presenter._view.can_sas_checkbox.setEnabled.assert_called_once_with(True)
        self.presenter._view.can_sas_checkbox.setChecked.assert_not_called()

    def test_that_canSAS_is_not_enabled_if_switch_to_1D_reduction_and_in_memory_mode(self):
        """This test checks that if you are in memory mode, the can sas file type is not
        re-enabled if you switch to 1D reduction."""

        view = mock.MagicMock()
        view.can_sas_checkbox.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setChecked = mock.Mock()
        view.can_sas_checkbox.setEnabled = mock.Mock()
        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=True)

        self.presenter.set_view(view)
        self.presenter.on_reduction_dimensionality_changed(True)

        self.presenter._view.can_sas_checkbox.setChecked.assert_not_called()
        self.presenter._view.can_sas_checkbox.setEnabled.assert_not_called()

    def test_that_updating_default_save_directory_also_updates_add_runs_save_directory(self):
        """This test checks that add runs self.presenter.s save directory update method is called
        when the defaultsave directory is updated."""

        view = mock.MagicMock()
        self.presenter.set_view(view)

        self.presenter._handle_output_directory_changed("a_new_directory")
        self.presenter._view.add_runs_presenter.handle_new_save_directory.assert_called_once_with("a_new_directory")

    def test_that_validate_output_modes_raises_if_no_file_types_selected_for_file_mode(self):

        view = mock.MagicMock()

        view.save_types = [SaveType.NO_TYPE]

        view.output_mode_memory_radio_button.isChecked = mock.MagicMock(return_value=False)
        view.output_mode_file_radio_button.isChecked = mock.MagicMock(return_value=True)
        view.output_mode_both_radio_button.isChecked = mock.MagicMock(return_value=False)
        self.presenter.set_view(view)

        self.assertRaises(RuntimeError, self.presenter._validate_output_modes)

    def test_that_validate_output_modes_raises_if_no_file_types_selected_for_both_mode(self):

        view = mock.MagicMock()

        view.save_types = [SaveType.NO_TYPE]

        view.output_mode_memory_radio_button.isChecked = mock.MagicMock(return_value=False)
        view.output_mode_file_radio_button.isChecked = mock.MagicMock(return_value=False)
        view.output_mode_both_radio_button.isChecked = mock.MagicMock(return_value=True)
        self.presenter.set_view(view)

        self.assertRaises(RuntimeError, self.presenter._validate_output_modes)

    def test_that_validate_output_modes_does_not_raise_if_no_file_types_selected_for_memory_mode(self):

        view = mock.MagicMock()
        view.save_types = [SaveType.NO_TYPE]

        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=True)
        view.output_mode_file_radio_button.isChecked = mock.Mock(return_value=False)
        view.output_mode_both_radio_button.isChecked = mock.Mock(return_value=False)
        self.presenter.set_view(view)

        try:
            self.presenter._validate_output_modes()
        except RuntimeError:
            self.fail("Did not expect _validate_output_modes to fail when no file types are selected "
                      "for memory output mode.")

    def test_that_switching_to_memory_mode_disables_all_file_type_buttons(self):
        """This tests that all file type buttons are disabled when memory mode is selected."""

        view = mock.MagicMock()

        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=True)
        view.disable_file_type_buttons = mock.Mock()

        self.presenter.set_view(view)

        self.presenter.on_output_mode_changed()
        self.presenter._view.disable_file_type_buttons.assert_called_once_with()

    def test_that_all_file_type_buttons_are_enabled_if_switching_to_non_memory_mode_and_in_1D_reduction_mode(self):
        """This tests that all file type buttons are enabled if switching to file or both mode, when reduction
        dimensionality is 1D"""

        view = mock.MagicMock()

        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=False)
        view.reduction_dimensionality_1D.isChecked = mock.Mock(return_value=True)
        view.can_sas_checkbox.setEnabled = mock.Mock()
        view.nx_can_sas_checkbox.setEnabled = mock.Mock()
        view.rkh_checkbox.setEnabled = mock.Mock()

        self.presenter.set_view(view)

        self.presenter.on_output_mode_changed()
        self.presenter._view.can_sas_checkbox.setEnabled.assert_called_once_with(True)
        self.presenter._view.nx_can_sas_checkbox.setEnabled.assert_called_once_with(True)
        self.presenter._view.rkh_checkbox.setEnabled.assert_called_once_with(True)

    def test_that_rkh_and_nx_can_sas_are_enabled_if_switching_to_non_memory_mode_and_in_2D_reduction_mode(self):
        """This tests that nx_can_sas and rkh file type buttons are enabled if switching to file or both mode, when
         reduction dimensionality is 1D, but can sas is not enabled"""

        view = mock.MagicMock()

        view.output_mode_memory_radio_button.isChecked = mock.Mock(return_value=False)
        view.reduction_dimensionality_1D.isChecked = mock.Mock(return_value=False)
        view.can_sas_checkbox.setEnabled = mock.Mock()
        view.nx_can_sas_checkbox.setEnabled = mock.Mock()
        view.rkh_checkbox.setEnabled = mock.Mock()

        self.presenter.set_view(view)

        self.presenter.on_output_mode_changed()
        self.presenter._view.can_sas_checkbox.setEnabled.assert_not_called()
        self.presenter._view.nx_can_sas_checkbox.setEnabled.assert_called_once_with(True)
        self.presenter._view.rkh_checkbox.setEnabled.assert_called_once_with(True)

    def test_that_on_reduction_mode_changed_calls_update_hab_if_selection_is_HAB(self):

        self.presenter._beam_centre_presenter = mock.MagicMock()

        self.presenter.on_reduction_mode_selection_has_changed("Hab")
        self.presenter._beam_centre_presenter.update_hab_selected.assert_called_once_with()

        self.presenter._beam_centre_presenter.reset_mock()
        self.presenter.on_reduction_mode_selection_has_changed("front")
        self.presenter._beam_centre_presenter.update_hab_selected.assert_called_once_with()

    def test_that_on_reduction_mode_changed_calls_update_lab_if_selection_is_LAB(self):

        self.presenter._beam_centre_presenter = mock.MagicMock()

        self.presenter.on_reduction_mode_selection_has_changed("rear")
        self.presenter._beam_centre_presenter.update_lab_selected.assert_called_once_with()

        self.presenter._beam_centre_presenter.reset_mock()
        self.presenter.on_reduction_mode_selection_has_changed("main-detector")
        self.presenter._beam_centre_presenter.update_lab_selected.assert_called_once_with()

        self.presenter._beam_centre_presenter.reset_mock()
        self.presenter.on_reduction_mode_selection_has_changed("DetectorBench")
        self.presenter._beam_centre_presenter.update_lab_selected.assert_called_once_with()

        self.presenter._beam_centre_presenter.reset_mock()
        self.presenter.on_reduction_mode_selection_has_changed("rear-detector")
        self.presenter._beam_centre_presenter.update_lab_selected.assert_called_once_with()

    @staticmethod
    def _clear_property_manager_data_service():
        for element in PropertyManagerDataService.getObjectNames():
            if PropertyManagerDataService.doesExist(element):
                PropertyManagerDataService.remove(element)

    def _get_files_and_mock_presenter(self, content, is_multi_period=True, row_user_file_path=""):
        if row_user_file_path:
            content[1].user_file = row_user_file_path

        batch_parser = mock.MagicMock()
        batch_parser.parse_batch_file = mock.MagicMock(return_value=content)
        self._mock_csv_parser.return_value = batch_parser
        batch_file_path = 'batch_file_path'

        user_file_path = create_user_file(sample_user_file)
        view, _, _ = create_mock_view(user_file_path, batch_file_path, row_user_file_path)
        # We just use the sample_user_file since it exists.
        view.get_mask_file = mock.MagicMock(return_value=user_file_path)
        view.is_multi_period_view = mock.MagicMock(return_value=is_multi_period)
        self.presenter.set_view(view)
        return batch_file_path, user_file_path, view

    @staticmethod
    def _remove_files(user_file_path=None, batch_file_path=None):
        if user_file_path:
            remove_file(user_file_path)
        if batch_file_path:
            remove_file(batch_file_path)


if __name__ == '__main__':
    unittest.main()
