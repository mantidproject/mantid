# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_focus, _ = load_ui(__file__, "focus_tab.ui")


class FocusView(QtWidgets.QWidget, Ui_focus):
    sig_enable_controls = QtCore.Signal(bool)

    def __init__(self, parent=None, instrument="ENGINX"):
        super(FocusView, self).__init__(parent)
        self.setupUi(self)
        self.setup_tabbing_order()

        self.finder_focus.setLabelText("Sample Run #")
        self.finder_focus.setInstrumentOverride(instrument)
        self.finder_focus.allowMultipleFiles(True)

    # =================
    # Slot Connectors
    # =================

    def set_on_focus_clicked(self, slot):
        self.button_focus.clicked.connect(slot)

    def set_enable_controls_connection(self, slot):
        self.sig_enable_controls.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_instrument_override(self, instrument):
        self.finder_focus.setInstrumentOverride(instrument)

    def set_focus_button_enabled(self, enabled):
        self.button_focus.setEnabled(enabled)

    def set_plot_output_enabled(self, enabled):
        self.check_plotOutput.setEnabled(enabled)

    def set_region_display_text(self, text):
        self.regionDisplay.setText(text)

    # =================
    # Component Getters
    # =================

    def get_focus_filenames(self):
        return self.finder_focus.getFilenames()

    def get_focus_valid(self):
        return self.finder_focus.isValid()

    def get_plot_output(self):
        return self.check_plotOutput.isChecked()

    # =================
    # State Getters
    # =================

    def is_searching(self):
        return self.finder_focus.isSearching()

    # =================
    # Internal Setup
    # =================

    def setup_tabbing_order(self):
        self.finder_focus.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setTabOrder(self.finder_focus, self.check_plotOutput)
        self.setTabOrder(self.check_plotOutput, self.button_focus)

    def set_default_files(self, filepaths, directory):
        if not filepaths:
            return
        self.finder_focus.setUserInput(",".join(filepaths))
        if directory:
            self.set_finder_last_directory(directory)

    def set_finder_last_directory(self, directory):
        self.finder_focus.setLastDirectory(directory)
