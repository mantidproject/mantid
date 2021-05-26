# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal


class DrillParameter(QObject):

    """
    """
    _sample = None

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

    def __init__(self, sample, name, controller):
        super().__init__()
        self._sample = sample
        self._name = name
        self._controller = controller

    def getName(self):
        return self._name

    def setValue(self, value):
        self._value = value
        self._controller.check(self)

    def getValue(self):
        return self._value
