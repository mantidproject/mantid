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
    Index of the sample.
    """
    _index = None

    """
    Controller for the parameters.
    """
    _controller = None

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

    def addParameter(self, parameter):
        """
        Add a sample parameter.

        Args:
            parameter (DrillParameter): new sample parameter
        """
        name = parameter.getName()
        self._parameters[name] = parameter

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
