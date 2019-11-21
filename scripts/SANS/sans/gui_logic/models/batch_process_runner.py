# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Slot, QThreadPool, Signal, QObject
from sans.sans_batch import SANSBatchReduction
from sans.algorithm_detail.batch_execution import load_workspaces_from_states
from ui.sans_isis.worker import Worker
from sans.common.enums import ISISReductionMode


class BatchProcessRunner(QObject):
    row_processed_signal = Signal(int, list, list)
    row_failed_signal = Signal(int, str)

    def __init__(self, notify_progress, notify_done, notify_error):
        super(BatchProcessRunner, self).__init__()
        self.row_processed_signal.connect(notify_progress)
        self.row_failed_signal.connect(notify_error)
        self.notify_done = notify_done
        self.batch_processor = SANSBatchReduction()
        self._worker = None

    @Slot()
    def on_finished(self):
        result = self._worker.result if self._worker else None
        self._worker = None
        self.notify_done(result)

    @Slot()
    def on_error(self):
        self._worker = None

    def process_states(self, rows, get_states_func, get_thickness_for_rows_func, use_optimizations, output_mode, plot_results, output_graph,
                       save_can=False):
        self._worker = Worker(self._process_states_on_thread,
                              get_thickness_for_rows_func=get_thickness_for_rows_func,
                              rows=rows, get_states_func=get_states_func, use_optimizations=use_optimizations,
                              output_mode=output_mode, plot_results=plot_results,
                              output_graph=output_graph, save_can=save_can)
        self._worker.signals.finished.connect(self.on_finished)
        self._worker.signals.error.connect(self.on_error)

        QThreadPool.globalInstance().start(self._worker)

    def load_workspaces(self, selected_rows, get_states_func, get_thickness_for_rows_func):

        self._worker = Worker(self._load_workspaces_on_thread, selected_rows,
                              get_states_func, get_thickness_for_rows_func)
        self._worker.signals.finished.connect(self.on_finished)
        self._worker.signals.error.connect(self.on_error)

        QThreadPool.globalInstance().start(self._worker)

    def _process_states_on_thread(self, rows, get_states_func, get_thickness_for_rows_func, use_optimizations, output_mode, plot_results,
                                  output_graph, save_can=False):
        get_thickness_for_rows_func()
        # The above must finish before we can call get states
        states, errors = get_states_func(row_index=rows)

        for row, error in errors.items():
            self.row_failed_signal.emit(row, error)

        for key, state in states.items():
            try:
                out_scale_factors, out_shift_factors = \
                    self.batch_processor([state], use_optimizations, output_mode, plot_results, output_graph, save_can)
                if state.reduction.reduction_mode == ISISReductionMode.Merged:
                    out_shift_factors = out_shift_factors[0]
                    out_scale_factors = out_scale_factors[0]
                else:
                    out_shift_factors = []
                    out_scale_factors = []
                self.row_processed_signal.emit(key, out_shift_factors, out_scale_factors)

            except Exception as e:
                self.row_failed_signal.emit(key, str(e))

    def _load_workspaces_on_thread(self, selected_rows, get_states_func, get_thickness_for_rows_func):
        get_thickness_for_rows_func()
        # The above must finish before we can call get states
        states, errors = get_states_func(row_index=selected_rows)

        for row, error in errors.items():
            self.row_failed_signal.emit(row, error)

        for key, state in states.items():
            try:
                load_workspaces_from_states(state)
                self.row_processed_signal.emit(key, [], [])
            except Exception as e:
                self.row_failed_signal.emit(key, str(e))
