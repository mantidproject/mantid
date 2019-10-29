# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_focus, _ = load_ui(__file__, "focus_tab.ui")


class FocusView(QtWidgets.QWidget, Ui_focus):
    sig_enable_controls = QtCore.Signal(bool)

    def __init__(self, parent=None, instrument="ENGINX"):
        super(FocusView, self).__init__(parent)
        self.setupUi(self)

        self.finder_focus.setLabelText("Sample Run #")
        self.finder_focus.setInstrumentOverride(instrument)

    def set_on_focus_clicked(self, slot):
        self.button_focus.clicked.connect(slot)

    def set_enable_controls_connection(self, slot):
        self.sig_enable_controls.connect(slot)

    def set_instrument_override(self, instrument):
        self.finder_focus.setInstrumentOverride(instrument)

    def set_focus_button_enabled(self, enabled):
        self.button_focus.setEnabled(enabled)

    def set_plot_output_enabled(self, enabled):
        self.check_plotOutput.setEnabled(enabled)

    def get_focus_filename(self):
        return self.finder_focus.getFirstFilename()

    def get_focus_valid(self):
        return self.finder_focus.isValid()

    def is_searching(self):
        return self.finder_focus.isSearching()

    def get_north_bank(self):
        return self.check_northBank.isChecked()

    def get_south_bank(self):
        return self.check_southBank.isChecked()

    def get_plot_output(self):
        return self.check_plotOutput.isChecked()
