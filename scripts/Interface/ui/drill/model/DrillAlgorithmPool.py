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
    taskFinished = Signal(int)    # task reference
    taskError = Signal(int)       # task reference
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
        # for now, processing can not be shared amond different threads
        self.setMaxThreadCount(1)

    def addProcess(self, task):
        """
        Add a task to the thread pool.

        Args:
            task (DrillTask): the task
        """
        task.signals.started.connect(
                lambda ref : self.signals.taskStarted.emit(ref)
                )
        task.signals.finished.connect(
                lambda ref : self.onTaskDone(ref, 0)
                )
        task.signals.error.connect(
                lambda ref : self.onTaskDone(ref, -1)
                )
        task.signals.progress.connect(self.onProgress)
        self.tasks.append(task)
        self.start(task)

    def abortProcessing(self):
        """
        Abort the processing. This function stops the currently running
        process(es) and remove the pending one(s) from the queue.
        """
        self.clear()
        for t in self.tasks:
            t.cancel()
        self.tasks.clear()
        self.tasksDone = 0

    def onTaskDone(self, ref, ret):
        """
        Called each time a task in the pool is ending.

        Args:
            ref (int): task reference
            ret (int): return code. (0 for success)
        """
        self.tasksDone += 1
        if ret:
            self.signals.taskError.emit(ref)
        else:
            self.signals.taskFinished.emit(ref)
        if (self.tasksDone == len(self.tasks)):
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

