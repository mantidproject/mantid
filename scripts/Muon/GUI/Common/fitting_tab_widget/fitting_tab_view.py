# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.utilities import table_utils
from Muon.GUI.Common.message_box import warning
from mantidqt.utils.qt import load_ui

ui_fitting_tab, _ = load_ui(__file__, "fitting_tab.ui")


class FittingTabView(QtWidgets.QWidget, ui_fitting_tab):
    def __init__(self, parent=None):
        super(FittingTabView, self).__init__(parent)
        self.setupUi(self)
        self.setup_fit_options_table()

        self.increment_parameter_display_button.clicked.connect(self.increment_display_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_display_combo_box)

    def update_displayed_data_combo_box(self, data_list):
        name = self.parameter_display_combo.currentText()
        self.parameter_display_combo.clear()

        self.parameter_display_combo.addItems(data_list)

        index = self.parameter_display_combo.findText(name)

        if index != -1:
            self.parameter_display_combo.setCurrentIndex(index)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def increment_display_combo_box(self):
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index < count - 1:
            self.parameter_display_combo.setCurrentIndex(index + 1)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def decrement_display_combo_box(self):
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index != 0:
            self.parameter_display_combo.setCurrentIndex(index - 1)
        else:
            self.parameter_display_combo.setCurrentIndex(count - 1)

    def set_slot_for_select_workspaces_to_fit(self, slot):
        self.select_workspaces_to_fit_button.clicked.connect(slot)

    def set_slot_for_display_workspace_changed(self, slot):
        self.parameter_display_combo.currentIndexChanged.connect(slot)

    @property
    def display_workspace(self):
        return str(self.parameter_display_combo.currentText())

    def setup_fit_options_table(self):
        self.fit_options_table.setRowCount(6)
        self.fit_options_table.setColumnCount(2)
        self.fit_options_table.setColumnWidth(0, 300)
        self.fit_options_table.setColumnWidth(1, 300)
        self.fit_options_table.verticalHeader().setVisible(False)
        self.fit_options_table.horizontalHeader().setStretchLastSection(True)
        self.fit_options_table.setHorizontalHeaderLabels(
            ("Property;Value").split(";"))

        table_utils.setRowName(self.fit_options_table, 0, "Minimizer")
        self.minimizer_combo = table_utils.addComboToTable(self.fit_options_table, 0, [])

        table_utils.setRowName(self.fit_options_table, 1, "TF Asymmetry Mode")
        self.tf_asymmetry_mode_checkbox = table_utils.addCheckBoxToTable(
            self.fit_options_table, False, 1)

        table_utils.setRowName(self.fit_options_table, 2, "Plot Difference")
        self.plot_differences_checkbox = table_utils.addCheckBoxToTable(
            self.fit_options_table, True, 2)

        table_utils.setRowName(self.fit_options_table, 3, "Fit To Raw Data")
        self.fit_to_raw_data_checkbox = table_utils.addCheckBoxToTable(
            self.fit_options_table, False, 3)

        table_utils.setRowName(self.fit_options_table, 4, "Show Parameter Errors")
        self.show_parameter_errors_checkbox = table_utils.addCheckBoxToTable(
            self.fit_options_table, True, 4)

        table_utils.setRowName(self.fit_options_table, 5, "Evaluate Function As")
        self.minimizer_combo = table_utils.addComboToTable(self.fit_options_table, 5, ['CentrePoint', 'Histogram'])

