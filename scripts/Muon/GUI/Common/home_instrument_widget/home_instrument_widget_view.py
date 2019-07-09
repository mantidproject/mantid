# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# -*- coding: utf8 -*-

from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Signal
from Muon.GUI.Common.utilities.muon_file_utils import allowed_instruments, show_file_browser_and_return_selection
from Muon.GUI.Common.utilities.run_string_utils import valid_float_regex
from Muon.GUI.Common.message_box import warning


class InstrumentWidgetView(QtWidgets.QWidget):
    dataChanged = Signal()

    def __init__(self, parent=None):
        super(InstrumentWidgetView, self).__init__(parent)

        self.layout = QtWidgets.QGridLayout(self)

        self._button_height = 40
        self._cached_instrument = ["None", "None"]

        self.setup_interface()
        self.dead_time_file_loader_hidden(True)
        self.dead_time_other_file_hidden(True)

        self.dead_time_selector.currentIndexChanged.connect(
            self.on_dead_time_combo_changed)
        self.rebin_selector.currentIndexChanged.connect(
            self.on_rebin_combo_changed)
        self.time_zero_checkbox.stateChanged.connect(
            self.on_time_zero_checkbox_state_change)
        self.first_good_data_checkbox.stateChanged.connect(
            self.on_first_good_data_checkbox_state_change)
        self.last_good_data_checkbox.stateChanged.connect(
            self.on_last_good_data_checkbox_state_change)

        self._on_dead_time_from_data_selected = None
        self._on_dead_time_from_other_file_selected = lambda: 0

        self.first_good_data_checkbox.setChecked(True)
        self.time_zero_checkbox.setChecked(True)
        self.time_zero_edit_enabled(True)
        self.first_good_data_edit_enabled(True)
        self.last_good_data_edit_enabled(True)

        self._on_time_zero_changed = lambda: 0
        self._on_first_good_data_changed = lambda: 0
        self._on_dead_time_from_file_selected = lambda: 0
        self._on_dead_time_file_option_selected = lambda: 0
        self._on_dead_time_unselected = lambda: 0

        self.time_zero_edit.editingFinished.connect(
            lambda: self._on_time_zero_changed() if not self.is_time_zero_checked() else None)
        self.first_good_data_edit.editingFinished.connect(
            lambda: self._on_first_good_data_changed() if not self.is_first_good_data_checked() else None)
        self.last_good_data_edit.editingFinished.connect(
            lambda: self._on_last_good_data_changed() if not self.is_last_good_data_checked() else None)
        self.dead_time_file_selector.currentIndexChanged.connect(
            self.on_dead_time_file_combo_changed)

    def setup_interface(self):
        self.setObjectName("InstrumentWidget")

        self.setup_instrument_row()
        self.setup_time_zero_row()
        self.setup_first_good_data_row()
        self.setup_last_good_data_row()
        self.setup_dead_time_row()
        self.setup_rebin_row()

        self.group = QtWidgets.QGroupBox("Instrument")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                           "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: 0px;'
                           'padding-bottom: 0px;'
                           "padding-right: 10px;"
                           ' color: grey; }')

        self.group.setLayout(self.layout)

        self.group2 = QtWidgets.QGroupBox("Rebin")
        self.group2.setFlat(False)

        self.group2.setLayout(self.rebin_layout)

        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.widget_layout.addWidget(self.group2)
        self.setLayout(self.widget_layout)

    def show_file_browser_and_return_selection(
            self, file_filter, search_directories, multiple_files=False):
        return show_file_browser_and_return_selection(self, file_filter, search_directories, multiple_files)

    def set_combo_boxes_to_default(self):
        self.rebin_selector.setCurrentIndex(0)
        self.rebin_fixed_hidden(True)
        self.rebin_variable_hidden(True)
        self.dead_time_selector.setCurrentIndex(0)
        self.dead_time_data_info_hidden(True)
        self.dead_time_file_loader_hidden(True)

    def set_checkboxes_to_defualt(self):
        self.time_zero_checkbox.setChecked(True)
        self.first_good_data_checkbox.setChecked(True)

    def warning_popup(self, message):
        warning(message, parent=self)

    # ------------------------------------------------------------------------------------------------------------------
    # Instrument selection
    # ------------------------------------------------------------------------------------------------------------------

    def _fixed_aspect_ratio_size_policy(self, widget):
        size_policy = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Fixed,
            QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(widget.sizePolicy().hasHeightForWidth())
        return size_policy

    def setup_instrument_row(self):
        self.instrument_selector = QtWidgets.QComboBox(self)
        self.instrument_selector.setSizePolicy(
            self._fixed_aspect_ratio_size_policy(
                self.instrument_selector))
        self.instrument_selector.addItems(allowed_instruments)

        self.instrument_label = QtWidgets.QLabel(self)
        self.instrument_label.setText("Instrument : ")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.addWidget(self.instrument_label)
        self.horizontal_layout.addWidget(self.instrument_selector)
        self.horizontal_layout.addStretch(0)

        self.layout.addWidget(self.instrument_label, 0, 0)
        self.layout.addWidget(self.instrument_selector, 0, 1)

    def get_instrument(self):
        return str(self.instrument_selector.currentText())

    def set_instrument(self, instrument, block=False):
        index = self.instrument_selector.findText(instrument)
        if index != -1:
            self.instrument_selector.blockSignals(block)
            self.instrument_selector.setCurrentIndex(index)
            self.instrument_selector.blockSignals(False)

    def on_instrument_changed(self, slot):
        self.instrument_selector.currentIndexChanged.connect(slot)
        self.instrument_selector.currentIndexChanged.connect(
            self.cache_instrument)

    def cache_instrument(self):
        self._cached_instrument.pop(0)
        self._cached_instrument.append(
            str(self.instrument_selector.currentText()))

    @property
    def cached_instrument(self):
        return self._cached_instrument[-1]

    def instrument_changed_warning(self):
        msg = QtWidgets.QMessageBox(self)
        msg.setIcon(QtWidgets.QMessageBox.Warning)
        msg.setText("Changing instrument will reset the interface, continue?")
        msg.setWindowTitle("Changing Instrument")
        msg.setStandardButtons(
            QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel)
        retval = msg.exec_()
        if retval == 1024:
            # The "OK" code
            return 1
        else:
            return 0

    # ------------------------------------------------------------------------------------------------------------------
    # Time zero
    # ------------------------------------------------------------------------------------------------------------------

    def setup_time_zero_row(self):
        self.time_zero_label = QtWidgets.QLabel(self)
        self.time_zero_label.setText("Time Zero : ")

        self.time_zero_edit = QtWidgets.QLineEdit(self)
        timezero_validator = QtGui.QRegExpValidator(
            QtCore.QRegExp(valid_float_regex),
            self.time_zero_edit)
        self.time_zero_edit.setValidator(timezero_validator)
        self.time_zero_edit.setText("")

        self.time_zero_unit_label = QtWidgets.QLabel(self)
        self.time_zero_unit_label.setText(u"\u03BCs (From data file ")

        self.time_zero_checkbox = QtWidgets.QCheckBox(self)
        self.time_zero_checkbox.setChecked(True)

        self.time_zero_label_2 = QtWidgets.QLabel(self)
        self.time_zero_label_2.setText(" )")

        self.time_zero_layout = QtWidgets.QHBoxLayout()
        self.time_zero_layout.addSpacing(10)
        self.time_zero_layout.addWidget(self.time_zero_unit_label)
        self.time_zero_layout.addWidget(self.time_zero_checkbox)
        self.time_zero_layout.addWidget(self.time_zero_label_2)
        self.time_zero_layout.addStretch(0)

        self.layout.addWidget(self.time_zero_label, 1, 0)
        self.layout.addWidget(self.time_zero_edit, 1, 1)
        self.layout.addItem(self.time_zero_layout, 1, 2)

    def set_time_zero(self, time_zero):
        self.time_zero_edit.setText(
            "{0:.3f}".format(round(float(time_zero), 3)))

    def get_time_zero(self):
        return float(self.time_zero_edit.text())

    def time_zero_edit_enabled(self, enabled):
        self.time_zero_edit.setEnabled(not enabled)

    def is_time_zero_checked(self):
        return self.time_zero_checkbox.isChecked()

    def on_time_zero_changed(self, slot):
        self._on_time_zero_changed = slot

    def on_time_zero_checkState_changed(self, slot):
        self.time_zero_checkbox.stateChanged.connect(slot)

    def time_zero_state(self):
        return self.time_zero_checkbox.isChecked()

    def on_time_zero_checkbox_state_change(self):
        self.time_zero_edit_enabled(self.time_zero_checkbox.isChecked())

    # ------------------------------------------------------------------------------------------------------------------
    # First good data
    # ------------------------------------------------------------------------------------------------------------------

    def setup_first_good_data_row(self):

        self.first_good_data_label = QtWidgets.QLabel(self)
        self.first_good_data_label.setText("First Good Data : ")

        self.first_good_data_edit = QtWidgets.QLineEdit(self)

        first_good_data_validator = QtGui.QRegExpValidator(
            QtCore.QRegExp(valid_float_regex),
            self.time_zero_edit)
        self.first_good_data_edit.setValidator(first_good_data_validator)
        self.first_good_data_edit.setText("")

        self.first_good_data_unit_label = QtWidgets.QLabel(self)
        self.first_good_data_unit_label.setText(u" \u03BCs (From data file ")

        self.first_good_data_checkbox = QtWidgets.QCheckBox(self)
        self.first_good_data_checkbox.setChecked(True)

        self.first_good_data_label_2 = QtWidgets.QLabel(self)
        self.first_good_data_label_2.setText(" )")

        self.first_good_data_layout = QtWidgets.QHBoxLayout()
        self.first_good_data_layout.addSpacing(10)
        self.first_good_data_layout.addWidget(self.first_good_data_unit_label)
        self.first_good_data_layout.addWidget(self.first_good_data_checkbox)
        self.first_good_data_layout.addWidget(self.first_good_data_label_2)
        self.first_good_data_layout.addStretch(0)

        self.layout.addWidget(self.first_good_data_label, 2, 0)
        self.layout.addWidget(self.first_good_data_edit, 2, 1)
        self.layout.addItem(self.first_good_data_layout, 2, 2)

    def on_first_good_data_changed(self, slot):
        self._on_first_good_data_changed = slot

    def set_first_good_data(self, first_good_data):
        self.first_good_data_edit.setText(
            "{0:.3f}".format(round(float(first_good_data), 3)))

    def on_first_good_data_checkState_changed(self, slot):
        self.first_good_data_checkbox.stateChanged.connect(slot)

    def first_good_data_state(self):
        return self.first_good_data_checkbox.checkState()

    def is_first_good_data_checked(self):
        return self.first_good_data_checkbox.checkState()

    def on_first_good_data_checkbox_state_change(self):
        self.first_good_data_edit_enabled(
            self.first_good_data_checkbox.checkState())

    def first_good_data_edit_enabled(self, disabled):
        self.first_good_data_edit.setEnabled(not disabled)

    def get_first_good_data(self):
        return float(self.first_good_data_edit.text())

    # ------------------------------------------------------------------------------------------------------------------
    # Last Good Data
    # -------------------------------------------------------------------------------------------------------------------

    def setup_last_good_data_row(self):

        self.last_good_data_label = QtWidgets.QLabel(self)
        self.last_good_data_label.setText("Last Good Data : ")

        self.last_good_data_edit = QtWidgets.QLineEdit(self)

        last_good_data_validator = QtGui.QRegExpValidator(
            QtCore.QRegExp(valid_float_regex),
            self.first_good_data_edit)
        self.last_good_data_edit.setValidator(last_good_data_validator)
        self.last_good_data_edit.setText("")

        self.last_good_data_unit_label = QtWidgets.QLabel(self)
        self.last_good_data_unit_label.setText(u" \u03BCs (From data file ")

        self.last_good_data_checkbox = QtWidgets.QCheckBox(self)
        self.last_good_data_checkbox.setChecked(True)

        self.last_good_data_label_2 = QtWidgets.QLabel(self)
        self.last_good_data_label_2.setText(" )")

        self.last_good_data_layout = QtWidgets.QHBoxLayout()
        self.last_good_data_layout.addSpacing(10)
        self.last_good_data_layout.addWidget(self.last_good_data_unit_label)
        self.last_good_data_layout.addWidget(self.last_good_data_checkbox)
        self.last_good_data_layout.addWidget(self.last_good_data_label_2)
        self.last_good_data_layout.addStretch(0)

        self.layout.addWidget(self.last_good_data_label, 3, 0)
        self.layout.addWidget(self.last_good_data_edit, 3, 1)
        self.layout.addItem(self.last_good_data_layout, 3, 2)

    def on_last_good_data_changed(self, slot):
        self._on_last_good_data_changed = slot

    def set_last_good_data(self, last_good_data):
        self.last_good_data_edit.setText(
            "{0:.3f}".format(round(float(last_good_data), 3)))

    def on_last_good_data_checkState_changed(self, slot):
        self.last_good_data_checkbox.stateChanged.connect(slot)

    def last_good_data_state(self):
        return self.last_good_data_checkbox.checkState()

    def is_last_good_data_checked(self):
        return self.last_good_data_checkbox.checkState()

    def on_last_good_data_checkbox_state_change(self):
        self.last_good_data_edit_enabled(
            self.last_good_data_checkbox.checkState())

    def last_good_data_edit_enabled(self, disabled):
        self.last_good_data_edit.setEnabled(not disabled)

    def get_last_good_data(self):
        return float(self.last_good_data_edit.text())

    # ------------------------------------------------------------------------------------------------------------------
    # Dead time correction
    # ------------------------------------------------------------------------------------------------------------------

    def setup_dead_time_row(self):
        self.dead_time_label = QtWidgets.QLabel(self)
        self.dead_time_label.setText("Dead Time : ")

        self.dead_time_selector = QtWidgets.QComboBox(self)
        self.dead_time_selector.addItems(
            ["From data file",
             "From table workspace",
             "From other file",
             "None"])

        self.dead_time_label_2 = QtWidgets.QLabel(self)
        self.dead_time_label_2.setText("Dead Time Workspace : ")

        self.dead_time_label_3 = QtWidgets.QLabel(self)
        self.dead_time_label_3.setText("No loaded dead time")

        self.dead_time_file_selector = QtWidgets.QComboBox(self)
        self.dead_time_file_selector.addItem("None")
        self.dead_time_file_selector.setToolTip(
            "Select a table which is loaded into the ADS.")

        self.dead_time_browse_button = QtWidgets.QPushButton(self)
        self.dead_time_browse_button.setText("Browse")
        self.dead_time_browse_button.setToolTip("Browse for a .nxs file to load dead times from. If valid, the "
                                                "dead times will be saved as a table, and automatically selected "
                                                "as the dead time for the current data.")

        self.dead_time_layout = QtWidgets.QHBoxLayout()
        self.dead_time_layout.addSpacing(10)
        self.dead_time_layout.addWidget(self.dead_time_label_3)

        self.dead_time_file_layout = QtWidgets.QHBoxLayout()
        self.dead_time_file_layout.addWidget(self.dead_time_browse_button)
        self.dead_time_file_layout.addStretch(0)

        self.dead_time_other_file_label = QtWidgets.QLabel(self)
        self.dead_time_other_file_label.setText("From other file : ")

        self.layout.addWidget(self.dead_time_label, 4, 0)
        self.layout.addWidget(self.dead_time_selector, 4, 1)
        self.layout.addItem(self.dead_time_layout, 4, 2)
        self.layout.addWidget(self.dead_time_label_2, 5, 0)
        self.layout.addWidget(self.dead_time_file_selector, 5, 1)
        self.layout.addWidget(self.dead_time_other_file_label, 6, 0)
        self.layout.addWidget(self.dead_time_browse_button, 6, 1)

    def on_dead_time_file_option_changed(self, slot):
        self._on_dead_time_file_option_selected = slot

    def on_dead_time_from_data_selected(self, slot):
        self._on_dead_time_from_data_selected = slot

    def on_dead_time_unselected(self, slot):
        self._on_dead_time_unselected = slot

    def on_dead_time_browse_clicked(self, slot):
        self.dead_time_browse_button.clicked.connect(slot)

    def on_dead_time_from_file_selected(self, slot):
        self._on_dead_time_from_file_selected = slot

    def populate_dead_time_combo(self, names):
        self.dead_time_file_selector.blockSignals(True)
        self.dead_time_file_selector.clear()
        self.dead_time_file_selector.addItem("None")
        for name in names:
            self.dead_time_file_selector.addItem(name)
        self.dead_time_file_selector.blockSignals(False)

    def get_dead_time_file_selection(self):
        return self.dead_time_file_selector.currentText()

    def set_dead_time_file_selection_text(self, text):
        index = self.dead_time_file_selector.findText(text)
        if index >= 0:
            self.dead_time_file_selector.setCurrentIndex(index)
            return True
        return False

    def set_dead_time_file_selection(self, index):
        self.dead_time_file_selector.setCurrentIndex(index)

    def set_dead_time_selection(self, index):
        self.dead_time_selector.setCurrentIndex(index)

    def dead_time_file_loader_hidden(self, hidden=True):
        if hidden:
            self.dead_time_file_selector.hide()

            self.dead_time_label_2.hide()
            self.dead_time_data_info_hidden(hidden)
        if not hidden:
            self.dead_time_file_selector.setVisible(True)
            self.dead_time_label_2.setVisible(True)
            self.dead_time_data_info_hidden(hidden)

    def dead_time_other_file_hidden(self, hidden):
        if hidden:
            self.dead_time_other_file_label.hide()
            self.dead_time_browse_button.hide()

        if not hidden:
            self.dead_time_browse_button.setVisible(True)
            self.dead_time_other_file_label.setVisible(True)

    def dead_time_data_info_hidden(self, hidden=True):
        if hidden:
            self.dead_time_label_3.hide()
        if not hidden:
            self.dead_time_label_3.setVisible(True)

    def set_dead_time_label(self, text):
        self.dead_time_label_3.setText(text)

    def on_dead_time_combo_changed(self, index):
        if index == 0:
            self._on_dead_time_from_data_selected()
            self.dead_time_file_loader_hidden(True)
            self.dead_time_data_info_hidden(False)
            self.dead_time_other_file_hidden(True)
        if index == 1:
            self._on_dead_time_from_file_selected()
            self.dead_time_file_loader_hidden(False)
            self.dead_time_data_info_hidden(False)
            self.dead_time_other_file_hidden(True)
        if index == 2:
            self._on_dead_time_from_other_file_selected()
            self.dead_time_file_loader_hidden(True)
            self.dead_time_data_info_hidden(True)
            self.dead_time_other_file_hidden(False)
        if index == 3:
            self._on_dead_time_unselected()
            self.dead_time_file_loader_hidden(True)
            self.dead_time_data_info_hidden(True)
            self.dead_time_other_file_hidden(True)

    def on_dead_time_from_other_file_selected(self, slot):
        self._on_dead_time_from_other_file_selected = slot

    def on_dead_time_file_combo_changed(self, index):
        self._on_dead_time_file_option_selected()

    # ------------------------------------------------------------------------------------------------------------------
    # Rebin row
    # ------------------------------------------------------------------------------------------------------------------

    def setup_rebin_row(self):
        self.rebin_label = QtWidgets.QLabel(self)
        self.rebin_label.setText("Rebin : ")

        self.rebin_selector = QtWidgets.QComboBox(self)
        self.rebin_selector.addItems(["None", "Fixed", "Variable"])

        self.rebin_steps_label = QtWidgets.QLabel(self)
        self.rebin_steps_label.setText("Steps : ")

        self.rebin_steps_edit = QtWidgets.QLineEdit(self)
        int_validator = QtGui.QDoubleValidator()
        self.rebin_steps_edit.setValidator(int_validator)
        self.rebin_steps_edit.setToolTip(
            'Value to scale current bin width by.')

        self.rebin_variable_label = QtWidgets.QLabel(self)
        self.rebin_variable_label.setText("Bin Boundaries : ")
        self.rebin_variable_edit = QtWidgets.QLineEdit(self)
        self.rebin_variable_edit.setToolTip('A comma separated list of first bin boundary, width, last bin boundary.\n'
                                            'Optionally this can be followed by a comma and more widths and last boundary pairs.\n'
                                            'Optionally this can also be a single number, which is the bin width.\n'
                                            'Negative width values indicate logarithmic binning.\n\n'
                                            'For example:\n'
                                            '2,-0.035,10: from 2 rebin in Logarithmic bins of 0.035 up to 10;\n'
                                            '0,100,10000,200,20000: from 0 rebin in steps of 100 to 10,000 then steps of 200 to 20,000')
        variable_validator = QtGui.QRegExpValidator(
            QtCore.QRegExp('^(\s*-?\d+(\.\d+)?)(\s*,\s*-?\d+(\.\d+)?)*$'))
        self.rebin_variable_edit.setValidator(variable_validator)

        self.rebin_layout = QtWidgets.QHBoxLayout()
        self.rebin_layout.addSpacing(10)

        self.rebin_layout.addWidget(self.rebin_label)
        self.rebin_layout.addWidget(self.rebin_selector)

        self.rebin_layout.addWidget(self.rebin_steps_label)
        self.rebin_layout.addWidget(self.rebin_steps_edit)
        self.rebin_layout.addWidget(self.rebin_variable_label)
        self.rebin_layout.addWidget(self.rebin_variable_edit)
        self.rebin_layout.addStretch(0)
        self.rebin_layout.addSpacing(10)

        self.rebin_steps_label.hide()
        self.rebin_steps_edit.hide()
        self.rebin_variable_label.hide()
        self.rebin_variable_edit.hide()

    def rebin_fixed_hidden(self, hidden=True):
        if hidden:
            self.rebin_steps_label.hide()
            self.rebin_steps_edit.hide()
        if not hidden:
            self.rebin_steps_label.setVisible(True)
            self.rebin_steps_edit.setVisible(True)

    def rebin_variable_hidden(self, hidden=True):
        if hidden:
            self.rebin_variable_label.hide()
            self.rebin_variable_edit.hide()
        if not hidden:
            self.rebin_variable_label.setVisible(True)
            self.rebin_variable_edit.setVisible(True)

    def on_rebin_combo_changed(self, index):
        if index == 0:
            self.rebin_fixed_hidden(True)
            self.rebin_variable_hidden(True)
        if index == 1:
            self.rebin_fixed_hidden(False)
            self.rebin_variable_hidden(True)
        if index == 2:
            self.rebin_fixed_hidden(True)
            self.rebin_variable_hidden(False)

    def on_fixed_rebin_edit_changed(self, slot):
        self.rebin_steps_edit.editingFinished.connect(slot)

    def on_variable_rebin_edit_changed(self, slot):
        self.rebin_variable_edit.editingFinished.connect(slot)

    def on_rebin_type_changed(self, slot):
        self.rebin_selector.currentIndexChanged.connect(slot)

    def get_fixed_bin_text(self):
        return self.rebin_steps_edit.text()

    def get_variable_bin_text(self):
        return self.rebin_variable_edit.text()
