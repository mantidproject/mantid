# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui

Ui_cropping, _ = load_ui(__file__, "cropping_widget.ui")


class CroppingView(QtWidgets.QWidget, Ui_cropping):
    def __init__(self, parent):
        super(CroppingView, self).__init__(parent)
        self.setupUi(self)
        self.widget_custom.hide()

    # =================
    # Slot Connectors
    # =================

    def set_on_combo_changed(self, slot):
        self.combo_bank.currentIndexChanged.connect(slot)

    def set_on_custom_spectra_changed(self, slot):
        self.edit_custom.textChanged.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_custom_spectra_entry_hidden(self):
        self.widget_custom.hide()

    def set_custom_spectra_entry_visible(self):
        self.widget_custom.show()

    def set_invalid_indicator_hidden(self):
        self.label_customValid.hide()

    def set_invalid_indicator_visible(self, string):
        self.label_customValid.setToolTip(string)
        self.label_customValid.show()

    # =================
    # Component Getters
    # =================

    def get_combo_value(self):
        return self.combo_bank.currentText()

    def get_custom_spectra_text(self):
        return self.edit_custom.text()
