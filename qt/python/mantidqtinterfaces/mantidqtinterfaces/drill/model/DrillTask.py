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
    Sent when the task finishes.
    """
    finished = Signal(object)

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

    """
    Callbacks for task start.
    """
    _startCallbacks = None

    """
    Callbacks in case of success.
    """
    _successCallbacks = None

    """
    Callbacks in case of error.
    """
    _errorCallbacks = None

    def __init__(self, name, alg, **kwargs):
        super(DrillTask, self).__init__()
        self._name = name
        self._startCallbacks = list()
        self._successCallbacks = list()
        self._errorCallbacks = list()
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

    def addStartedCallback(self, callback):
        """
        Add a callback in the start callbaks list.

        Args:
            callback (function): callback
        """
        self._startCallbacks.append(callback)

    def addSuccessCallback(self, callback):
        """
        Add a callback in the success callbaks list.

        Args:
            callback (function): callback
        """
        self._successCallbacks.append(callback)

    def addErrorCallback(self, callback):
        """
        Add a callback in the error callbacks list.

        Args:
            callback (function): callback
        """
        self._errorCallbacks.append(callback)

    def run(self):
        """
        Override QRunnable::run. Provide the running part of the task that will
        start in an other thread.
        """
        for f in self._startCallbacks:
            f()
        self.alg = sapi.AlgorithmManager.create(self.algName)
        errors = list()
        for k, v in self.properties.items():
            try:
                self.alg.setProperty(k, v)
            except Exception as e:
                errors.append(str(e))
        if errors:
            self._onFinished(1, " - ".join(errors))
            return
        # setup the observer
        self.observer = DrillAlgorithmObserver()
        self.observer.observeFinish(self.alg)
        self.observer.observeError(self.alg)
        self.observer.signals.finished.connect(self._onFinished)
        self.observer.observeProgress(self.alg)
        self.observer.signals.progress.connect(lambda p: self.signals.progress.emit(self, p))
        try:
            ret = self.alg.execute()
            if not ret:
                self._onFinished(1, "")
        except Exception as ex:
            self._onFinished(1, str(ex))
        except KeyboardInterrupt:
            pass

    def _onFinished(self, returnCode, errorMsg=""):
        """
        To be called when task is finished. If will call the adapted callbacks
        and emit the finish signal.

        Args:
            returnCode (int): return code of the task
            errorMsg (str): optionnal error message
        """
        if returnCode == 0:
            for fct in self._successCallbacks:
                fct()
        else:
            for fct in self._errorCallbacks:
                fct(errorMsg)
        self.signals.finished.emit(self)

    def cancel(self):
        """
        Cancel the algorithm. This function first tests if the algorithm is
        currently running. It avoids emitting error signal for cancelling non
        running ones.
        """
        if self.alg and self.alg.isRunning():
            self.alg.cancel()
            self._onFinished(1, "Processing cancelled")
