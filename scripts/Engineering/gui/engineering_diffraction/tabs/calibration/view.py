# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_calib, _ = load_ui(__file__, "calibration_tab.ui")


class CalibrationView(QtWidgets.QWidget, Ui_calib):
    sig_enable_controls = QtCore.Signal(bool)

    def __init__(self, parent=None, instrument="ENGINX"):
        super(CalibrationView, self).__init__(parent)
        self.setupUi(self)
        self.setup_tabbing_order()
        self.finder_calib.setLabelText("Calibration Sample #")
        self.finder_calib.setInstrumentOverride(instrument)

        self.finder_vanadium.setLabelText("Vanadium #")
        self.finder_vanadium.setInstrumentOverride(instrument)

        self.finder_path.setLabelText("Path")
        self.finder_path.isForRunFiles(False)
        self.finder_path.setEnabled(False)

    # =================
    # Slot Connectors
    # =================

    def set_on_text_changed(self, slot):
        self.finder_vanadium.fileTextChanged.connect(slot)
        self.finder_calib.fileTextChanged.connect(slot)

    def set_on_finding_files_finished(self, slot):
        self.finder_vanadium.fileFindingFinished.connect(slot)
        self.finder_calib.fileFindingFinished.connect(slot)

    def set_on_calibrate_clicked(self, slot):
        self.button_calibrate.clicked.connect(slot)

    def set_on_radio_new_toggled(self, slot):
        self.radio_newCalib.toggled.connect(slot)

    def set_on_radio_existing_toggled(self, slot):
        self.radio_loadCalib.toggled.connect(slot)

    def set_enable_controls_connection(self, slot):
        self.sig_enable_controls.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_calibrate_button_enabled(self, enabled):
        self.button_calibrate.setEnabled(enabled)

    def set_check_plot_output_enabled(self, enabled):
        self.check_plotOutput.setEnabled(enabled)

    def set_instrument_override(self, instrument):
        self.finder_vanadium.setInstrumentOverride(instrument)
        self.finder_calib.setInstrumentOverride(instrument)

    def set_vanadium_enabled(self, set_to):
        self.finder_vanadium.setEnabled(set_to)

    def set_calib_enabled(self, set_to):
        self.finder_calib.setEnabled(set_to)

    def set_path_enabled(self, set_to):
        self.finder_path.setEnabled(set_to)

    def set_vanadium_text(self, text):
        self.finder_vanadium.setText(text)

    def set_calib_text(self, text):
        self.finder_calib.setText(text)

    def set_calibrate_button_text(self, text):
        self.button_calibrate.setText(text)

    # =================
    # Component Getters
    # =================

    def get_vanadium_filename(self):
        return self.finder_vanadium.getFirstFilename()

    def get_vanadium_valid(self):
        return self.finder_vanadium.isValid()

    def get_calib_filename(self):
        return self.finder_calib.getFirstFilename()

    def get_calib_valid(self):
        return self.finder_calib.isValid()

    def get_path_filename(self):
        return self.finder_path.getFirstFilename()

    def get_path_valid(self):
        return self.finder_path.isValid()

    def get_plot_output(self):
        return self.check_plotOutput.isChecked()

    def get_new_checked(self):
        return self.radio_newCalib.isChecked()

    def get_load_checked(self):
        return self.radio_loadCalib.isChecked()

    # =================
    # State Getters
    # =================

    def is_searching(self):
        return self.finder_calib.isSearching() or self.finder_calib.isSearching()

    # =================
    # Force Actions
    # =================

    def find_calib_files(self):
        self.finder_calib.findFiles(True)

    def find_vanadium_files(self):
        self.finder_vanadium.findFiles(True)

    # =================
    # Internal Setup
    # =================

    def setup_tabbing_order(self):
        self.setTabOrder(self.radio_newCalib, self.finder_vanadium)
        self.setTabOrder(self.finder_vanadium, self.finder_calib)
        self.setTabOrder(self.finder_calib, self.check_plotOutput)
        self.setTabOrder(self.check_plotOutput, self.button_calibrate)
