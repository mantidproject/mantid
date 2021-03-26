# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, QRunnable, Signal

import mantid.simpleapi as sapi

from .DrillAlgorithmObserver import DrillAlgorithmObserver


class DrillTaskSignals(QObject):
    """
    Signals that the DrillTask could send.
    """
    """
    Sent when the task starts.
    Args:
        object: the task
    """
    started = Signal(object)
    """
    Sent when the task finishes.
    Args:
        object: the task
        int: return value (0: success)
        str: error message
    """
    finished = Signal(object, int, str)
    """
    Sent when the task progress is updated.
    Args:
        object: the task
        float: progress value between 0.0 and 1.0
    """
    progress = Signal(object, float)


class DrillTask(QRunnable):
    """
    Class that defines a processing task in the drill interface.
    """
    """
    Name of the task.
    """
    _name = None

    def __init__(self, name, alg, **kwargs):
        super(DrillTask, self).__init__()
        self._name = name
        self.signals = DrillTaskSignals()
        self.algName = alg
        self.alg = None
        self.properties = kwargs

    def getName(self):
        """
        Get the name of the task.

        Returns:
            str: name
        """
        return self._name

    def run(self):
        """
        Override QRunnable::run. Provide the running part of the task that will
        start in an other thread.
        """
        self.signals.started.emit(self)
        self.alg = sapi.AlgorithmManager.create(self.algName)
        errors = list()
        for (k, v) in self.properties.items():
            try:
                self.alg.setProperty(k, v)
            except Exception as e:
                errors.append(str(e))
        if errors:
            self.signals.finished.emit(self, 1, " - ".join(errors))
            return
        # setup the observer
        self.observer = DrillAlgorithmObserver()
        self.observer.observeFinish(self.alg)
        self.observer.observeError(self.alg)
        self.observer.signals.finished.connect(lambda ret, msg: self.signals.finished.emit(self, ret, msg))
        self.observer.observeProgress(self.alg)
        self.observer.signals.progress.connect(lambda p: self.signals.progress.emit(self, p))
        try:
            ret = self.alg.execute()
            if not ret:
                self.signals.finished.emit(self, 1, "")
        except Exception as ex:
            self.signals.finished.emit(self, 1, str(ex))
        except KeyboardInterrupt:
            pass

    def cancel(self):
        """
        Cancel the algorithm. This function first tests if the algorithm is
        currently running. It avoids emitting error signal for cancelling non
        running ones.
        """
        if self.alg and self.alg.isRunning():
            self.alg.cancel()
            self.signals.finished.emit(self, 1, "Processing cancelled")
