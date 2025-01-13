# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os
from qtpy.QtCore import QObject, Signal, QEvent

from qtpy.QtWidgets import QDialog, QComboBox, QCheckBox, QLineEdit, QLabel

from qtpy import uic

from mantidqt.widgets.filefinderwidget import FileFinderWidget
from mantidqt.widgets.workspaceselector import WorkspaceSelector


class MouseScrollEventFilter(QObject):
    """
    Event filter to eat the scroll event. This is used on comboboxes that appear
    in potential scroll area to avoid scrolling the combobox while scrolling in
    the dialog.
    """

    def __init__(self):
        super(MouseScrollEventFilter, self).__init__()

    def eventFilter(self, obj, event):
        """
        Override QObject::eventFilter

        Args:
            obj (QObject): object on which the event is called
            event (QEvent): event received
        """
        return event.type() == QEvent.Wheel


class DrillSetting(QObject):
    """
    Class that groups needed data of a single setting that will be displayed
    on the dialog.
    """

    valueChanged = Signal(str)
    fileChecked = Signal(bool)

    def __init__(self, name, values, settingType, doc):
        """
        Initialize the setting by providing its name, allowed values, type and
        documentation. This constructor chooses a widget to represent the
        setting on the basis of its type.

        Args:
            name (str): setting name
            values (list(str)): allowed values. This is only used with combobox
                                type settings
            settingType (str): a str that represents the setting type
            doc (str): setting documentation
        """
        super(DrillSetting, self).__init__()
        self._doc = doc

        if (settingType == "file") or (settingType == "files"):
            self._widget = FileFinderWidget()
            self._widget.isOptional(True)
            if settingType == "files":
                self._widget.allowMultipleFiles(True)
            self._widget.setLabelText("")
            self._widget.fileInspectionFinished.connect(lambda: self.fileChecked.emit(self._widget.isValid()))
            self._setter = self._widget.setUserInput
            self._getter = self._widget.getUserInput

        if settingType == "workspace":
            self._widget = WorkspaceSelector()
            self._widget.setOptional(True)
            self._widget.currentTextChanged.connect(lambda t: self.valueChanged.emit(name))
            self._setter = self._widget.setCurrentText
            self._getter = self._widget.currentText

        elif settingType == "combobox":
            self._widget = QComboBox()
            self._widgetEventFilter = MouseScrollEventFilter()
            self._widget.installEventFilter(self._widgetEventFilter)
            self._widget.addItems(values)
            self._widget.currentTextChanged.connect(lambda t: self.valueChanged.emit(name))
            self._setter = self._widget.setCurrentText
            self._getter = self._widget.currentText

        elif settingType == "bool":
            self._widget = QCheckBox()
            self._widget.stateChanged.connect(lambda s: self.valueChanged.emit(name))
            self._setter = self._widget.setChecked
            self._getter = self._widget.isChecked

        elif (settingType == "floatArray") or (settingType == "intArray"):
            self._widget = QLineEdit()
            self._widget.textChanged.connect(lambda _: self.valueChanged.emit(name))
            self._setter = lambda v: self._widget.setText(",".join(str(e) for e in v))

            if settingType == "floatArray":
                self._getter = lambda: [float(v) for v in self._widget.text().split(",") if v]
            else:
                self._getter = lambda: [int(v) for v in self._widget.text().split(",") if v]

        elif settingType == "string":
            self._widget = QLineEdit()
            self._widget.textChanged.connect(lambda _: self.valueChanged.emit(name))
            self._setter = lambda v: self._widget.setText(str(v))
            self._getter = self._widget.text

    @property
    def doc(self):
        """
        Get the documentation.

        Returns:
            str: documentation
        """
        return self._doc

    @property
    def widget(self):
        """
        Get the widget.

        Returns
            QWidget: widget
        """
        return self._widget

    @property
    def setter(self):
        """
        Get the function that could be used to set the widget value.

        Returns
            fn: setter function
        """
        return self._setter

    @property
    def getter(self):
        """
        Get the function that could be used to get the widget value.

        Returns
            fn: getter function
        """
        return self._getter


class DrillSettingsDialog(QDialog):
    """
    Class that automatically create a dialog containing widgets for each
    provided settings.
    """

    ui_filename = "ui/settings.ui"

    valueChanged = Signal(str)  # setting name

    """
    Sent when the apply button is clicked.
    """
    applied = Signal()

    def __init__(self, presenter, parent=None):
        """
        Initialize ths dialog. Connect the static buttons.
        """
        super(DrillSettingsDialog, self).__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, self.ui_filename), self)
        self.okButton.clicked.connect(self.accept)
        self.cancelButton.clicked.connect(self.reject)
        self.applyButton.clicked.connect(lambda: self.applied.emit())

        # widgets
        self.settings = dict()

        self.invalidSettings = set()

        self._presenter = presenter

    def initWidgets(self, types, values, doc):
        """
        Initialize the dialog widgets by providing the settings types, allowed
        values and documentation. This method create a widget by setting and
        add them to the dialog.

        Args:
            types (dict(str: str)): setting name and type
            values (dict(str: list(str))): setting name and allowed values
            doc (dict(str: str)): setting name and documentation
        """
        for n, t in types.items():
            label = QLabel(n, self)
            self.settings[n] = DrillSetting(n, values[n], types[n], doc[n])
            self.settings[n].valueChanged.connect(self.onValueChanged)
            self.settings[n].fileChecked.connect(lambda v, n=n: self.onSettingValidation(n, v))

            widget = self.settings[n].widget
            widget.setToolTip(doc[n])

            self.formLayout.addRow(label, widget)

    def onValueChanged(self, setting):
        """
        Check the get value before sending the signal.

        Args:
            setting (str): name of the setting
        """
        try:
            self.getSettingValue(setting)
            self.valueChanged.emit(setting)
        except:
            self.onSettingValidation(setting, False, "Unable to parse the value. Check the input")

    def setSettings(self, settings):
        """
        Fill the settings with values by providing a dictionnary.

        Args:
            settings (dict(str: any)): settings name and value. Value can be
                                       anything, other that bool will be
                                       converted to str.
        """
        for k, v in settings.items():
            if k in self.settings:
                self.settings[k].setter(v)

    def getSettings(self):
        """
        Get all the settings as a dictionnary.

        Returns:
            dict(str: any): setting name and value.
        """
        settings = dict()
        for k, v in self.settings.items():
            settings[k] = v.getter()
        return settings

    def setSettingValue(self, name, value):
        """
        Set the value of a specific setting.

        Args:
            value (any): the value
        """
        if name in self.settings:
            self.settings[name].setter(value)

    def getSettingValue(self, name):
        """
        Get a setting value.

        Args:
            name (str): setting name

        Returns:
            (any): setting value. The type can be str or bool depending on the
                   setting
        """
        if name in self.settings:
            return self.settings[name].getter()
        else:
            return ""

    def onSettingValidation(self, name, valid, msg=None):
        """
        This method is called when the validation has been done. It changes the
        stylesheet of the corresponding widget to inform the user on the
        parameter state (valid or not). It changes the widget tooltip to
        display the error msg. It also toogles the state of the validation
        button depending on the presence or not of invalid values in the dialog.

        Args:
            name (str): setting name
            valid (bool): True if the setting is valid
            msg (str): facultative error message associated with an invalid
                       setting
        """
        if name not in self.settings:
            return

        if valid:
            self.settings[name].widget.setStyleSheet("")
            self.settings[name].widget.setToolTip(self.settings[name].doc)
            self.invalidSettings.discard(name)
            if not self.invalidSettings:
                self.okButton.setDisabled(False)
                self.applyButton.setDisabled(False)
        else:
            self.settings[name].widget.setStyleSheet("QLineEdit {background-color: #3fff0000;}")
            if msg is not None:
                self.settings[name].widget.setToolTip(msg)
            else:
                self.settings[name].widget.setToolTip(self.settings[name].doc)
            self.invalidSettings.add(name)
            self.okButton.setDisabled(True)
            self.applyButton.setDisabled(True)
