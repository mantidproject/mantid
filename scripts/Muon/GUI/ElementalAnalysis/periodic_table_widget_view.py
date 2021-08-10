# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from qtpy import QtWidgets
from Muon.GUI.Common import message_box


class PeriodicTableWidgetView(QtWidgets.QWidget):

    @staticmethod
    def warning_popup(message):
        message_box.warning(message)

    def __init__(self, parent=None):
        super(PeriodicTableWidgetView, self).__init__(parent)
        # setup periodic table and peaks
        self.ptable_view = PeriodicTableView()
        self.peaks_view = PeaksView()
        self.widget_list = QtWidgets.QVBoxLayout()
        self.layout = QtWidgets.QHBoxLayout()
        self.setup_widget()

    def setup_widget(self):
        self.layout.addWidget(self.ptable_view)

        self.widget_list.addWidget(self.peaks_view)
        self.layout.addLayout(self.widget_list)
        self.setLayout(self.layout)

    def add_widget_to_view(self, widget: QtWidgets.QWidget):
        self.widget_list.addWidget(widget)

    def add_layout_to_view(self, layout: QtWidgets.QBoxLayout):
        self.widget_list.addLayout(layout)
