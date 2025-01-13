# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import call

from unittest import mock
from sans.common.enums import OutputMode, RowState
from sans.gui_logic.models.async_workers.sans_run_tab_async import SansRunTabAsync


class SansRunTabAsyncTest(unittest.TestCase):
    def setUp(self):
        self.notify_progress = mock.MagicMock()
        self.notify_done = mock.MagicMock()
        self.notify_error = mock.MagicMock()
        self._mock_rows = [(mock.Mock(), i) for i in range(3)]

        self.async_worker = SansRunTabAsync(self.notify_progress, self.notify_done, self.notify_error)
        worker = self.async_worker
        worker.set_unit_test_mode(True)

        # Mock out various expensive methods
        worker._notify_progress_signal = mock.create_autospec(worker._notify_progress_signal, instance=True)
        worker.batch_processor = mock.create_autospec(worker.batch_processor, instance=True)

    def test_that_notify_done_method_set_correctly(self):
        self.async_worker.success_cb_slot(mock.NonCallableMock())
        self.notify_done.assert_called_once_with()

    def test_that_process_states_calls_batch_reduce_for_specified_row(self):
        get_states_mock = mock.MagicMock()
        states = {0: mock.MagicMock()}
        errors = {}
        get_states_mock.return_value = states, errors

        expected_shift_scale_factors = (1.1, 2.2)
        self.async_worker.batch_processor.return_value = expected_shift_scale_factors

        self.async_worker.process_states_on_thread(
            row_index_pairs=self._mock_rows,
            get_states_func=get_states_mock,
            use_optimizations=False,
            output_mode=OutputMode.BOTH,
            plot_results=False,
            output_graph="",
        )

        for row, _ in self._mock_rows:
            self.assertEqual(RowState.PROCESSED, row.state)
            self.assertIsNone(row.tool_tip)

        self.assertEqual(self.async_worker.batch_processor.call_count, 3)
        expected_emit_calls = [call(i, [], []) for i in range(len(self._mock_rows))]

        self.async_worker._notify_progress_signal.signal.emit.assert_has_calls(expected_emit_calls, any_order=True)

    def test_that_process_states_emits_row_failed_information(self):
        self.async_worker.batch_processor.side_effect = Exception("failure")

        get_states_mock = mock.MagicMock()
        states = {0: mock.MagicMock()}
        errors = {}
        get_states_mock.return_value = states, errors

        self.async_worker.process_states_on_thread(
            row_index_pairs=self._mock_rows,
            get_states_func=get_states_mock,
            use_optimizations=False,
            output_mode=OutputMode.BOTH,
            plot_results=False,
            output_graph="",
        )

        for row, _ in self._mock_rows:
            self.assertEqual(RowState.ERROR, row.state)
            self.assertEqual("failure", row.tool_tip)

    def test_that_process_states_emits_row_failed_information_when_get_states_returns_error(self):
        get_states_mock = mock.MagicMock()
        states = {}
        errors = {row[0]: "error message" for row in self._mock_rows}
        get_states_mock.return_value = states, errors

        self.async_worker.process_states_on_thread(
            row_index_pairs=self._mock_rows,
            get_states_func=get_states_mock,
            use_optimizations=False,
            output_mode=OutputMode.BOTH,
            plot_results=False,
            output_graph="",
        )

        for row, _ in self._mock_rows:
            self.assertEqual(RowState.ERROR, row.state)
            self.assertEqual("error message", row.tool_tip)

    def test_that_process_states_emits_row_failed_information_when_get_states_throws(self):
        get_states_mock = mock.MagicMock()
        get_states_mock.side_effect = Exception("failure")

        self.async_worker.process_states_on_thread(
            row_index_pairs=self._mock_rows,
            get_states_func=get_states_mock,
            use_optimizations=False,
            output_mode=OutputMode.BOTH,
            plot_results=False,
            output_graph="",
        )

        for row, _ in self._mock_rows:
            self.assertEqual(RowState.ERROR, row.state)
            self.assertEqual("failure", row.tool_tip)

    @mock.patch("sans.gui_logic.models.async_workers.sans_run_tab_async.load_workspaces_from_states")
    def test_that_load_workspaces_sets_row_to_processed(self, mocked_loader):
        states = {0: mock.MagicMock()}
        errors = {}
        get_states_mock = mock.MagicMock()
        get_states_mock.return_value = states, errors

        self.async_worker.load_workspaces_on_thread(row_index_pairs=self._mock_rows, get_states_func=get_states_mock)

        self.assertEqual(len(self._mock_rows), mocked_loader.call_count)
        for row, _ in self._mock_rows:
            self.assertEqual(RowState.PROCESSED, row.state)
            self.assertIsNone(row.tool_tip)

    @mock.patch("sans.gui_logic.models.async_workers.sans_run_tab_async.load_workspaces_from_states")
    def test_that_load_workspaces_sets_rows_to_error(self, mocked_loader):
        mocked_loader.side_effect = Exception("failure")

        states = {0: mock.MagicMock()}
        errors = {}
        get_states_mock = mock.MagicMock()
        get_states_mock.return_value = states, errors

        self.async_worker.load_workspaces_on_thread(row_index_pairs=self._mock_rows, get_states_func=get_states_mock)

        self.assertEqual(len(self._mock_rows), mocked_loader.call_count)
        for row, _ in self._mock_rows:
            self.assertEqual(RowState.ERROR, row.state)
            self.assertEqual("failure", row.tool_tip)

    def test_success_cb_triggers_notify_done(self):
        self.async_worker.success_cb_slot(mock.NonCallableMock())
        self.notify_done.assert_called_once()

    def test_error_cb_triggers_with_stack_trace(self):
        expected = mock.NonCallableMagicMock()
        self.async_worker.error_cb_slot(expected)
        self.notify_error.assert_called_once_with(str(expected))


if __name__ == "__main__":
    unittest.main()
