# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal

from mantid.kernel import logger

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
    Name of the group, if the sample is in a group.
    """
    _group = None

    """
    Index of the sample in its group.
    """
    _groupIndex = None

    """
    True if the sample is the master sample of its group.
    """
    _master = None

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

    """
    Sent if the group changed.
    """
    groupChanged = Signal()

    """
    Sent when a new parameter is added.
    Args:
        DrillParameter: the new parameter
    """
    newParameter = Signal(DrillParameter)

    def __init__(self, index):
        """
        Create an empty sample.
        """
        super().__init__()
        self._parameters = dict()
        self._index = index
        self._group = None
        self._master = False

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

    def setGroup(self, group, index=None):
        """
        Set the group and index of the sample.

        Args:
            group (str): name of the group. None if the sample is not in a
                         group.
            index (int): index of the sample in its group
        """
        if not group:
            self._group = None
            self._groupIndex = None
            self._master = False
        else:
            self._group = group
            self._groupIndex = index
        self.groupChanged.emit()

    def getGroupName(self):
        """
        Get the name of the group.

        Returns:
            str: name of the group. None if the sample is not in a group
        """
        return self._group

    def getGroupIndex(self):
        """
        Get the index of the sample in its group.

        Returns:
            int: index of the sample. None if the sample is not in a group
        """
        return self._groupIndex

    def setMaster(self, state):
        """
        Set the master status.

        Args:
            state (bool): if True, the sample become the master sample of its
                          group
        """
        if self._group is None:
            self._master = False
            return
        self._master = state
        self.groupChanged.emit()

    def isMaster(self):
        """
        Return the "group master" status.

        Returns:
            bool: True if the sample is the master sample of its group.
        """
        return self._master

    def addParameter(self, name):
        """
        Add a new sample parameter. If this parameter exists, it will be
        replaced.

        Args:
            parameter (DrillParameter): new sample parameter

        Returns:
            DrillParameter: the new empty parameter
        """
        parameter = DrillParameter(name)
        parameter.setController(self._controller)
        self._parameters[name] = parameter
        self.newParameter.emit(parameter)
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

    def onProcessStarted(self):
        """
        Triggered when the sample processing starts.
        """
        logger.information("Starting of sample {0} processing"
                           .format(self._index + 1))
        self.processStarted.emit()

    def onProcessSuccess(self):
        """
        Triggered when the sample process succeed.
        """
        logger.information("Processing of sample {0} finished with sucess"
                           .format(self._index + 1))
        self.processDone.emit(0)

    def onProcessError(self, msg):
        """
        Triggered when the sample processing end with an error.

        Args:
            msg (str): error message
        """
        logger.error("Error while processing sample {0}: {1}"
                     .format(self._index + 1, msg))
        self.processDone.emit(-1)
