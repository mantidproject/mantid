# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView


class PeriodicTableTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(PeriodicTableTabView, self).__init__(parent)
        self.ptable_view = PeriodicTableView(parent=self)
        self.peakview = PeaksView(parent=self)
        self.default_peak_data_label = QtWidgets.QLabel(" Use default peak data")
        self.default_peak_data_checkbox = QtWidgets.QCheckBox()
        self.default_peak_data_checkbox.setChecked(True)
        self.peak_data_file_label = QtWidgets.QLabel()
        self.select_peak_data_file_button = QtWidgets.QPushButton("Select peak data file")
        self.load_peak_data_file_button = QtWidgets.QPushButton("Load")
        self._setup_widget_interface()

    def _setup_widget_interface(self):
        vertical_layout1 = QtWidgets.QVBoxLayout()
        horizontal_layout1 = QtWidgets.QHBoxLayout()
        horizontal_layout2 = QtWidgets.QHBoxLayout()
        vertical_layout2 = QtWidgets.QVBoxLayout()

        horizontal_layout2.addWidget(self.default_peak_data_checkbox, alignment=QtCore.Qt.AlignHCenter)
        horizontal_layout2.addWidget(self.default_peak_data_label, alignment=QtCore.Qt.AlignHCenter)

        vertical_layout2.setAlignment(QtCore.Qt.AlignTop)
        vertical_layout2.addLayout(horizontal_layout2)
        vertical_layout2.addWidget(self.select_peak_data_file_button)
        vertical_layout2.addWidget(self.peak_data_file_label)
        vertical_layout2.addWidget(self.load_peak_data_file_button)

        vertical_layout1.addWidget(self.peakview)
        vertical_layout1.addLayout(vertical_layout2)

        horizontal_layout1.addWidget(self.ptable_view)
        horizontal_layout1.addLayout(vertical_layout1)
        self.setLayout(horizontal_layout1)

        self.set_select_peak_data_file_visible(False)
        self.set_peak_data_file_label_text()

    def change_peak_data_checkbox_quietly(self, state):
        self.default_peak_data_checkbox.blockSignals(True)
        self.default_peak_data_checkbox.setChecked(state)
        self.default_peak_data_checkbox.blockSignals(False)

    def set_select_peak_data_file_visible(self, state):
        self.load_peak_data_file_button.setVisible(state)
        self.peak_data_file_label.setVisible(state)
        self.select_peak_data_file_button.setVisible(state)

    def set_peak_data_file_label_text(self, text=None):
        if text is None:
            self.peak_data_file_label.setVisible(False)
            self.peak_data_file_label.setText("")
            self.load_peak_data_file_button.setEnabled(True)
        else:
            self.peak_data_file_label.setText(text)
            self.peak_data_file_label.setVisible(True)
            self.load_peak_data_file_button.setEnabled(True)
