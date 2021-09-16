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

        # set text of labels
        self.log_list_label.setText("Check logs to average when loading focused data")
        self.primary_log_label.setText(
            "Sort workspaces by selected log average in sequential fitting (default is ascending order)\n"
            "If the box below is empty the workspaces will be fitted in the order they appear in the table.")
        self.peak_list_label.setText("Default Peak Function")

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

    def set_on_check_ascending_changed(self, slot):
        self.check_ascending.stateChanged.connect(slot)

    def set_on_check_descending_changed(self, slot):
        self.check_descending.stateChanged.connect(slot)

    # =================
    # Component Getters
    # =================

    def get_save_location(self):
        return self.finder_save.getFirstFilename()

    def get_full_calibration(self):
        return self.finder_fullCalib.getFirstFilename()

    def get_checked_logs(self):
        return ','.join([self.log_list.item(ilog).text() for ilog in range(self.log_list.count()) if
                         self.log_list.item(ilog).checkState() == QtCore.Qt.Checked])

    def get_primary_log(self):
        return self.primary_log.currentText()

    def get_ascending_checked(self):
        return self.check_ascending.isChecked()

    def get_peak_function(self):
        return self.peak_list.currentText()

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
        # block signal so as not to reset primary log
        self.log_list.blockSignals(True)
        for log in logs.split(','):
            items = self.log_list.findItems(log, QtCore.Qt.MatchExactly)
            items[0].setCheckState(QtCore.Qt.Checked)
        self.log_list.blockSignals(False)

    def set_primary_log_combobox(self, primary_log):
        checked_logs = self.get_checked_logs().split(',') + ['']
        self.primary_log.clear()
        self.primary_log.addItems(checked_logs)
        if primary_log in checked_logs:
            self.primary_log.setCurrentText(primary_log)
        else:
            self.primary_log.setCurrentText('')

    def set_ascending_checked(self, checked):
        self.check_ascending.setChecked(checked)

    def set_descending_checked(self, checked):
        self.check_descending.setChecked(checked)

    def set_peak_function(self, peak_name):
        self.peak_list.setCurrentText(peak_name)

    def populate_peak_function_list(self, peak_names):
        self.peak_list.addItems(peak_names.split(','))

    # =================
    # Force Actions
    # =================

    def find_full_calibration(self):
        self.finder_fullCalib.findFiles(True)

    def find_save(self):
        self.finder_save.findFiles(True)
