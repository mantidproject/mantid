# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class DrillSample:
    """
    Processing parameters.
    """
    _parameters = None
    """
    Name of the output workspace.
    """
    _outputName = None

    def __init__(self):
        """
        Create an empty sample.
        """
        self._parameters = dict()

    def setParameters(self, parameters):
        """
        Set the processing parameters.

        Args:
            parameters (dict(str:str)): parameter key:value pairs
        """
        self._parameters = {k: v for k, v in parameters.items()}

    def getParameters(self):
        """
        Get the processing parameters.

        Returns:
            dict(str:str): parameter key:value pairs
        """
        return {k: v for k, v in self._parameters.items()}

    def getParameter(self, name):
        """
        Get the value of a parameter.

        Args:
            name (str): name of the parameter

        Returns:
            value of the parameter, None if it does not exist
        """
        if name in self._parameters:
            return self._parameters[name]
        else:
            return None

    def changeParameter(self, name, value):
        """
        Change a parameter value. If this parameter is not already present, it
        will be created. If the value is empty, the parameter is deleted.

        Args:
            name (str): name of the parameter name
            value (str): value of the parameter
        """
        if value == "":
            if name in self._parameters:
                del self._parameters[name]
        else:
            self._parameters[name] = value

    def setOutputName(self, name):
        """
        Set the name of the output.

        Args:
            name (str): name
        """
        self._outputName = name

    def getOutputName(self):
        """
        Get the name of the output.

        Returns:
            str: name
        """
        return self._outputName
