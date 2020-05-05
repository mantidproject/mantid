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
    started = Signal(int)         # task reference
    finished = Signal(int)        # task reference
    error = Signal(int)           # task reference
    progress = Signal(int, float) # task reference and progress between 0.0 and 1.0


class DrillTask(QRunnable):
    """
    Class that defines a processing task in the drill interface.
    """
    def __init__(self, ref, alg, **kwargs):
        super(DrillTask, self).__init__()
        self.ref = ref
        self.signals = DrillTaskSignals()
        # setup the algo
        self.alg = sapi.AlgorithmManager.create(alg)
        for (k, v) in kwargs.items():
            self.alg.setProperty(k, v)
        # setup the observer
        self.observer = DrillAlgorithmObserver()
        self.observer.observeFinish(self.alg)
        self.observer.signals.finished.connect(
                lambda : self.signals.finished.emit(self.ref)
                )
        self.observer.observeError(self.alg)
        self.observer.signals.error.connect(
                lambda : self.signals.error.emit(self.ref)
                )
        self.observer.observeProgress(self.alg)
        self.observer.signals.progress.connect(
                lambda p : self.signals.progress.emit(self.ref, p)
                )

    def run(self):
        """
        Override QRunnable::run. Provide the running part of the task that will
        start in an other thread.
        """
        self.signals.started.emit(self.ref)
        try:
            self.alg.execute()
        except Exception as e:
            print(e)
            self.signals.error.emit(self.ref)
        except:
            self.signals.error.emit(self.ref)

    def cancel(self):
        """
        Cancel the algorithm. This function first tests if the algorithm is
        currently running. It avoids emitting error signal for cancelling non
        running ones.
        """
        if self.alg.isRunning():
            self.alg.cancel()
            self.signals.error.emit(self.ref)

