# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal

from mantid.api import AlgorithmObserver


class DrillAlgorithmObserverSignals(QObject):
    """
    Signals that the observer could send.
    """
    finished = Signal()
    error = Signal()
    progress = Signal(float)  # progress value between 0.0 and 1.0


class DrillAlgorithmObserver(AlgorithmObserver):
    """
    Class that defines an observer for the algorithms started through the DrILL
    interface. It basically overrides the handle methods to propagate signals.
    """
    def __init__(self):
        super(DrillAlgorithmObserver, self).__init__()
        self.signals = DrillAlgorithmObserverSignals()

    def finishHandle(self):
        """
        Called when the observed algo is finished.
        """
        self.signals.finished.emit()

    def errorHandle(self, msg):
        """
        Called when the observed algo encounter an error.
        """
        self.signals.error.emit()

    def progressHandle(self, p, msg):
        """
        Called when the observed algo reports its progress.

        Args:
            p (float): progress value between 0.0 and 1.0
            msp (str): an associated message
        """
        self.signals.progress.emit(p)

