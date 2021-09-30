# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import queue
import threading
import time

from qtpy.QtCore import QObject, Signal

import mantid.simpleapi as sapi

from .DrillParameter import DrillParameter


class DrillControllerSignals(QObject):
    """
    Signals that the controller is using
    """

    okParam = Signal(DrillParameter)
    wrongParam = Signal(DrillParameter)


class DrillParameterController(threading.Thread):
    """
    Class that checks algorithm parameters. It runs in a separate daemon
    thread.
    """

    def __init__(self, algName):
        """
        Create a controller for a specific algorithm.

        Args:
            algName (str): algorithm name
        """
        super(DrillParameterController, self).__init__()
        self.daemon = True
        self._signals = DrillControllerSignals()
        self._paramQueue = queue.Queue()
        self._alg = sapi.AlgorithmManager.createUnmanaged(algName)
        self._alg.initialize()
        self._running = True

    @property
    def signals(self):
        return self._signals

    def check(self, parameter):
        """
        Add a parameter for validation.

        Args:
            parameter (DrillParameter): the parameter to be validated
        """
        self._paramQueue.put(parameter)

    def stop(self):
        """
        Stop the running thread.
        """
        self._running = False
        self.join()

    def run(self):
        """
        This method runs in a separate thread. I polls a queue and check the
        available parameters. Depending on the check, the corresponding
        signal is thrown and the error message of the parameter is added if
        needed.
        """
        while self._running:
            try:
                p = self._paramQueue.get(timeout=0.1)
                time.sleep(0.01)
                try:
                    pName = p.getName()
                    pValue = p.getValue()
                    defaultValue = self._alg.getProperty(pName).getDefault
                    self._alg.setProperty(pName, pValue)
                    try:
                        self._alg.setProperty(pName, defaultValue)
                    except:
                        pass # in case of mandatory parameter
                    p.setValidationState(True)
                except Exception as e:
                    p.setValidationState(False, str(e))
            except Exception:
                pass
