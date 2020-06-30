# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal, QThreadPool


class DrillAlgorithmPoolSignals(QObject):
    """
    Signals that the observer could send.
    """
    taskStarted = Signal(int)     # task reference
    taskSuccess = Signal(int)     # task reference
    taskError = Signal(int, str)  # task reference
    progressUpdate = Signal(int)  # progress in percent
    processingDone = Signal()


class DrillAlgorithmPool(QThreadPool):
    """
    Class that defines an observer for the algorithms started through the DrILL
    interface.
    """
    def __init__(self):
        super(DrillAlgorithmPool, self).__init__()
        self.signals = DrillAlgorithmPoolSignals()
        self.tasks = list()
        self.tasksDone = 0
        self._running = False
        # for now, processing can not be shared amond different threads
        self.setMaxThreadCount(1)

    def addProcess(self, task):
        """
        Add a task to the thread pool.

        Args:
            task (DrillTask): the task
        """
        self._running = True
        task.signals.started.connect(
                lambda ref : self.signals.taskStarted.emit(ref)
                )
        task.signals.finished.connect(self.onTaskFinished)
        task.signals.progress.connect(self.onProgress)
        self.tasks.append(task)
        self.start(task)

    def abortProcessing(self):
        """
        Abort the processing. This function stops the currently running
        process(es) and remove the pending one(s) from the queue.
        """
        self._running = False
        self.clear()
        for t in self.tasks:
            t.cancel()
        self.tasks.clear()
        self.tasksDone = 0
        self.signals.processingDone.emit()

    def onTaskFinished(self, ref, ret, msg):
        """
        Called each time a task in the pool is ending.

        Args:
            ref (int): task reference
            ret (int): return code. (0 for success)
            msf (str): error msg if needed
        """
        if ret:
            self.signals.taskError.emit(ref, msg)
        else:
            self.signals.taskSuccess.emit(ref)

        if self._running:
            self.tasksDone += 1
            if (self.tasksDone == len(self.tasks)):
                self._running = False
                self.clear()
                self.tasks.clear()
                self.tasksDone = 0
                self.signals.processingDone.emit()

    def onProgress(self, ref, p):
        """
        Called each time a task in the pool reports on its progress.

        Args:
            ref (int): task reference
            p (float): progress between 0.0 and 1.0
        """
        progress = int((self.tasksDone + p) * 100 / len(self.tasks))
        self.signals.progressUpdate.emit(progress)

