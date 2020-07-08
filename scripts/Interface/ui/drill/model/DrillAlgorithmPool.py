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
        # list of all tasks
        self._tasks = list()
        # set of finished tasks
        self._tasksDone = set()
        # progress value of each task (between 0.0 and 1.0)
        self._progresses = dict()
        # if the threadpool is currently running
        self._running = False
        # for now, processing can not be shared amond different threads
        self.setMaxThreadCount(1)

    def addProcesses(self, tasks):
        """
        Add a list of tasks to the thread pool.

        Args:
            tasks (list(DrillTask)): list of tasks
        """
        if not tasks:
            return
        self._running = True
        self._tasks = tasks
        for task in tasks:
            self._progresses[task.ref] = 0.0
            task.signals.started.connect(self.signals.taskStarted.emit)
            task.signals.finished.connect(self.onTaskFinished)
            task.signals.progress.connect(self.onProgress)
            self.start(task)

    def abortProcessing(self):
        """
        Abort the processing. This function stops the currently running
        process(es) and remove the pending one(s) from the queue.
        """
        self._running = False
        self.clear()
        for task in self._tasks:
            task.cancel()
        self._tasks.clear()
        self._tasksDone.clear()
        self._progresses.clear()
        self.signals.processingDone.emit()

    def onTaskFinished(self, ref, ret, msg):
        """
        Called each time a task in the pool is ending.

        Args:
            ref (int): task reference
            ret (int): return code. (0 for success)
            msf (str): error msg if needed
        """
        if (ref in self._tasksDone):
            return
        self._progresses[ref] = 1.0
        if ret:
            self.signals.taskError.emit(ref, msg)
        else:
            self.signals.taskSuccess.emit(ref)

        if self._running:
            self._tasksDone.add(ref)
            if (len(self._tasksDone) == len(self._tasks)):
                self.clear()
                self._running = False
                self._tasks.clear()
                self._tasksDone.clear()
                self._progresses.clear()
                self.signals.processingDone.emit()

    def onProgress(self, ref, p):
        """
        Called each time a task in the pool reports on its progress.

        Args:
            ref (int): task reference
            p (float): progress between 0.0 and 1.0
        """
        self._progresses[ref] = p
        progress = 0.0
        for i in self._progresses:
            progress += self._progresses[i] * 100
        progress /= len(self._tasks)
        self.signals.progressUpdate.emit(progress)
