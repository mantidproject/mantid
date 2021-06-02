# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal


class DrillParameter(QObject):

    """
    Name of the parameter.
    """
    _name = None

    """
    Value of the parameter.
    """
    _value = None

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
        self._controller = controller

    def getName(self):
        return self._name

    def setValue(self, value):
        self._value = value
        if self._controller:
            self._controller.check(self)
        self.valueChanged.emit()

    def getValue(self):
        return self._value
