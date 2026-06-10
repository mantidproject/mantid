# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui
from typing import Callable, Sequence

Ui_calib, _ = load_ui(__file__, "calibration_tab.ui")


class CalibrationView(QtWidgets.QWidget, Ui_calib):
    sig_enable_controls = QtCore.Signal(bool)
    sig_update_sample_field = QtCore.Signal()

    def __init__(self, parent: QtWidgets.QWidget | None = None, instrument: str = "ENGINX"):
        super(CalibrationView, self).__init__(parent)
        self.setupUi(self)
        self.setup_tabbing_order()
        self.finder_sample.setLabelText("Calibration Sample #")
        self.finder_sample.setInstrumentOverride(instrument)

        self.finder_vanadium.setLabelText("Vanadium #")
        self.finder_vanadium.setInstrumentOverride(instrument)
        self.finder_vanadium.allowMultipleFiles(True)

        self.finder_path.setLabelText("Path")
        self.finder_path.isForRunFiles(False)
        self.finder_path.setEnabled(False)
        self.finder_path.setFileExtensions([".prm"])

        self.finder_focus.setLabelText("Sample Run #")
        self.finder_focus.setInstrumentOverride(instrument)
        self.finder_focus.allowMultipleFiles(True)

    # =================
    # Slot Connectors
    # =================

    def set_on_text_changed(self, slot: Callable) -> None:
        self.finder_sample.fileTextChanged.connect(slot)

    def set_on_finding_files_finished(self, slot: Callable) -> None:
        self.finder_sample.fileFindingFinished.connect(slot)

    def set_on_calibrate_clicked(self, slot: Callable) -> None:
        self.button_calibrate.clicked.connect(slot)

    def set_on_radio_new_toggled(self, slot: Callable) -> None:
        self.radio_newCalib.toggled.connect(slot)

    def set_on_radio_existing_toggled(self, slot: Callable) -> None:
        self.radio_loadCalib.toggled.connect(slot)

    def set_enable_controls_connection(self, slot: Callable) -> None:
        self.sig_enable_controls.connect(slot)

    def set_update_field_connection(self, slot: Callable) -> None:
        self.sig_update_sample_field.connect(slot)

    def set_on_check_cropping_state_changed(self, slot: Callable) -> None:
        self.check_roiCalib.stateChanged.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_calibrate_button_enabled(self, enabled: bool) -> None:
        self.button_calibrate.setEnabled(enabled)

    def set_check_plot_output_enabled(self, enabled: bool) -> None:
        self.check_plotOutput.setEnabled(enabled)

    def set_instrument_override(self, instrument: str) -> None:
        self.finder_sample.setInstrumentOverride(instrument)
        self.finder_vanadium.setInstrumentOverride(instrument)
        self.finder_focus.setInstrumentOverride(instrument)

    def set_sample_enabled(self, set_to: bool) -> None:
        self.finder_sample.setEnabled(set_to)

    def set_van_enabled(self, set_to: bool) -> None:
        self.finder_vanadium.setEnabled(set_to)

    def set_path_enabled(self, set_to: bool) -> None:
        self.finder_path.setEnabled(set_to)

    def set_sample_text(self, text: str) -> None:
        self.finder_sample.setText(text)

    def set_calibrate_button_text(self, text: str) -> None:
        self.button_calibrate.setText(text)

    def set_load_checked(self, ticked: bool) -> None:
        self.radio_loadCalib.setChecked(ticked)

    def set_file_text_with_search(self, text: str) -> None:
        self.finder_path.setFileTextWithSearch(text)

    def set_van_file_text_with_search(self, text: str) -> None:
        self.finder_vanadium.setFileTextWithSearch(text)

    def set_cropping_widget_visibility(self, visible: bool) -> None:
        self.widget_cropping.setVisible(visible)

    def set_check_cropping_enabled(self, enabled: bool) -> None:
        self.check_roiCalib.setEnabled(enabled)

    def set_check_cropping_checked(self, checked: bool) -> None:
        self.check_roiCalib.setChecked(checked)

    # =================
    # Component Getters
    # =================

    def get_sample_filename(self) -> str:
        return self.finder_sample.getFirstFilename()

    def get_sample_valid(self) -> bool:
        return self.finder_sample.isValid()

    def get_vanadium_filename(self) -> str:
        return self.finder_vanadium.getFirstFilename()

    def get_vanadium_run(self) -> str:
        return self.finder_vanadium.getText()

    def get_vanadium_valid(self):
        return self.finder_vanadium.isValid()

    def get_path_filename(self) -> str:
        return self.finder_path.getFirstFilename()

    def get_path_valid(self) -> bool:
        return bool(self.finder_path.isValid() and self.finder_path.getText())

    def get_plot_output(self) -> bool:
        return self.check_plotOutput.isChecked()

    def get_new_checked(self) -> bool:
        return self.radio_newCalib.isChecked()

    def get_load_checked(self) -> bool:
        return self.radio_loadCalib.isChecked()

    def get_crop_checked(self) -> bool:
        return self.check_roiCalib.isChecked()

    def get_cropping_widget(self) -> QtWidgets.QWidget:
        return self.widget_cropping

    # =================
    # State Getters
    # =================

    def is_searching(self) -> bool:
        return self.finder_sample.isSearching() or self.finder_vanadium.isSearching() or self.finder_focus.isSearching()

    # =================
    # Force Actions
    # =================

    def find_sample_files(self) -> None:
        self.finder_sample.findFiles(True)

    # =================
    # Internal Setup
    # =================

    def setup_tabbing_order(self) -> None:
        self.finder_sample.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.finder_vanadium.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.finder_path.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)

        self.setTabOrder(self.radio_newCalib, self.finder_sample.focusProxy())
        self.setTabOrder(self.finder_sample.focusProxy(), self.finder_vanadium.focusProxy())
        self.setTabOrder(self.finder_vanadium.focusProxy(), self.radio_loadCalib)
        self.setTabOrder(self.radio_loadCalib, self.finder_path.focusProxy())
        self.setTabOrder(self.finder_path.focusProxy(), self.check_roiCalib)
        self.setTabOrder(self.check_roiCalib, self.widget_cropping)
        self.setTabOrder(self.widget_cropping, self.check_plotOutput)
        self.setTabOrder(self.check_plotOutput, self.button_calibrate)

    # =================
    # Focus Slot Connectors
    # =================

    def set_on_focus_clicked(self, slot: Callable) -> None:
        self.button_focus.clicked.connect(slot)

    # =================
    # Focus Component Setters
    # =================

    def set_focus_button_enabled(self, enabled: bool) -> None:
        self.button_focus.setEnabled(enabled)

    def set_plot_output_enabled(self, enabled: bool) -> None:
        self.check_plotOutput.setEnabled(enabled)

    def set_region_display_text(self, text: str) -> None:
        self.regionDisplay.setText(text)

    # =================
    # Focus Component Getters
    # =================

    def get_focus_filenames(self) -> Sequence[str]:
        return self.finder_focus.getFilenames()

    def get_focus_valid(self) -> bool:
        return self.finder_focus.isValid()

    def get_focus_plot_output(self) -> bool:
        return self.check_focusPlotOutput.isChecked()

    # =================
    # Focus Internal Setup
    # =================

    def set_default_files(self, filepaths: Sequence[str], directory: str | None) -> None:
        if not filepaths:
            return
        self.finder_focus.setUserInput(",".join(filepaths))
        if directory:
            self.set_finder_last_directory(directory)

    def set_finder_last_directory(self, directory: str) -> None:
        self.finder_focus.setLastDirectory(directory)
