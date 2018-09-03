from __future__ import (absolute_import, division, print_function)

from qtpy import QtCore, QtGui, QtWidgets
from qtpy.QtCore import Signal

import Muon.GUI.Common.run_string_utils as runUtils


class LoadRunWidgetView(QtWidgets.QWidget):

    # signals for parent widgets
    loadingStarted = Signal()
    loadingFinished = Signal()
    dataChanged = Signal()

    def __init__(self, parent=None):
        super(LoadRunWidgetView, self).__init__(parent)
        self.setupUi(self)
        self.set_run_edit_regex()

        self._warning_window = None
        self._cached_text = ""

    def setupUi(self, Form):
        Form.setObjectName("Form")
        Form.resize(468, 45)

        self.horizontalLayout = QtWidgets.QHBoxLayout(self)
        self.horizontalLayout.setObjectName("horizontalLayout")

        self.loadCurrentRunButton = QtWidgets.QPushButton(Form)
        self.loadCurrentRunButton.setText("Load Current Run")
        self.loadCurrentRunButton.setMinimumSize(QtCore.QSize(100, 25))
        self.loadCurrentRunButton.setToolTip("Load the current run for the current instrument")
        self.loadCurrentRunButton.setObjectName("loadCurrentRunButton")

        self.incrementRunButton = QtWidgets.QToolButton(Form)
        self.incrementRunButton.setText(">")
        self.incrementRunButton.setMinimumSize(QtCore.QSize(25, 25))
        self.incrementRunButton.setToolTip("Increment the run")
        self.incrementRunButton.setObjectName("incrementRunButton")

        self.decrementRunButton = QtWidgets.QToolButton(Form)
        self.decrementRunButton.setText("<")
        self.decrementRunButton.setMinimumSize(QtCore.QSize(25, 25))
        self.decrementRunButton.setToolTip("Decrement the run")
        self.decrementRunButton.setObjectName("decrementRunButton")

        self.instrumentLabel = QtWidgets.QLabel(Form)
        self.instrumentLabel.setText("Instrument")
        self.instrumentLabel.setToolTip("")
        self.instrumentLabel.setObjectName("instrumentLabel")

        self.runEdit = QtWidgets.QLineEdit(Form)
        self.runEdit.setMinimumSize(QtCore.QSize(0, 25))
        self.runEdit.setToolTip(
            "Enter run number using " + runUtils.delimiter
            + " and " + runUtils.range_separator + " as delimiter and range-separator respectively")
        self.runEdit.setObjectName("runEdit")

        spacerItem = QtWidgets.QSpacerItem(25, 25, QtWidgets.QSizePolicy.Minimum,
                                           QtWidgets.QSizePolicy.Minimum)

        self.horizontalLayout.addWidget(self.loadCurrentRunButton)
        self.horizontalLayout.addWidget(self.decrementRunButton)
        self.horizontalLayout.addWidget(self.instrumentLabel)
        self.horizontalLayout.addWidget(self.runEdit)
        self.horizontalLayout.addWidget(self.incrementRunButton)
        self.horizontalLayout.addItem(spacerItem)

    def set_run_edit_regex(self):
        regex = QtCore.QRegExp(runUtils.run_string_regex)  # "^[0-9]*([0-9]+[,-]{0,1})*[0-9]+$"
        validator = QtGui.QRegExpValidator(regex)
        self.runEdit.setValidator(validator)

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
        self.loadCurrentRunButton.setEnabled(False)
        self.runEdit.setEnabled(False)
        self.incrementRunButton.setEnabled(False)
        self.decrementRunButton.setEnabled(False)

    def enable_load_buttons(self):
        self.loadCurrentRunButton.setEnabled(True)
        self.runEdit.setEnabled(True)
        self.incrementRunButton.setEnabled(True)
        self.decrementRunButton.setEnabled(True)

    def set_instrument_label(self, text):
        self.instrumentLabel.setText(text)

    def set_run_edit_text(self, text):
        self.runEdit.setText(text)
        self._cached_text = self.get_run_edit_text()

    def reset_run_edit_from_cache(self):
        tmp = self._cached_text
        self.set_run_edit_text(tmp)
        self._cached_text = tmp

    def set_current_instrument(self, instrument):
        self.instrumentLabel.setText(instrument)

    def get_run_edit_text(self):
        return self.runEdit.text()

    # Signal/slot connections called by presenter

    def on_decrement_run_clicked(self, slot):
        self.decrementRunButton.clicked.connect(slot)

    def on_increment_run_clicked(self, slot):
        self.incrementRunButton.clicked.connect(slot)

    def on_load_current_run_clicked(self, slot):
        self.loadCurrentRunButton.clicked.connect(slot)

    def on_run_edit_changed(self, slot):
        self.runEdit.returnPressed.connect(slot)

    def warning_popup(self, message):
        self._warning_window = None
        self._warning_window = QtWidgets.QMessageBox.warning(self, "Error", str(message))

    def clear(self):
        self.set_run_edit_text("")
