# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Tuple, List

from qtpy import QtCore
from PyQt5.QtCore import QObject, pyqtSignal

from mantid.kernel import Logger
from mantidqt.utils.async_qt_adaptor import qt_async_task, IQtAsync
from mantidqt.utils.asynchronous import AsyncTaskSuccess, AsyncTaskFailure
from sans_core.algorithm_detail.batch_execution import load_workspaces_from_states
from sans_core.common.enums import ReductionMode, RowState
from sans_core.common.RowEntries import RowEntries
from sans_core.sans_batch import SANSBatchReduction


class SignalNotifyProgress(QObject):
    # Since the Async task only has completed status, not incremental progress we do this manually
    signal = pyqtSignal(int, list, list)


class SansRunTabAsync(IQtAsync):
    def __init__(self, notify_progress, notify_done, notify_error):
        super().__init__()
        self.notify_done = notify_done
        self.notify_error = notify_error
        self.notify_progress = notify_progress

        self._notify_progress_signal = SignalNotifyProgress()
        self._notify_progress_signal.signal.connect(notify_progress, QtCore.Qt.QueuedConnection)

        self.batch_processor = SANSBatchReduction()
        self._logger = Logger("SANS")

    def success_cb_slot(self, result: AsyncTaskSuccess) -> None:
        self.notify_done()

    def error_cb_slot(self, result: AsyncTaskFailure) -> None:
        self.notify_error(str(result))

    @qt_async_task
    def process_states_on_thread(
        self, row_index_pairs, get_states_func, use_optimizations, output_mode, plot_results, output_graph, save_can=False
    ):
        for row, index in row_index_pairs:
            try:
                states, errors = get_states_func(row_entries=[row])
            except Exception as e:
                self._mark_row_error(row, e)
                continue

            if len(errors) > 0:
                self._mark_row_error(row, errors[row])
                continue

            assert len(states) == 1
            # XXX: Replace this when get_states_func stops returning a dict of len 1
            state = list(states.values())[0]

            try:
                out_scale_factors, out_shift_factors = self.batch_processor(
                    [state.all_states], use_optimizations, output_mode, plot_results, output_graph, save_can
                )
            except Exception as e:
                self._mark_row_error(row, e)
                continue

            self._mark_row_processed(row)

            if state.all_states.reduction.reduction_mode == ReductionMode.MERGED:
                out_shift_factors = out_shift_factors[0]
                out_scale_factors = out_scale_factors[0]
            else:
                out_shift_factors = []
                out_scale_factors = []
            self._notify_progress_signal.signal.emit(index, out_shift_factors, out_scale_factors)

    @qt_async_task
    def load_workspaces_on_thread(self, row_index_pairs: List[Tuple[RowEntries, int]], get_states_func):
        for row, index in row_index_pairs:
            try:
                states, errors = get_states_func(row_entries=[row])
            except Exception as e:
                self._mark_row_error(row, e)
                continue

            assert len(states) == 1
            # XXX: Replace this when get_states_func stops returning a dict of len 1
            state = list(states.values())[0]
            try:
                load_workspaces_from_states(state.all_states)
                self._mark_row_processed(row)
            except Exception as e:
                self._mark_row_error(row, e)
                continue

    @staticmethod
    def _mark_row_error(row: RowEntries, error: Exception):
        row.state = RowState.ERROR
        row.tool_tip = str(error)

    @staticmethod
    def _mark_row_processed(row: RowEntries):
        row.state = RowState.PROCESSED
        row.tool_tip = None
