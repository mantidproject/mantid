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


class Parameter:
    """
    Class that defines a parameter to be checked.
    """

    def __init__(self, name, value, sample):
        """
        Create a parameter by giving its name, value and the sample to which it
        is associated.

        Args:
            name (str): parameter name
            value (str): parameter value
            sample (int): associated sample
        """
        self._name = name
        self._value = value
        self._sample = sample
        self._errorMsg = str()

    @property
    def name(self):
        """
        Get the parameter name

        Returns:
            str: parameter name
        """
        return self._name

    @property
    def value(self):
        """
        Get the parameter value

        Returns:
            str: parameter value
        """
        return self._value

    @property
    def sample(self):
        """
        Get the sample number assiociated with this parameter.

        Returns:
            int: sample number
        """
        return self._sample

    @property
    def errorMsg(self):
        """
        Get the error message of the parameter if its validation failed.

        Returns:
            str: error message
        """
        return self._errorMsg

    @errorMsg.setter
    def errorMsg(self, msg):
        """
        Set the error message.

        Args:
            msg (str): error message
        """
        self._errorMsg = msg


class ControllerSignals(QObject):
    """
    Signals that the controller is using
    """

    okParam = Signal(Parameter)
    wrongParam = Signal(Parameter)


class ParameterController(threading.Thread):
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
        super(ParameterController, self).__init__()
        self.daemon = True
        self._signals = ControllerSignals()
        self._paramQueue = queue.Queue()
        self._alg = sapi.AlgorithmManager.createUnmanaged(algName)
        self._alg.initialize()
        self._running = True

    @property
    def signals(self):
        return self._signals

    def addParameter(self, parameter):
        """
        Add a parameter for validation.

        Args:
            parameter (Parameter): the parameter to be validated
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
                time.sleep(0.001)
                try:
                    defaultValue = self._alg.getProperty(p.name).getDefault
                    self._alg.setProperty(p.name, p.value)
                    self._alg.setProperty(p.name, defaultValue)
                    self._signals.okParam.emit(p)
                except Exception as e:
                    p.errorMsg = str(e)
                    self._signals.wrongParam.emit(p)
            except:
                pass
