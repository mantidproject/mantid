# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_settings, _ = load_ui(__file__, "settings_widget.ui")


class SettingsView(QtWidgets.QDialog, Ui_settings):
    def __init__(self, parent=None):
        super(SettingsView, self).__init__(parent)
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowContextHelpButtonHint)
        self.setupUi(self)
        self.setModal(True)

        self.finder_save.setLabelText("Save Location")
        self.finder_save.isForRunFiles(False)
        self.finder_save.isForDirectory(True)

        self.finder_fullCalib.setLabelText("Full Calibration")
        self.finder_fullCalib.isForRunFiles(False)

    # ===============
    # Slot Connectors
    # ===============

    def set_on_apply_clicked(self, slot):
        self.btn_apply.clicked.connect(slot)

    def set_on_ok_clicked(self, slot):
        self.btn_ok.clicked.connect(slot)

    def set_on_cancel_clicked(self, slot):
        self.btn_cancel.clicked.connect(slot)

    def set_on_log_changed(self, slot):
        self.log_list.itemChanged.connect(slot)

    # =================
    # Component Getters
    # =================

    def get_save_location(self):
        return self.finder_save.getFirstFilename()

    def get_full_calibration(self):
        return self.finder_fullCalib.getFirstFilename()

    def get_van_recalc(self):
        return self.check_vanRecalc.isChecked()

    def get_checked_logs(self):
        return ','.join([self.log_list.item(ilog).text() for ilog in range(self.log_list.count()) if
                         self.log_list.item(ilog).checkState() == QtCore.Qt.Checked])

    # =================
    # Component Setters
    # =================

    def set_save_location(self, text):
        self.finder_save.setText(text)

    def set_full_calibration(self, text):
        self.finder_fullCalib.setText(text)

    def set_van_recalc(self, checked):
        self.check_vanRecalc.setChecked(checked)

    def add_log_checkboxs(self, logs):
        for log in logs.split(','):
            item = QtWidgets.QListWidgetItem(self.log_list)
            item.setText(log)
            item.setCheckState(QtCore.Qt.Unchecked)
            self.log_list.addItem(item)

    def set_checked_logs(self, logs):
        for log in logs.split(','):
            items = self.log_list.findItems(log, QtCore.Qt.MatchExactly)
            items[0].setCheckState(QtCore.Qt.Checked)

    # =================
    # Force Actions
    # =================

    def find_full_calibration(self):
        self.finder_fullCalib.findFiles(True)

    def find_save(self):
        self.finder_save.findFiles(True)
