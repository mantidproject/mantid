# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.message_box import warning
from Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableWidget import SequentialTableWidget

ui_seq_fitting_tab, _ = load_ui(__file__, "seq_fitting_tab.ui")

default_columns = {"Run": 0, "Groups/Pairs": 1, "Fit quality": 2}
default_fit_status = "No fit"


class SeqFittingTabView(QtWidgets.QWidget, ui_seq_fitting_tab):

    def __init__(self, parent=None):
        super(SeqFittingTabView, self).__init__(parent)
        self.setupUi(self)
        self.fit_table = SequentialTableWidget(parent)
        self.tableLayout.addWidget(self.fit_table.widget)
        self.setEnabled(False)

    def warning_popup(self, message):
        warning(message, parent=self)

    def set_slot_for_display_data_type_changed(self, slot):
        self.data_type_combo_box.currentIndexChanged.connect(slot)

    def set_data_type_options(self, data_type_options: list) -> None:
        self.data_type_combo_box.clear()
        self.data_type_combo_box.addItems(data_type_options)

    def hide_data_type_combo_box(self):
        self.filter_by_label.hide()
        self.data_type_combo_box.hide()

    def selected_data_type(self) -> str:
        return self.data_type_combo_box.currentText()

    def use_initial_values_for_fits(self):
        return self.initial_fit_values_radio.isChecked()

    def setup_slot_for_fit_selected_button(self, slot):
        self.fit_selected_button.clicked.connect(slot)

    def setup_slot_for_sequential_fit_button(self, slot):
        self.seq_fit_button.clicked.connect(slot)

    def copy_values_for_fits(self):
        return self.copy_fit_checkbox.isChecked()
