# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import numpy

from qtpy.QtCore import QObject, Signal

from mantid.kernel import *
from mantid.api import *


class DrillParameter(QObject):

    FILE_TYPE = "file"
    FILES_TYPE = "files"
    WORKSPACE_TYPE = "workspace"
    COMBOBOX_TYPE = "combobox"
    STRING_TYPE = "string"
    BOOL_TYPE = "bool"
    FLOAT_ARRAY_TYPE = "floatArray"
    INT_ARRAY_TYPE = "intArray"

    """
    Name of the parameter.
    """
    _name = None

    """
    Value of the parameter.
    """
    _value = None

    """
    Parameter documentation.
    """
    _documentation = None

    """
    Parameter type.
    """
    _type = None

    """
    Parameter allowed values.
    """
    _allowedValues = None

    """
    Reference to the parameter controller.
    """
    _controller = None

    """
    Sent when the parameter is valid.
    """
    valid = Signal()

    """
    Sent when the parameter is invalid.
    Args:
        str: error message
    """
    invalid = Signal(str)

    """
    Sent when the parameter value changed.
    """
    valueChanged = Signal()

    def __init__(self, name):
        super().__init__()
        self._name = name

    def setController(self, controller):
        """
        Set the parameter controller.

        Args:
            controller (DrillParameterController): controller
        """
        self._controller = controller

    def initFromProperty(self, mantidProperty):
        """
        Inititialize the parameter from a mantid property. This method populates
        the type, documentation, allowed values of the parameters. It also sets
        the value to the property default value.

        Args:
            mantidProperty (Property): the property
        """
        self._documentation = mantidProperty.documentation
        self._allowedValues = mantidProperty.allowedValues
        value = mantidProperty.value
        if (isinstance(mantidProperty, FileProperty)):
            self._type = self.FILE_TYPE
        elif (isinstance(mantidProperty, MultipleFileProperty)):
            self._type = self.FILES_TYPE
            if not value:
                value = ""
        elif (isinstance(mantidProperty, (WorkspaceGroupProperty,
                                          MatrixWorkspaceProperty))):
            self._type = self.WORKSPACE_TYPE
        elif (isinstance(mantidProperty, StringPropertyWithValue)):
            if mantidProperty.allowedValues:
                self._type = self.COMBOBOX_TYPE
            else:
                self._type = self.STRING_TYPE
        elif (isinstance(mantidProperty, BoolPropertyWithValue)):
            self._type = self.BOOL_TYPE
        elif (isinstance(mantidProperty, FloatArrayProperty)):
            self._type = self.FLOAT_ARRAY_TYPE
        elif (isinstance(mantidProperty, IntArrayProperty)):
            self._type = self.INT_ARRAY_TYPE
        else:
            self._type = self.STRING_TYPE

        if isinstance(value, numpy.ndarray):
            self._value = value.tolist()
        elif value is None:
            self._value = ""
        else:
            self._value = value

    def getName(self):
        """
        Get the name of the parameter.

        Args:
            name (str): name of the parameter
        """
        return self._name

    def setValue(self, value):
        """
        Set the parameter value. If a controller is available, the value will
        be checked.

        Args:
            value (any): value
        """
        self._value = value
        if self._controller:
            self._controller.check(self)
        self.valueChanged.emit()

    def getValue(self):
        """
        Get the parameter value.

        Returns:
            (any): value
        """
        return self._value

    def getType(self):
        """
        Get the parameter type.

        Returns:
            (str): type
        """
        return self._type

    def getAllowedValues(self):
        """
        Get the allowed value.

        Returns:
            (list(str)): allowed values
        """
        return self._allowedValues

    def getDocumentation(self):
        """
        Get the parameter documentation.

        Returns:
            (str): documentation
        """
        return self._documentation
