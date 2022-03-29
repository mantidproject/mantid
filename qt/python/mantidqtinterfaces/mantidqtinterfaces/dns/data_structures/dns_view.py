# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Common Class for DNS Views - supports easy setting and getting of values
"""

from collections import OrderedDict

from qtpy.QtCore import Qt
from qtpy.QtWidgets import (QCheckBox, QComboBox, QDoubleSpinBox, QGroupBox,
                            QLineEdit, QMessageBox, QRadioButton, QSlider,
                            QSpinBox, QToolButton, QWidget)


class DNSView(QWidget):
    HAS_TAB = True
    NAME = 'DNSView'

    def __init__(self, parent):
        super().__init__(parent)
        if parent is not None:
            self.setVisible(False)
        self.parent = parent
        self._map = {}
        self.menues = []
        self.app = getattr(parent, 'app', None)
        self.within_manitd = getattr(parent, 'within_mantid', None)

    def process_events(self):
        self.app.processEvents()

    def set_single_state_by_name(self, name, value):
        if name in self._map:
            self.set_single_state(self._map[name], value)

    @staticmethod
    def set_single_state(target_object, value):
        """
         Setting widget value independent of type
        """
        if value is None:
            return
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
                str(value),
                Qt.MatchFixedString)  # crapy workaround for Qt4 compability
            if index >= 0:
                target_object.setCurrentIndex(index)
        elif isinstance(target_object, QSlider):
            target_object.setValue(value)
        elif isinstance(target_object, QToolButton):
            target_object.setChecked(value)
        elif isinstance(target_object, QGroupBox):
            if target_object.isCheckable():
                target_object.setChecked(value)

    @staticmethod
    def _get_single_state(target_object):
        # pylint: disable=too-many-return-statements
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
        if isinstance(target_object, QToolButton):
            return target_object.isChecked()
        if isinstance(target_object, QGroupBox):
            if target_object.isCheckable():
                return target_object.isChecked()
        return None

    def get_state(self):
        """
        Retuning of a dictionary with the names of the widgets
        as keys and the values
        """
        state_dict = OrderedDict()
        for key, target_object in self._map.items():
            state = self._get_single_state(target_object)
            if state is not None:
                # pushbuttons for example are not defined in the get function
                state_dict[key] = state
        return state_dict

    def set_state(self, state_dict):
        """
        Stetting the gui state from a dictionary
        containing the shortnames of the widgets as keys and the values
        """
        for key, target_object in self._map.items():
            self.set_single_state(target_object,
                                  value=state_dict.get(key, None))

    @staticmethod
    def raise_error(message, critical=False, info=False):
        """
        Errors are shown as popups
        """
        error_dialog = QMessageBox()
        if critical:
            error_dialog.setIcon(QMessageBox.Critical)
        elif info:
            error_dialog.setIcon(QMessageBox.information)
        else:
            error_dialog.setIcon(QMessageBox.Warning)
        error_dialog.setText(message)
        error_dialog.setWindowTitle("Error")
        error_dialog.exec_()

    def show_statusmessage(self, message='', time=1, clear=False):
        """Change of status message in global DNS GUI """
        if self.parent is not None:
            self.parent.show_statusmessage(message, time, clear=clear)
