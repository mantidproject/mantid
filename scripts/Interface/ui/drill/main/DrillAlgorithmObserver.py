# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import time

from qtpy.QtCore import QObject, Signal, QThreadPool

from mantid.api import AlgorithmObserver
import mantid.simpleapi as sapi

class DrillAlgorithmObserverSignals(QObject):
    """
    Signals that the observer could send.
    """
    taskProcessing = Signal(int)  # task reference
    taskFinished = Signal(int)    # task reference
    taskError = Signal(int)       # task reference
    progressUpdate = Signal(int)  # progress in percent
    processingDone = Signal()


class DrillAlgorithmObserver(AlgorithmObserver):
    """
    Class that defines an observer for the algorithms started through the DrILL
    interface.
    """
    def __init__(self):
        super(DrillAlgorithmObserver, self).__init__()
        self.signals = DrillAlgorithmObserverSignals()
        self.processes = dict()
        self.threadPool = QThreadPool()
        # for now, processing can not be shared amond different threads
        self.threadPool.setMaxThreadCount(1)
        self.tasksNumber = 0
        self.tasksFinished = 0

    def taskStarted(self, ref):
        """
        Called when the task is actually started.

        Args:
            ref (int): task reference
        """
        # to be sure that the thread had enough time to start
        time.sleep(0.1)
        p = sapi.AlgorithmManager.runningInstancesOf(self.processes[ref])
        if p:
            self.processes[ref] = p[0]
            self.observeProgress(p[0])
            self.signals.taskProcessing.emit(ref)

    def taskFinished(self, ref, ret):
        """
        Called when a running task is finished.

        Args:
            ref (int): task reference.
            ret (int): return value of the process. 0 if success.
        """
        del self.processes[ref]
        self.tasksFinished += 1
        if ret:
            self.signals.taskError.emit(ref)
        else:
            self.signals.taskFinished.emit(ref)
        if not self.processes:
            self.signals.processingDone.emit()

    def addProcess(self, task):
        """
        Add a task to the thread pool.

        Args:
            task (DrillTask): the task
        """
        self.processes[task.ref] = task.alg
        task.signals.started.connect(self.taskStarted)
        task.signals.finished.connect(self.taskFinished)
        self.tasksNumber += 1
        self.threadPool.start(task)

    def abortProcessing(self):
        """
        Abort the processing. This function stops the currently running process
        and remove the pending ones from the queue.
        """
        self.threadPool.clear()
        for p in self.processes:
            try:
                self.processes[p].cancel()
                self.signals.taskError.emit(p)
            except:
                pass
        self.processes.clear()

    def progressHandle(self, p, msg):
        """
        Override AlgorithmObserver::progressHandle. Called when a running
        algorithm is sending a progress step. This function is calculating
        an effective progress based on the number of running threads and
        send a signal to update the view.

        Args:
            p (float): progress value between 0.0 and 1.0
            msg (str): a facultative string sent by the algorithm to inform on
                       the progress
        """
        progress = int((p + self.tasksFinished) * 100 / self.tasksNumber)
        self.signals.progressUpdate.emit(progress)

