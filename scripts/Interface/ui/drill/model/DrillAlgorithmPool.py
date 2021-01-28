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
        self._tasks = set()
        # set of finished tasks
        self._tasksDone = 0
        # progress value of each task (between 0.0 and 1.0)
        self._progresses = dict()
        # if the threadpool is currently running
        self._running = False
        # to limit the number of threads
        # self.setMaxThreadCount(1)

    def addProcesses(self, tasks):
        """
        Add a list of tasks to the thread pool.

        Args:
            tasks (list(DrillTask)): list of tasks
        """
        if not tasks:
            self.signals.processingDone.emit()
            return
        self._running = True
        for task in tasks:
            self._tasks.add(task)
            self._progresses[task] = 0.0
            task.signals.started.connect(self.onTaskStarted)
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
        for task in [task for task in self._tasks]:
            task.cancel()
        self._tasks.clear()
        self._tasksDone = 0
        self._progresses.clear()
        self.signals.processingDone.emit()

    def onTaskStarted(self, task):
        """
        Called when a task is started.

        Args:
            task (DrillTask): the task
        """
        self.signals.taskStarted.emit(task.ref)

    def onTaskFinished(self, task, ret, msg):
        """
        Called each time a task in the pool is ending.

        Args:
            task (DrillTask): the task
            ret (int): return code. (0 for success)
            msf (str): error msg if needed
        """
        if task in self._tasks:
            self._tasks.remove(task)
            if task in self._progresses:
                del self._progresses[task]
        else:
            return

        self._tasksDone += 1
        if ret:
            self.signals.taskError.emit(task.ref, msg)
        else:
            self.signals.taskSuccess.emit(task.ref)

        if self._running:
            if not self._tasks:
                self._tasksDone = 0
                self.clear()
                self._running = False
                self._progresses.clear()
                self.signals.processingDone.emit()

    def onProgress(self, task, p):
        """
        Called each time a task in the pool reports on its progress.

        Args:
            task (DrillTask): the task
            p (float): progress between 0.0 and 1.0
        """
        self._progresses[task] = p
        progress = 0.0
        for i in self._progresses:
            progress += self._progresses[i] * 100
        progress += 100 * self._tasksDone
        progress /= len(self._tasks) + self._tasksDone
        self.signals.progressUpdate.emit(progress)
