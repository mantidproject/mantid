# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtWidgets, QtCore

from mantidqt.utils.qt import load_ui

Ui_data, _ = load_ui(__file__, "data_widget.ui")


class FittingDataView(QtWidgets.QWidget, Ui_data):
    sig_enable_load_button = QtCore.Signal(bool)

    def __init__(self, parent=None):
        super(FittingDataView, self).__init__(parent)
        self.setupUi(self)

        self.finder_data.setLabelText("Focused Run Files")
        self.finder_data.isForRunFiles(False)

    # =================
    # Slot Connectors
    # =================

    def set_on_load_clicked(self, slot):
        self.button_load.clicked.connect(slot)

    def set_enable_button_connection(self, slot):
        self.sig_enable_load_button.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_load_button_enabled(self, enabled):
        self.button_load.setEnabled(enabled)

    # =================
    # Component Getters
    # =================

    def get_filenames_to_load(self):
        return self.finder_data.getText()

    def get_filenames_valid(self):
        return self.finder_data.isValid()

    def get_add_to_plot(self):
        return self.check_addToPlot.isChecked()

    # =================
    # State Getters
    # =================

    def is_searching(self):
        return self.finder_data.isSearching()
