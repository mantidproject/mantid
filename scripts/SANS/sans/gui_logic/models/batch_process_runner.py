from PyQt4.QtCore import pyqtSlot, QThreadPool, pyqtSignal, QObject
from sans.sans_batch import SANSBatchReduction
from ui.sans_isis.worker import Worker


class BatchProcessRunner(QObject):
    row_processed_signal = pyqtSignal(int)
    row_failed_signal = pyqtSignal(int, str)

    def __init__(self, notify_progress, notify_done, notify_error):
        super(BatchProcessRunner, self).__init__()
        self.row_processed_signal.connect(notify_progress)
        self.row_failed_signal.connect(notify_error)
        self.notify_done = notify_done
        self.batch_processor = SANSBatchReduction()
        self._worker = None

    @pyqtSlot()
    def on_finished(self):
        result = self._worker.result if self._worker else None
        self._worker = None
        self.notify_done(result)

    @pyqtSlot()
    def on_error(self, error):
        self._worker = None

    def process_states(self, states, use_optimizations, output_mode, plot_results, output_graph):
        self._worker = Worker(self._process_states_on_thread, states, use_optimizations, output_mode, plot_results,
                              output_graph)
        self._worker.signals.finished.connect(self.on_finished)
        self._worker.signals.error.connect(self.on_error)

        QThreadPool.globalInstance().start(self._worker)

    def _process_states_on_thread(self, states, use_optimizations, output_mode, plot_results, output_graph):
        for key, state in states.items():
            try:
                self.batch_processor([state], use_optimizations, output_mode, plot_results, output_graph)
                self.row_processed_signal.emit(key)
            except Exception as e:
                self.row_failed_signal.emit(key, str(e))
