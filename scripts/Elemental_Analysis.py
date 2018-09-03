from __future__ import absolute_import, print_function

from PyQt4 import QtGui

import sys

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.Common import message_box
from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel
from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.Common.load_widget.load_presenter import LoadPresenter

from Muon.GUI.ElementalAnalysis.Detectors.detectors_presenter import DetectorsPresenter
from Muon.GUI.ElementalAnalysis.Detectors.detectors_view import DetectorsView
from Muon.GUI.ElementalAnalysis.Peaks.peaks_presenter import PeaksPresenter
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView

from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_presenter import PeakSelectorPresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view import PeakSelectorView


class ElementalAnalysisGui(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(ElementalAnalysisGui, self).__init__(parent)
        self.menu = self.menuBar()
        self.menu.addAction("File")
        edit_menu = self.menu.addMenu("Edit")
        edit_menu.addAction("Change Peak Data file", self.select_data_file)
        self.menu.addAction("Binning")
        self.menu.addAction("Normalise")

        self.ptable = PeriodicTablePresenter(
            PeriodicTableView(), PeriodicTableModel())
        self.ptable.register_table_changed(self.table_changed)
        self.ptable.register_table_lclicked(self.table_left_clicked)
        self.ptable.register_table_rclicked(self.table_right_clicked)

        self.load_widget = LoadPresenter(
            LoadView(), LoadModel(), CoLoadModel())
        self.widget_list = QtGui.QVBoxLayout()

        self.detectors = DetectorsPresenter(DetectorsView())
        self.peaks = PeaksPresenter(PeaksView())

        self.widget_list.addWidget(self.peaks.view)
        self.widget_list.addWidget(self.detectors.view)
        self.widget_list.addWidget(self.load_widget.view)

        self.box = QtGui.QHBoxLayout()
        self.box.addWidget(self.ptable.view)
        self.box.addLayout(self.widget_list)
        self.setCentralWidget(QtGui.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")

        self.element_widgets = {}
        self.element_data = {}
        self._generate_element_widgets()
        self._generate_element_data()

    def _generate_element_data(self):
        for element in self.ptable.peak_data:
            if element in ["Gammas", "Electrons"]:
                continue
            try:
                self.element_data[element] = self.ptable.peak_data[element]["Primary"]
                self.element_data[element].update(
                    self.ptable.peak_data[element]["Secondary"])
            except KeyError:
                continue

    def _update_peak_data(self, element, data):
        self.element_data[element] = data

    def _generate_element_widgets(self):
        self.element_widgets = {}
        for element in self.ptable.peak_data:
            if element in ["Gammas", "Electrons"]:
                continue
            data = self.ptable.element_data(element)
            widget = PeakSelectorPresenter(PeakSelectorView(data, element))
            widget.on_okay_pressed(self._update_peak_data)
            self.element_widgets[element] = widget

    def table_left_clicked(self, item):
        print("Element Left Clicked: {}".format(
            self.element_data[item.symbol]))

    def table_right_clicked(self, item):
        self.element_widgets[item.symbol].view.show()
        print("Element Right Clicked: {}".format(item.symbol))

    def table_changed(self, items):
        print("Table Changed: {}".format([i.symbol for i in items]))

    def select_data_file(self):
        filename = str(QtGui.QFileDialog.getOpenFileName())
        if filename:
            self.ptable.set_peak_datafile(filename)
        self._generate_element_widgets()
        self._generate_element_data()


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
try:
    window = ElementalAnalysisGui()
    window.show()
    app.exec_()
except RuntimeError as error:
    message_box.warning(str(error))
