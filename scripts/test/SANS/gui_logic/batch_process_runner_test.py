# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import QThreadPool
import unittest

from mantid.py3compat import mock
from sans.common.enums import (OutputMode)
from sans.gui_logic.models.batch_process_runner import BatchProcessRunner


class BatchProcessRunnerTest(unittest.TestCase):
    def setUp(self):
        self.notify_progress = mock.MagicMock()
        self.notify_done = mock.MagicMock()
        self.notify_error = mock.MagicMock()

        self.sans_batch_instance = mock.MagicMock()
        self.sans_batch_instance.return_value = (mock.MagicMock(), mock.MagicMock())
        batch_patcher = mock.patch('sans.gui_logic.models.batch_process_runner.SANSBatchReduction')
        self.addCleanup(batch_patcher.stop)
        self.batch_mock = batch_patcher.start()
        self.batch_mock.return_value = self.sans_batch_instance

        load_patcher = mock.patch('sans.gui_logic.models.batch_process_runner.load_workspaces_from_states')
        self.addCleanup(load_patcher.stop)
        self.load_mock = load_patcher.start()

        self.batch_process_runner = BatchProcessRunner(self.notify_progress, self.notify_done, self.notify_error)
        self.states = {0: mock.MagicMock(), 1: mock.MagicMock(), 2: mock.MagicMock()}

    def test_that_notify_done_method_set_correctly(self):
        self.batch_process_runner.notify_done()

        self.notify_done.assert_called_once_with()

    def test_that_process_states_calls_batch_reduce_for_each_row(self):
        self.batch_process_runner.process_states(self.states, False, OutputMode.Both, False, '')
        QThreadPool.globalInstance().waitForDone()

        self.assertEqual(self.sans_batch_instance.call_count, 3)

    def test_that_process_states_emits_row_processed_signal_after_each_row(self):
        self.batch_process_runner.row_processed_signal = mock.MagicMock()
        self.batch_process_runner.row_failed_signal = mock.MagicMock()
        self.batch_process_runner.process_states(self.states, False, OutputMode.Both, False, '')
        QThreadPool.globalInstance().waitForDone()

        self.assertEqual(self.batch_process_runner.row_processed_signal.emit.call_count, 3)
        self.batch_process_runner.row_processed_signal.emit.assert_any_call(0, [], [])
        self.batch_process_runner.row_processed_signal.emit.assert_any_call(1, [], [])
        self.batch_process_runner.row_processed_signal.emit.assert_any_call(2, [], [])
        self.assertEqual(self.batch_process_runner.row_failed_signal.emit.call_count, 0)

    def test_that_process_states_emits_row_failed_signal_after_each_failed_row(self):
        self.batch_process_runner.row_processed_signal = mock.MagicMock()
        self.batch_process_runner.row_failed_signal = mock.MagicMock()
        self.sans_batch_instance.side_effect = Exception('failure')

        self.batch_process_runner.process_states(self.states, False, OutputMode.Both, False, '')
        QThreadPool.globalInstance().waitForDone()

        self.assertEqual(self.batch_process_runner.row_failed_signal.emit.call_count, 3)
        self.batch_process_runner.row_failed_signal.emit.assert_any_call(0, 'failure')
        self.batch_process_runner.row_failed_signal.emit.assert_any_call(1, 'failure')
        self.batch_process_runner.row_failed_signal.emit.assert_any_call(2, 'failure')
        self.assertEqual(self.batch_process_runner.row_processed_signal.emit.call_count, 0)

    def test_that_load_workspaces_emits_row_processed_signal_after_each_row(self):
        self.batch_process_runner.row_processed_signal = mock.MagicMock()
        self.batch_process_runner.row_failed_signal = mock.MagicMock()

        self.batch_process_runner.load_workspaces(self.states)
        QThreadPool.globalInstance().waitForDone()

        self.assertEqual(self.batch_process_runner.row_processed_signal.emit.call_count, 3)
        self.batch_process_runner.row_processed_signal.emit.assert_any_call(0, [], [])
        self.batch_process_runner.row_processed_signal.emit.assert_any_call(1, [], [])
        self.batch_process_runner.row_processed_signal.emit.assert_any_call(2, [], [])
        self.assertEqual(self.batch_process_runner.row_failed_signal.emit.call_count, 0)

    def test_that_load_workspaces_emits_row_failed_signal_after_each_failed_row(self):
        self.batch_process_runner.row_processed_signal = mock.MagicMock()
        self.batch_process_runner.row_failed_signal = mock.MagicMock()
        self.load_mock.side_effect = Exception('failure')

        self.batch_process_runner.load_workspaces(self.states)
        QThreadPool.globalInstance().waitForDone()

        self.assertEqual(self.batch_process_runner.row_failed_signal.emit.call_count, 3)
        self.batch_process_runner.row_failed_signal.emit.assert_any_call(0, 'failure')
        self.batch_process_runner.row_failed_signal.emit.assert_any_call(1, 'failure')
        self.batch_process_runner.row_failed_signal.emit.assert_any_call(2, 'failure')
        self.assertEqual(self.batch_process_runner.row_processed_signal.emit.call_count, 0)



if __name__ == '__main__':
    unittest.main()