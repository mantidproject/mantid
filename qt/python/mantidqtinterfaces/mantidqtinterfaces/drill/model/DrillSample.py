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
    Status of sample when its processing is done.
    """

    STATUS_PROCESSED = "processing_done"

    """
    Status of the sample when its processing ended with an error.
    """
    STATUS_ERROR = "processing_error"

    """
    Status of the sample when its processing is running.
    """
    STATUS_PENDING = "processing"

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
    Sample group, if the sample is in a group.
    """
    _group = None

    _status = None

    """
    Controller for the parameters.
    """
    _controller = None

    """
    Reference to the exporter.
    """
    _exporter = None

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

    """
    Signals that the status of the sample changed.
    """
    statusChanged = Signal()

    def __init__(self, index):
        """
        Create an empty sample.
        """
        super().__init__()
        self._parameters = dict()
        self._index = index
        self._group = None

    def setController(self, controller):
        """
        Controller for the parameters.

        Args:
            controller (DrillParameterController): parameter controller
        """
        self._controller = controller

    def setExporter(self, exporter):
        """
        Set the exporter.

        Args:
            exporter (DrillExportModel): new exporter
        """
        self._exporter = exporter

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

    def setGroup(self, group):
        """
        Set the group and index of the sample.

        Args:
            group (DrillSampleGroup): group. None if the sample is not in a
                                      group.
        """
        self._group = group
        self.groupChanged.emit()

    def getGroup(self):
        """
        Get the name of the group.

        Returns:
            DrillSampleGroup: group. None if the sample is not in a group
        """
        return self._group

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
        parameter.valueChanged.connect(self.onParameterChanged)
        self._parameters[name] = parameter
        self.newParameter.emit(parameter)
        if self._status is not None:
            self._status = None
            self.statusChanged.emit()
        return parameter

    def delParameter(self, name):
        """
        Delete a parameter from its name.

        Args:
            name (str): name of the parameter
        """
        if name in self._parameters:
            del self._parameters[name]
            if self._status is not None:
                self._status = None
                self.statusChanged.emit()

    def onParameterChanged(self):
        """
        Triggered when a parameter changed its value. This function udpates the
        sample status.
        """
        if self._status is not None:
            self._status = None
            self.statusChanged.emit()

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

    def isValid(self):
        """
        Check if all the parameters are valid.

        Returns:
            bool: True if the sample is valid
        """
        for _, param in self._parameters.items():
            if not param.isValid():
                return False
        return True

    def getParameterValues(self):
        """
        Get the parameters as a dictionnary.

        Returns:
            dict(str: str): parameters
        """
        return {name: param.getValue() for name, param in self._parameters.items()}

    def onProcessStarted(self):
        """
        Triggered when the sample processing starts.
        """
        logger.information("Starting of sample {0} processing".format(self._index + 1))
        self._status = self.STATUS_PENDING
        self.statusChanged.emit()

    def onProcessSuccess(self):
        """
        Triggered when the sample process succeed.
        """
        logger.information("Processing of sample {0} finished with sucess".format(self._index + 1))
        if self._exporter is not None:
            self._exporter.run(self)
        self._status = self.STATUS_PROCESSED
        self.statusChanged.emit()

    def onProcessError(self, msg):
        """
        Triggered when the sample processing end with an error.

        Args:
            msg (str): error message
        """
        if msg:
            logger.error("Error while processing sample {0}: {1}".format(self._index + 1, msg))
        self._status = self.STATUS_ERROR
        self.statusChanged.emit()

    def setStatus(self, status):
        """
        Set the sample status. Must be one of STATUS_PROCESSED, STATUS_ERROR,
        STATUS_PENDING or None.

        Args:
            str: sample status
        """
        self._status = status

    def getStatus(self):
        """
        Get the sample status.

        Returns:
            str: sample status
        """
        return self._status
