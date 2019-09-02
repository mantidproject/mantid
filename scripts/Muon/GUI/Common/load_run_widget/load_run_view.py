# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Signal

import Muon.GUI.Common.utilities.run_string_utils as run_utils
from Muon.GUI.Common.message_box import warning


class LoadRunWidgetView(QtWidgets.QWidget):
    # signals for parent widgets
    loadingStarted = Signal()
    loadingFinished = Signal()
    dataChanged = Signal()

    def __init__(self, parent=None):
        super(LoadRunWidgetView, self).__init__(parent)
        self.load_current_run_button = None
        self.increment_run_button = None
        self.decrement_run_button = None
        self.horizontal_layout = None
        self.instrument_label = None
        self.run_edit = None
        self.spacer_item = None

        self.setup_interface_layout()

        self.set_run_edit_regex()

        self._cached_text = ""

    def setup_interface_layout(self):
        self.setObjectName("LoadRunWidget")
        self.resize(468, 45)

        self.load_current_run_button = QtWidgets.QPushButton(self)
        self.load_current_run_button.setText("Load Current Run")
        self.load_current_run_button.setToolTip("Load the current run for the current instrument")
        self.load_current_run_button.setObjectName("loadCurrentRunButton")

        self.increment_run_button = QtWidgets.QToolButton(self)
        self.increment_run_button.setText(">")
        self.increment_run_button.setToolTip("Increment the run")
        self.increment_run_button.setObjectName("incrementRunButton")

        self.decrement_run_button = QtWidgets.QToolButton(self)
        self.decrement_run_button.setText("<")
        self.decrement_run_button.setToolTip("Decrement the run")
        self.decrement_run_button.setObjectName("decrementRunButton")

        self.instrument_label = QtWidgets.QLabel(self)
        self.instrument_label.setText("Instrument")
        self.instrument_label.setToolTip("")
        self.instrument_label.setObjectName("instrumentLabel")

        self.run_edit = QtWidgets.QLineEdit(self)
        self.run_edit.setToolTip(
            "Enter run number using " + run_utils.delimiter
            + " and " + run_utils.range_separator + " as delimiter and range-separator respectively")
        self.run_edit.setObjectName("runEdit")

        self.horizontal_layout = QtWidgets.QHBoxLayout(self)
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.load_current_run_button)
        self.horizontal_layout.addWidget(self.decrement_run_button)
        self.horizontal_layout.addWidget(self.instrument_label)
        self.horizontal_layout.addWidget(self.run_edit)
        self.horizontal_layout.addWidget(self.increment_run_button)

        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)

    def getLayout(self):
        return self.horizontalLayout

    # ------------------------------------------------------------------------------------------------------------------
    # Enabling / disabling the interface
    # ------------------------------------------------------------------------------------------------------------------

    def disable_loading(self):
        self.disable_load_buttons()
        self.loadingStarted.emit()

    def enable_loading(self):
        self.enable_load_buttons()
        self.loadingFinished.emit()
        self.dataChanged.emit()

    def notify_loading_started(self):
        self.loadingStarted.emit()

    def notify_loading_finished(self):
        self.loadingFinished.emit()
        self.dataChanged.emit()

    def disable_load_buttons(self):
        self.load_current_run_button.setEnabled(False)
        self.run_edit.setEnabled(False)
        self.increment_run_button.setEnabled(False)
        self.decrement_run_button.setEnabled(False)

    def enable_load_buttons(self):
        self.load_current_run_button.setEnabled(True)
        self.run_edit.setEnabled(True)
        self.increment_run_button.setEnabled(True)
        self.decrement_run_button.setEnabled(True)

    # ------------------------------------------------------------------------------------------------------------------
    # Instrument / run-edit
    # ------------------------------------------------------------------------------------------------------------------

    def get_instrument_label(self):
        return str(self.instrument_label.text())

    def set_instrument_label(self, text):
        self.instrument_label.setText(text)

    def set_current_instrument(self, instrument):
        self.instrument_label.setText(instrument)

    def set_run_edit_regex(self):
        # The regular expression string here is "^[0-9]*([0-9]+[,-]{0,1})*[0-9]+$"
        regex = QtCore.QRegExp(run_utils.run_string_regex)
        validator = QtGui.QRegExpValidator(regex)
        self.run_edit.setValidator(validator)

    def set_run_edit_text(self, text):
        self.run_edit.setText(text)
        self._cached_text = self.get_run_edit_text()

    def set_run_edit_without_validator(self, text):
        self.run_edit.setValidator(None)
        self.run_edit.setText(text)
        self.set_run_edit_regex()

    def reset_run_edit_from_cache(self):
        tmp = self._cached_text
        self.set_run_edit_text(tmp)
        self._cached_text = tmp

    def get_run_edit_text(self):
        return str(self.run_edit.text())

    def warning_popup(self, message):
        warning(message, parent=self)

    def clear(self):
        self.set_run_edit_text("")

    # ------------------------------------------------------------------------------------------------------------------
    # Signal/slot connections called by presenter
    # ------------------------------------------------------------------------------------------------------------------

    def on_decrement_run_clicked(self, slot):
        self.decrement_run_button.clicked.connect(slot)

    def on_increment_run_clicked(self, slot):
        self.increment_run_button.clicked.connect(slot)

    def on_load_current_run_clicked(self, slot):
        self.load_current_run_button.clicked.connect(slot)

    def on_run_edit_changed(self, slot):
        self.run_edit.returnPressed.connect(slot)
