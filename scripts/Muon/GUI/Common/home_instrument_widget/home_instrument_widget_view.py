# -*- coding: utf8 -*-
from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal as Signal


class InstrumentWidgetView(QtGui.QWidget):
    dataChanged = Signal()

    def __init__(self, parent=None):
        super(InstrumentWidgetView, self).__init__(parent)

        self._button_height = 40

        self.setup_interface()
        self.apply_to_all_hidden(True)
        self.dead_time_file_loader_hidden(True)

        self.deadtime_selector.currentIndexChanged.connect(self.on_dead_time_combo_changed)
        self.rebin_selector.currentIndexChanged.connect(self.on_rebin_combo_changed)
        self.timezero_checkbox.stateChanged.connect(self.on_time_zero_checkbox_state_change)
        self.firstgooddata_checkbox.stateChanged.connect(self.on_first_good_data_checkbox_state_change)

        self._on_dead_time_from_data_selected = None

        self.firstgooddata_checkbox.setChecked(1)
        self.timezero_checkbox.setChecked(1)
        self.time_zero_edit_enabled(True)
        self.first_good_data_edit_enabled(True)

        self._on_time_zero_changed = lambda: 0
        self._on_first_good_data_changed = lambda: 0

        self.timezero_edit.editingFinished.connect(
            lambda: self._on_time_zero_changed() if not self.is_time_zero_checked() else None)
        self.firstgooddata_edit.editingFinished.connect(
            lambda: self._on_first_good_data_changed() if not self.is_first_good_data_checked() else None)

    def on_time_zero_changed(self, slot):
        self._on_time_zero_changed = slot

    def on_first_good_data_changed(self, slot):
        self._on_first_good_data_changed = slot

    def on_dead_time_from_data_selected(self, slot):
        self._on_dead_time_from_data_selected = slot

    def set_time_zero(self, time_zero):
        self.timezero_edit.setText(str(time_zero))

    def set_first_good_data(self, first_good_data):
        self.firstgooddata_edit.setText(str(first_good_data))

    def on_time_zero_checkState_changed(self, slot):
        self.timezero_checkbox.stateChanged.connect(slot)

    def time_zero_state(self):
        return self.timezero_checkbox.checkState()

    def on_first_good_data_checkState_changed(self, slot):
        self.firstgooddata_checkbox.stateChanged.connect(slot)

    def first_good_data_state(self):
        return self.firstgooddata_checkbox.checkState()

    def on_time_zero_checkbox_state_change(self):
        self.time_zero_edit_enabled(self.timezero_checkbox.checkState())

    def on_first_good_data_checkbox_state_change(self):
        self.first_good_data_edit_enabled(self.firstgooddata_checkbox.checkState())

    def time_zero_edit_enabled(self, enabled):
        self.timezero_edit.setEnabled(not enabled)

    def first_good_data_edit_enabled(self, disabled):
        self.firstgooddata_edit.setEnabled(not disabled)

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

    def set_dead_time_label(self, text):
        self.deadtime_label_3.setText(text)

    def on_dead_time_combo_changed(self, index):
        if index == 0:
            self.dead_time_file_loader_hidden(True)
            self.dead_time_data_info_hidden(True)
        if index == 1:
            self._on_dead_time_from_data_selected()
            self.dead_time_file_loader_hidden(True)
            self.dead_time_data_info_hidden(False)
        if index == 2:
            self.dead_time_file_loader_hidden(False)
            self.dead_time_data_info_hidden(True)

    def apply_to_all_hidden(self, hidden=True):
        if hidden:
            self.apply_all_label.hide()
            self.apply_all_checkbox.hide()
        if not hidden:
            self.apply_all_label.setVisible(True)
            self.apply_all_checkbox.setVisible(True)

    def dead_time_file_loader_hidden(self, hidden=True):
        if hidden:
            self.deadtime_file_edit.hide()
            self.deadtime_browse_button.hide()
            self.deadtime_label_2.hide()
        if not hidden:
            self.deadtime_file_edit.setVisible(True)
            self.deadtime_browse_button.setVisible(True)
            self.deadtime_label_2.setVisible(True)

    def dead_time_data_info_hidden(self, hidden=True):
        if hidden:
            self.deadtime_label_3.hide()
        if not hidden:
            self.deadtime_label_3.setVisible(True)

    def setup_instrument_row(self):
        self.instrument_selector = QtGui.QComboBox(self)
        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.instrument_selector.sizePolicy().hasHeightForWidth())
        self.instrument_selector.setSizePolicy(size_policy)
        self.instrument_selector.setMinimumSize(QtCore.QSize(100, self._button_height))
        self.instrument_selector.setObjectName("instrumentSelector")
        self.instrument_selector.addItems(["EMU", "MUSR", "HIFI"])

        self.instrument_label = QtGui.QLabel(self)
        self.instrument_label.setObjectName("instrumentLabel")
        self.instrument_label.setText("Instrument : ")

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.instrument_label)
        self.horizontal_layout.addWidget(self.instrument_selector)
        self.horizontal_layout.addStretch(0)

    def setup_time_zero_row(self):
        self.timezero_label = QtGui.QLabel(self)
        self.timezero_label.setObjectName("timeZeroLabel")
        self.timezero_label.setText("Time Zero : ")

        self.timezero_edit = QtGui.QLineEdit(self)
        reg_ex = QtCore.QRegExp("^[0-9]+([.][0-9]*)?$")
        timezero_validator = QtGui.QRegExpValidator(reg_ex, self.timezero_edit)
        self.timezero_edit.setValidator(timezero_validator)
        self.timezero_edit.setObjectName("timeZeroEdit")
        self.timezero_edit.setText("")


        self.timezero_unit_label = QtGui.QLabel(self)
        self.timezero_unit_label.setObjectName("timeZeroUnitLabel")
        self.timezero_unit_label.setText(u" µs ( ")

        self.timezero_checkbox = QtGui.QCheckBox(self)
        self.timezero_checkbox.setObjectName("timeZeroCheckbox")
        self.timezero_checkbox.setChecked(True)

        self.timezero_label_2 = QtGui.QLabel(self)
        self.timezero_label_2.setObjectName("timeZeroLabel")
        self.timezero_label_2.setText(" From datafile)")

        self.horizontal_layout_2 = QtGui.QHBoxLayout()
        self.horizontal_layout_2.setObjectName("horizontalLayout2")
        self.horizontal_layout_2.addWidget(self.timezero_label)
        self.horizontal_layout_2.addItem(
            QtGui.QSpacerItem(70, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum))
        self.horizontal_layout_2.addWidget(self.timezero_edit)
        self.horizontal_layout_2.addSpacing(10)

        self.horizontal_layout_2.addWidget(self.timezero_unit_label)
        self.horizontal_layout_2.addWidget(self.timezero_checkbox)
        self.horizontal_layout_2.addWidget(self.timezero_label_2)

    def setup_first_good_data_row(self):
        self.firstgooddata_label = QtGui.QLabel(self)
        self.firstgooddata_label.setObjectName("firstgooddataLabel")
        self.firstgooddata_label.setText("First Good Data : ")

        self.firstgooddata_edit = QtGui.QLineEdit(self)
        reg_ex = QtCore.QRegExp("^[0-9]+([.][0-9]*)?$")
        firstgooddata_validator = QtGui.QRegExpValidator(reg_ex, self.timezero_edit)
        self.timezero_edit.setValidator(firstgooddata_validator)
        self.firstgooddata_edit.setObjectName("firstgooddataEdit")
        self.firstgooddata_edit.setText("")

        self.firstgooddata_unit_label = QtGui.QLabel(self)
        self.firstgooddata_unit_label.setObjectName("firstgooddataUnitLabel")
        self.firstgooddata_unit_label.setText(u" µs ( ")  # "micro seconds ( ")

        self.firstgooddata_checkbox = QtGui.QCheckBox(self)
        self.firstgooddata_checkbox.setObjectName("firstgooddataCheckbox")
        self.firstgooddata_checkbox.setChecked(True)

        self.firstgooddata_label_2 = QtGui.QLabel(self)
        self.firstgooddata_label_2.setObjectName("timeZeroLabel")
        self.firstgooddata_label_2.setText(" From datafile)")

        self.horizontal_layout_3 = QtGui.QHBoxLayout()
        self.horizontal_layout_3.setObjectName("horizontalLayout3")
        self.horizontal_layout_3.addWidget(self.firstgooddata_label)
        self.horizontal_layout_3.addItem(
            QtGui.QSpacerItem(15, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum))
        self.horizontal_layout_3.addWidget(self.firstgooddata_edit)
        self.horizontal_layout_3.addSpacing(10)
        self.horizontal_layout_3.addWidget(self.firstgooddata_unit_label)
        self.horizontal_layout_3.addWidget(self.firstgooddata_checkbox)
        self.horizontal_layout_3.addWidget(self.firstgooddata_label_2)

    def setup_dead_time_row(self):
        self.deadtime_label = QtGui.QLabel(self)
        self.deadtime_label.setObjectName("deadTimeLabel")
        self.deadtime_label.setText("Dead Time : ")

        horizontal_spacer = QtGui.QSpacerItem(60, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)

        self.deadtime_selector = QtGui.QComboBox(self)
        self.deadtime_selector.setObjectName("deadTimeSelector")
        self.deadtime_selector.addItems(["None", "From data", "From disk"])
        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.deadtime_selector.sizePolicy().hasHeightForWidth())
        self.deadtime_selector.setSizePolicy(size_policy)
        self.deadtime_selector.setMinimumSize(QtCore.QSize(100, self._button_height))

        self.deadtime_label_2 = QtGui.QLabel(self)
        self.deadtime_label_2.setObjectName("deadTimeFileLabel")
        self.deadtime_label_2.setText("File : ")

        self.deadtime_label_3 = QtGui.QLabel(self)
        self.deadtime_label_3.setObjectName("deadTimeInfoLabel")
        self.deadtime_label_3.setText("")

        self.deadtime_file_edit = QtGui.QLineEdit(self)
        # TODO : Have edit display "No file selected (no dead time applied)" if no file has been chosen
        self.deadtime_file_edit.setObjectName("deadTimeFileEdit")
        self.deadtime_file_edit.setText("")

        self.deadtime_browse_button = QtGui.QPushButton(self)
        self.deadtime_browse_button.setObjectName("deadTimeBrowseButton")
        self.deadtime_browse_button.setText("Browse")

        self.horizontal_layout_4 = QtGui.QHBoxLayout()
        self.horizontal_layout_4.setObjectName("horizontalLayout3")
        self.horizontal_layout_4.addWidget(self.deadtime_label)
        self.horizontal_layout_4.addItem(horizontal_spacer)
        self.horizontal_layout_4.addWidget(self.deadtime_selector)
        self.horizontal_layout_4.addSpacing(10)
        # self.horizontal_layout_4.addItem(horizontal_spacer)
        self.horizontal_layout_4.addWidget(self.deadtime_label_2)
        self.horizontal_layout_4.addWidget(self.deadtime_label_3)
        self.horizontal_layout_4.addWidget(self.deadtime_file_edit)
        self.horizontal_layout_4.addSpacing(10)

        self.horizontal_layout_4.addWidget(self.deadtime_browse_button)
        self.horizontal_layout_4.addItem(
            QtGui.QSpacerItem(100, 40, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum))
        # self.horizontal_layout_4.addStretch(10)

    def setup_rebin_row(self):
        self.rebin_label = QtGui.QLabel(self)
        self.rebin_label.setObjectName("rebinLabel")
        self.rebin_label.setText("Rebin : ")

        self.rebin_selector = QtGui.QComboBox(self)
        self.rebin_selector.setObjectName("rebinSelector")
        self.rebin_selector.addItems(["None", "Fixed", "Variable"])
        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.rebin_selector.sizePolicy().hasHeightForWidth())
        self.rebin_selector.setSizePolicy(size_policy)
        self.rebin_selector.setMinimumSize(QtCore.QSize(100, self._button_height))

        self.rebin_steps_label = QtGui.QLabel(self)
        self.rebin_steps_label.setText("Steps : ")
        self.rebin_steps_edit = QtGui.QLineEdit(self)

        self.rebin_variable_label = QtGui.QLabel(self)
        self.rebin_variable_label.setText("Bin Boundaries : ")
        self.rebin_variable_edit = QtGui.QLineEdit(self)

        self.rebin_help_button = QtGui.QPushButton(self)

        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.rebin_help_button.sizePolicy().hasHeightForWidth())
        self.rebin_help_button.setSizePolicy(size_policy)
        self.rebin_help_button.setMinimumSize(QtCore.QSize(40, 40))
        self.rebin_help_button.setMaximumSize(QtCore.QSize(40, 40))

        self.rebin_help_button.setObjectName("rebinHelpButton")
        self.rebin_help_button.setText("?")

        self.horizontal_layout_5 = QtGui.QHBoxLayout()
        self.horizontal_layout_5.setObjectName("horizontalLayout3")
        self.horizontal_layout_5.addWidget(self.rebin_label)
        self.horizontal_layout_5.addItem(
            QtGui.QSpacerItem(110, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum))
        self.horizontal_layout_5.addWidget(self.rebin_selector)
        self.horizontal_layout_5.addSpacing(10)
        self.horizontal_layout_5.addWidget(self.rebin_steps_label)
        self.horizontal_layout_5.addWidget(self.rebin_steps_edit)
        self.horizontal_layout_5.addSpacing(10)
        self.horizontal_layout_5.addWidget(self.rebin_variable_label)
        self.horizontal_layout_5.addWidget(self.rebin_variable_edit)
        self.rebin_steps_label.hide()
        self.rebin_steps_edit.hide()
        self.rebin_variable_label.hide()
        self.rebin_variable_edit.hide()
        self.horizontal_layout_5.addStretch(0)
        self.horizontal_layout_5.addSpacing(10)
        self.horizontal_layout_5.addWidget(self.rebin_help_button)

    def setup_filter_row(self):
        self.apply_all_label = QtGui.QLabel(self)
        self.apply_all_label.setObjectName("applyAllLabel")
        self.apply_all_label.setText("Apply to all loaded data ")

        self.apply_all_checkbox = QtGui.QCheckBox(self)

        self.horizontal_layout_6 = QtGui.QHBoxLayout()
        self.horizontal_layout_6.setObjectName("horizontalLayout6")
        self.horizontal_layout_6.addWidget(self.apply_all_label)
        self.horizontal_layout_6.addWidget(self.apply_all_checkbox)

    def setup_interface(self):
        self.setObjectName("InstrumentWidget")
        # self.resize(500, 100)

        self.setup_instrument_row()
        self.setup_time_zero_row()
        self.setup_first_good_data_row()
        self.setup_dead_time_row()
        self.setup_rebin_row()
        self.setup_filter_row()

        self.group = QtGui.QGroupBox("Run Pre-processing Parameters")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                           "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: -10px;'
                           'padding-bottom: 0px;'
                           "padding-right: 10px;"
                           ' color: grey; }')

        self.vertical_layout = QtGui.QVBoxLayout()

        self.vertical_layout.addItem(self.horizontal_layout)
        self.vertical_layout.addItem(self.horizontal_layout_2)
        self.vertical_layout.addItem(self.horizontal_layout_3)
        self.vertical_layout.addItem(self.horizontal_layout_4)
        self.vertical_layout.addItem(self.horizontal_layout_5)
        self.vertical_layout.addItem(self.horizontal_layout_6)

        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtGui.QVBoxLayout(self)
        # self.widget_layout.addItem(self.horizontal_layout)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def on_dead_time_browse_clicked(self, slot):
        self.deadtime_browse_button.clicked.connect(slot)

    def show_file_browser_and_return_selection(self, file_filter, search_directories, multiple_files=False):
        default_directory = search_directories[0]
        if multiple_files:
            chosen_files = QtGui.QFileDialog.getOpenFileNames(self, "Select files", default_directory,
                                                              file_filter)
            return [str(chosen_file) for chosen_file in chosen_files]
        else:
            chosen_file = QtGui.QFileDialog.getOpenFileName(self, "Select file", default_directory,
                                                            file_filter)
            return [str(chosen_file)]

    def set_combo_boxes_to_default(self):
        self.rebin_selector.setCurrentIndex(0)
        self.rebin_fixed_hidden(True)
        self.rebin_variable_hidden(True)
        self.deadtime_selector.setCurrentIndex(0)
        self.dead_time_data_info_hidden(True)
        self.dead_time_file_loader_hidden(True)

    def set_checkboxes_to_defualt(self):
        self.timezero_checkbox.setChecked(1)
        self.firstgooddata_checkbox.setChecked(1)

    def on_instrument_changed(self, slot):
        self.instrument_selector.currentIndexChanged.connect(slot)

    def is_time_zero_checked(self):
        return self.timezero_checkbox.checkState()

    def is_first_good_data_checked(self):
        return self.firstgooddata_checkbox.checkState()

    def get_time_zero(self):
        return float(self.timezero_edit.text())

    def get_first_good_data(self):
        return float(self.firstgooddata_edit.text())
