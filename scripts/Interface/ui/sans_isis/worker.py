# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from PyQt4.QtCore import QRunnable, pyqtSlot, pyqtSignal, QObject
import traceback
import sys
# Following https://martinfitzpatrick.name/article/multithreading-pyqt-applications-with-qthreadpool/


class WorkerSignals(QObject):
    finished = pyqtSignal()
    error = pyqtSignal(tuple)


class Worker(QRunnable):
    """
    Worker thread which allows for async execution
    """

    def __init__(self, func, *args, **kwargs):
        super(Worker, self).__init__()
        self.func = func
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()

        self.result = None

    @pyqtSlot()
    def run(self):
        """
        Async runner
        """
        try:
            self.result = self.func(*self.args, **self.kwargs)
        except:  # noqa
            traceback.print_exc()
            exception_type, value = sys.exc_info()[:2]
            self.signals.error.emit((exception_type, value, traceback.format_exc()))
        finally:
            self.signals.finished.emit()
