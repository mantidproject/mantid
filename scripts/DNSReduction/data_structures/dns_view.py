# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Common Class for DNS Views - supports easy setting and getting of values
"""
from __future__ import (absolute_import, division, print_function)
from collections import OrderedDict

from qtpy.QtWidgets import QWidget, QDoubleSpinBox, QLineEdit, QCheckBox
from qtpy.QtWidgets import QSpinBox, QRadioButton, QGroupBox, QMessageBox, QSlider, QComboBox
from qtpy.QtCore import Qt

from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()


class DNSView(QWidget):
    def __init__(self, parent):
        super(DNSView, self).__init__(parent)
        self.parent = parent
        self._content = None
        self.has_tab = True
        self._mapping = {}
        self.error_dialog = None
        self.within_mantid = within_mantid

    def set_single_state(self, target_object, value):
        """
         Setting widget value independent of type
        """
        if isinstance(target_object, QDoubleSpinBox):
            target_object.setValue(float(value))
        elif isinstance(target_object, QSpinBox):
            target_object.setValue(int(value))
        elif isinstance(target_object, QLineEdit):
            target_object.setText(str(value))
        elif isinstance(target_object, QCheckBox):
            target_object.setChecked(value)
        elif isinstance(target_object, QRadioButton):
            target_object.setChecked(value)
        elif isinstance(target_object, QComboBox):
            index = target_object.findText(
                value,
                Qt.MatchFixedString)  ### crapy workaround for Qt4 compability
            if index >= 0:
                target_object.setCurrentIndex(index)
        elif isinstance(target_object, QSlider):
            target_object.setValue(value)
        elif isinstance(target_object, QGroupBox):
            if target_object.isCheckable():
                target_object.setChecked(value)

    def get_single_state(self, target_object):
        """
        Returning of widget value independent of type
        """
        if isinstance(target_object, QDoubleSpinBox):
            return target_object.value()
        if isinstance(target_object, QSpinBox):
            return target_object.value()
        if isinstance(target_object, QLineEdit):
            return target_object.text()
        if isinstance(target_object, QCheckBox):
            return target_object.isChecked()
        if isinstance(target_object, QRadioButton):
            return target_object.isChecked()
        if isinstance(target_object, QComboBox):
            return target_object.currentText()
        if isinstance(target_object, QSlider):
            return target_object.value()
        if isinstance(target_object, QGroupBox):
            if target_object.isCheckable():
                return target_object.isChecked()
        return None

    def get_state(self):
        """
        Retuning of a dictionary with the names of the widgets as keys and the values
        """
        state_dict = OrderedDict()
        for key, target_object in self._mapping.items():
            state = self.get_single_state(target_object)
            if state is not None:  ## pushbuttons for example are not defined in the get function
                state_dict[key] = state
        return state_dict

    def set_state(self, state_dict):
        """
        Stetting the gui state from a dictionary
        containing the shortnames of the widgets as keys and the values
        """
        for key, target_object in self._mapping.items():
            self.set_single_state(target_object,
                                  value=state_dict.get(key, None))
        return

    def raise_error(self, message, critical=False, info=False):
        """
        Errors are shown as popups
        """
        self.error_dialog = QMessageBox()
        if critical:
            self.error_dialog.setIcon(QMessageBox.Critical)
        elif info:
            self.error_dialog.setIcon(QMessageBox.information)
        else:
            self.error_dialog.setIcon(QMessageBox.Warning)
        self.error_dialog.setText(message)
        self.error_dialog.setWindowTitle("Error")
        self.error_dialog.exec_()

    def show_statusmessage(self, message='', time=1, clear=False):
        """Change of status message in global DNS GUI """
        self.parent.show_statusmessage(message, time, clear=clear)
