# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, QRunnable, Signal

import mantid.simpleapi as sapi

class DrillTaskSignals(QObject):
    """
    Signals that the DrillTask could send.
    """
    started = Signal(int)        # the task reference
    finished = Signal(int, int)  # the task reference and the return code


class DrillTask(QRunnable):
    """
    Class that defines a processing task in the drill interface.
    """
    def __init__(self, ref, alg, **kwargs):
        super(DrillTask, self).__init__()
        self.ref = ref
        self.alg = alg
        self.kwargs = kwargs
        self.signals = DrillTaskSignals()

    def run(self):
        """
        Override QRunnable::run. Provide the running part of the task that will
        start in an other thread.
        """
        self.signals.started.emit(self.ref)
        try:
            fn = getattr(sapi, self.alg)
            fn(**self.kwargs)
            self.signals.finished.emit(self.ref, 0)
        except Exception as e:
            print(e)
            self.signals.finished.emit(self.ref, -1)
        except:
            pass

