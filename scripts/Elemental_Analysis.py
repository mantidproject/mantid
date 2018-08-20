from __future__ import absolute_import, print_function

from PyQt4 import QtGui

import sys
import random
from time import time

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.ElementalAnalysis.Plotting.plotting_view import PlotView
from Muon.GUI.ElementalAnalysis.Plotting.plotting_presenter import PlotPresenter
from Muon.GUI.Common import message_box
from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel
from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.Common.load_widget.load_presenter import LoadPresenter

import mantid.simpleapi as mantid


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
        self.plotting = PlotPresenter(PlotView())
        self.plotting.view.setFixedSize(self.plotting.view.sizeHint())

        self.add = QtGui.QPushButton("Add")
        self.add.clicked.connect(self.add_plot)

        self.rem = QtGui.QPushButton("Del")
        self.rem.clicked.connect(self.del_plot)

        self.box = QtGui.QHBoxLayout()
        self.box.addWidget(self.ptable.view)
        self.box.addWidget(self.load_widget.view)
        self.box.addWidget(self.add)
        self.box.addWidget(self.rem)
        # layout.addWidget(self.plot_view)
        self.setCentralWidget(QtGui.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")
        self.plotting.view.show()

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

    def add_plot(self):
        name = "Plot {}".format(time())
        p = self.plotting.add_subplot(name)
        p.set_title(name)
        self.plotting.update_canvas()
        c = mantid.CreateSampleWorkspace(OutputWorkspace=name)
        self.plotting.plot(name, c)

    def del_plot(self):
        to_del = random.choice(self.plotting.get_subplots().keys())
        self.plotting.remove_subplot(to_del)


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
