# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui

Ui_settings, _ = load_ui(__file__, "settings_widget.ui")


class SettingsView(QtWidgets.QDialog, Ui_settings):
    def __init__(self, parent=None):
        super(SettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setModal(True)

        self.finder_save.setLabelText("Save Location")
        self.finder_save.isForRunFiles(False)
        self.finder_save.isForDirectory(True)

        self.finder_fullCalib.setLabelText("Full Calibration")
        self.finder_fullCalib.isForRunFiles(False)
        # TODO: Once it becomes possible to load the .csv containing the full calibration into mantid,
        #  this can be used. Until then, this option is hidden from the user.
        self.finder_fullCalib.hide()

    # ===============
    # Slot Connectors
    # ===============

    def set_on_apply_clicked(self, slot):
        self.btn_apply.clicked.connect(slot)

    def set_on_ok_clicked(self, slot):
        self.btn_ok.clicked.connect(slot)

    def set_on_cancel_clicked(self, slot):
        self.btn_cancel.clicked.connect(slot)

    # =================
    # Component Getters
    # =================

    def get_save_location(self):
        return self.finder_save.getFirstFilename()

    def get_full_calibration(self):
        return self.finder_fullCalib.getFirstFilename()

    def get_van_recalc(self):
        return self.check_vanRecalc.isChecked()

    # =================
    # Component Setters
    # =================

    def set_save_location(self, text):
        self.finder_save.setText(text)

    def set_full_calibration(self, text):
        self.finder_fullCalib.setText(text)

    def set_van_recalc(self, checked):
        self.check_vanRecalc.setChecked(checked)
