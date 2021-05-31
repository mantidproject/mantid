# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal

from .DrillParameter import DrillParameter


class DrillSample(QObject):

    """
    Processing parameters.
    """
    _parameters = None

    """
    Name of the sample.
    """
    _name = None

    """
    Name used for the output workspace.
    """
    _outputName = None

    """
    Index of the sample.
    """
    _index = None

    """
    Controller for the parameters.
    """
    _controller = None

    """
    Triggered when the processing of the sample started.
    """
    processStarted = Signal()

    """
    Triggered when the processing of the sample finished.
    Args:
        int: return code. 0: success; -1: error
    """
    processDone = Signal(int)

    def __init__(self, index):
        """
        Create an empty sample.
        """
        super().__init__()
        self._parameters = dict()
        self._index = index

    def setController(self, controller):
        """
        Controller for the parameters.

        Args:
            controller (DrillParameterController): parameter controller
        """
        self._controller = controller

    def setIndex(self, index):
        """
        Set the sample index.

        Args:
            index (int): index of the sample
        """
        self._index = index

    def getIndex(self):
        """
        Get the sample index.

        Returns:
            int: index of the sample
        """
        return self._index

    def setName(self, name):
        """
        Set the sample name.

        Args:
            name (str): name of the sample
        """
        self._name = name

    def getName(self):
        """
        Get the sample name.

        Returns:
            str: name of the sample
        """
        return self._name

    def setOutputName(self, name):
        """
        Set the name of the output workspace.

        Args:
            name (str): name
        """
        self._outputName = name

    def getOutputName(self):
        """
        Get the name of the output workspace.

        Returns:
            str: name
        """
        return self._outputName

    def addParameter(self, name):
        """
        Add a new sample parameter. If this parameter exists, it will be
        replaced.

        Args:
            parameter (DrillParameter): new sample parameter

        Returns:
            DrillParameter: the new empty parameter
        """
        parameter = DrillParameter(name, self._controller)
        self._parameters[name] = parameter
        return parameter

    def delParameter(self, name):
        """
        Delete a parameter from its name.

        Args:
            name (str): name of the parameter
        """
        if name in self._parameters:
            del self._parameters[name]

    def getParameter(self, name):
        """
        Get the parameter from its name or None if it does not exist.

        Args:
            name (str): name of the parameter

        Returns:
            DrillParameter: the sample parameter
        """
        if name in self._parameters:
            return self._parameters[name]
        else:
            return None

    def getParameterValues(self):
        """
        Get the parameters as a dictionnary.

        Returns:
            dict(str: str): parameters
        """
        out = dict()
        for name, param in self._parameters.items():
            out[name] = param.getValue()
        return out
