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
    finished = Signal(int, str)  # return code (0: success - 1: error), error msg
    progress = Signal(float)     # progress value between 0.0 and 1.0


class DrillAlgorithmObserver(AlgorithmObserver):
    """
    Class that defines an observer for the algorithms started through the DrILL
    interface. It basically overrides the handle methods to propagate signals.
    """
    def __init__(self):
        super(DrillAlgorithmObserver, self).__init__()
        self.signals = DrillAlgorithmObserverSignals()
        self.error = False
        self.errorMsg = None

    def finishHandle(self):
        """
        Called when the observed algo is finished.
        """
        if self.error:
            self.signals.finished.emit(1, self.errorMsg)
        else:
            self.signals.finished.emit(0, "")

    def errorHandle(self, msg):
        """
        Called when the observed algo encounter an error.
        """
        self.error = True
        self.errorMsg = msg

    def progressHandle(self, p, msg):
        """
        Called when the observed algo reports its progress.

        Args:
            p (float): progress value between 0.0 and 1.0
            msp (str): an associated message
        """
        self.signals.progress.emit(p)

