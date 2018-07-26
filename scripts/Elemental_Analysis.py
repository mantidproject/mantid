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
from Muon.GUI.ElementalAnalysis.LoadWidget.load_presenter import LoadPresenter
from Muon.GUI.ElementalAnalysis.Detectors.detectors_presenter import DetectorsPresenter
from Muon.GUI.ElementalAnalysis.Detectors.detectors_view import DetectorsView
from Muon.GUI.ElementalAnalysis.Peaks.peaks_presenter import PeaksPresenter
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView


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

        self.peak_types = ["Major", "Minor", "Gamma", "Electron"]
        self._checkbox_view = CheckboxView(
            ["{} Peaks".format(x) for x in self.peak_types])
        self.checkbox = CheckboxPresenter(self._checkbox_view, CheckboxModel())
        for _, v in self.checkbox.view.checkbox_dict.iteritems():
            v.on_checkbox_unchecked(lambda c: print("unchecked: {}".format(c.name)))
            v.on_checkbox_checked(lambda c: print("checked: {}".format(c.name)))

        self.widget_list.addWidget(self.checkbox.view)

        self.detectors = ["GE{}".format(x) for x in range(1, 5)]
        self.detector_view = CheckboxView(self.detectors, "Detectors:")
        self.detectors_widget = CheckboxPresenter(
            self.detector_view, CheckboxModel())
        self.widget_list.addWidget(self.detectors_widget.view)

        self.widget_list.addWidget(self.load_widget.view)

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

    def table_left_clicked(self, item):
        print("Element Left Clicked: {}".format(
            self.ptable.element_data(item.symbol)))

    def table_right_clicked(self, item):
        print("Element Right Clicked: {}".format(item.symbol))

    def table_changed(self, items):
        print("Table Changed: {}".format([i.symbol for i in items]))

    def select_data_file(self):
        filename = str(QtGui.QFileDialog.getOpenFileName())
        if filename:
            self.ptable.set_peak_datafile(filename)


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
