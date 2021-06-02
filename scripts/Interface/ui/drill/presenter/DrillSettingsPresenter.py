# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ..model.DrillParameter import DrillParameter
from ..view.DrillSettingsDialog import DrillSettingsDialog


class DrillSettingsPresenter:

    """
    Settings dialog.
    """
    _dialog = None

    """
    List of processing parameters.
    """
    _parameters = None

    _initialValues = None

    def __init__(self, parent, parameters):
        self._dialog = DrillSettingsDialog(self, parent=parent)
        self._parameters = dict()
        self._initialValues = dict()
        types = dict()
        values = dict()
        doc = dict()
        for p in parameters:
            name = p.getName()
            p.valid.connect(lambda name=name: self.onValid(name))
            p.invalid.connect(lambda msg, name=name: self.onInvalid(name, msg))
            self._parameters[name] = p
            types[name] = p.getType()
            values[name] = p.getAllowedValues()
            doc[name] = p.getDocumentation()
        self._dialog.initWidgets(types, values, doc)
        for p in parameters:
            name = p.getName()
            value = p.getValue()
            self._initialValues[name] = value
            self._dialog.setSettingValue(name, value)
        self._dialog.valueChanged.connect(self.onValueChanged)
        self._dialog.rejected.connect(self.onRejected)
        self._dialog.show()

    def onValueChanged(self, parameterName):
        """
        Triggered when the setting value changed in the view.

        Args:
            parameterName (str): name of the parameter
        """
        value = self._dialog.getSettingValue(parameterName)
        self._parameters[parameterName].setValue(value)

    def onValid(self, parameterName):
        """
        Triggered when the parameter value is valid.

        Args:
            parameterName (str): name of the parameter
        """
        self._dialog.onSettingValidation(parameterName, True)

    def onInvalid(self, ParameterName, msg):
        """
        Triggered when the parameter value is invalid.

        Args:
            parameterName (str): name of the parameter
            msg (str): error message
        """
        self._dialog.onSettingValidation(ParameterName, False, msg)

    def onRejected(self):
        """
        Triggered when the dialog send the "reject" signal (i.e. when "cancel"
        is pressed. This method restores the initial values in the different
        parameters.
        """
        for name in self._initialValues:
            value = self._initialValues[name]
            self._parameters[name].setValue(value)
