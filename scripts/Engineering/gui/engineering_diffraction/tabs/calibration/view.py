# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_calib, _ = load_ui(__file__, "calibration_tab.ui")


class CalibrationView(QtWidgets.QWidget, Ui_calib):
    sig_enable_controls = QtCore.Signal(bool)
    sig_update_sample_field = QtCore.Signal()
    sig_update_van_field = QtCore.Signal()

    def __init__(self, parent=None, instrument="ENGINX"):
        super(CalibrationView, self).__init__(parent)
        self.setupUi(self)
        self.setup_tabbing_order()
        self.finder_sample.setLabelText("Calibration Sample #")
        self.finder_sample.setInstrumentOverride(instrument)

        self.finder_vanadium.setLabelText("Vanadium #")
        self.finder_vanadium.setInstrumentOverride(instrument)

        self.finder_path.setLabelText("Path")
        self.finder_path.isForRunFiles(False)
        self.finder_path.setEnabled(False)
        self.finder_path.setFileExtensions([".prm"])

        self.finder_path_van.setLabelText("Path")
        self.finder_path_van.isForRunFiles(False)
        self.finder_path_van.setEnabled(False)
        self.finder_path_van.setFileExtensions([".nxs"])

    # =================
    # Slot Connectors
    # =================

    def set_on_text_changed(self, slot):
        self.finder_vanadium.fileTextChanged.connect(slot)
        self.finder_sample.fileTextChanged.connect(slot)

    def set_on_finding_files_finished(self, slot):
        self.finder_vanadium.fileFindingFinished.connect(slot)
        self.finder_sample.fileFindingFinished.connect(slot)

    def set_on_calibrate_clicked(self, slot):
        self.button_calibrate.clicked.connect(slot)

    def set_on_radio_new_toggled(self, slot):
        self.radio_newCalib.toggled.connect(slot)

    def set_on_radio_existing_toggled(self, slot):
        self.radio_loadCalib.toggled.connect(slot)

    def set_enable_controls_connection(self, slot):
        self.sig_enable_controls.connect(slot)

    def set_update_sample_field_connection(self, slot):
        self.sig_update_sample_field.connect(slot)

    def set_on_check_cropping_state_changed(self, slot):
        self.check_cropCalib.stateChanged.connect(slot)

    def set_on_create_van_clicked(self, slot):
        self.button_create_v.clicked.connect(slot)

    def set_update_van_field_connection(self, slot):
        self.sig_update_van_field.connect(slot)

    def set_on_radio_new_van_toggled(self, slot):
        self.radio_new_van.toggled.connect(slot)

    def set_on_radio_existing_van_toggled(self, slot):
        self.radio_load_van.toggled.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_calibrate_button_enabled(self, enabled):
        self.button_calibrate.setEnabled(enabled)

    def set_check_plot_output_enabled(self, enabled):
        self.check_plotOutput.setEnabled(enabled)

    def set_instrument_override(self, instrument):
        self.finder_vanadium.setInstrumentOverride(instrument)
        self.finder_sample.setInstrumentOverride(instrument)

    def set_vanadium_enabled(self, set_to):
        self.finder_vanadium.setEnabled(set_to)

    def set_sample_enabled(self, set_to):
        self.finder_sample.setEnabled(set_to)

    def set_calib_path_enabled(self, set_to):
        self.finder_path.setEnabled(set_to)

    def set_vanadium_text(self, text):
        self.finder_vanadium.setText(text)

    def set_sample_text(self, text):
        self.finder_sample.setText(text)

    def set_calibrate_button_text(self, text):
        self.button_calibrate.setText(text)

    def set_load_calib_checked(self, ticked: bool):
        self.radio_loadCalib.setChecked(ticked)

    def set_calib_file_text_with_search(self, text: str):
        self.finder_path.setFileTextWithSearch(text)

    def set_cropping_widget_visibility(self, visible):
        self.widget_cropping.setVisible(visible)

    def set_check_cropping_enabled(self, enabled):
        self.check_cropCalib.setEnabled(enabled)

    def set_check_cropping_checked(self, checked):
        self.check_cropCalib.setChecked(checked)

    def set_create_van_button_enabled(self, enabled):
        self.button_create_v.setEnabled(enabled)

    def set_create_van_button_text(self, text):
        self.button_create_v.setText(text)

    def set_van_path_enabled(self, set_to):
        self.finder_path_van.setEnabled(set_to)

    def set_load_van_checked(self, ticked: bool):
        self.radio_load_van.setChecked(ticked)

    def set_van_file_text_with_search(self, text: str):
        self.finder_path_van.setFileTextWithSearch(text)

    # =================
    # Component Getters
    # =================

    def get_vanadium_filename(self):
        return self.finder_vanadium.getFirstFilename()

    def get_vanadium_valid(self):
        return self.finder_vanadium.isValid()

    def get_sample_filename(self):
        return self.finder_sample.getFirstFilename()

    def get_sample_valid(self):
        return self.finder_sample.isValid()

    def get_path_filename(self):
        return self.finder_path.getFirstFilename()

    def get_calib_path_valid(self):
        return self.finder_path.isValid() and self.finder_path.getText()

    def get_plot_output(self):
        return self.check_plotOutput.isChecked()

    def get_new_calib_checked(self):
        return self.radio_newCalib.isChecked()

    def get_load_calib_checked(self):
        return self.radio_loadCalib.isChecked()

    def get_crop_checked(self):
        return self.check_cropCalib.isChecked()

    def get_cropping_widget(self):
        return self.widget_cropping

    def get_new_van_checked(self):
        return self.radio_new_van.isChecked()

    def get_load_van_checked(self):
        return self.radio_load_van.isChecked()

    def get_van_path_filename(self):
        return self.finder_path_van.getFirstFilename()

    def get_van_path_valid(self):
        return self.finder_path_van.isValid() and self.finder_path_van.getText()

    # =================
    # State Getters
    # =================

    def is_searching(self):
        return self.finder_sample.isSearching() or self.finder_sample.isSearching()

    def is_searching_van(self):
        return self.finder_vanadium.isSearching() or self.finder_vanadium.isSearching()

    # =================
    # Force Actions
    # =================

    def find_sample_files(self):
        self.finder_sample.findFiles(True)

    def find_vanadium_files(self):
        self.finder_vanadium.findFiles(True)

    # =================
    # Internal Setup
    # =================

    def setup_tabbing_order(self):
        self.finder_vanadium.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.finder_sample.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.finder_path.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.finder_path_van.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)

        self.setTabOrder(self.radio_newCalib, self.finder_sample.focusProxy())
        self.setTabOrder(self.finder_sample.focusProxy(), self.radio_loadCalib)
        self.setTabOrder(self.radio_loadCalib, self.finder_path.focusProxy())
        self.setTabOrder(self.finder_path.focusProxy(), self.check_cropCalib)
        self.setTabOrder(self.check_cropCalib, self.widget_cropping)
        self.setTabOrder(self.widget_cropping, self.check_plotOutput)
        self.setTabOrder(self.check_plotOutput, self.button_calibrate)
        self.setTabOrder(self.button_calibrate, self.finder_vanadium.focusProxy())
        self.setTabOrder(self.finder_vanadium.focusProxy(), self.finder_path_van.focusProxy())
        self.setTabOrder(self.finder_path_van.focusProxy(), self.button_create_v)
