from __future__ import absolute_import, print_function

from PyQt4 import QtGui

import sys

from itertools import cycle

from six import iteritems

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.ElementalAnalysis.Plotting.plotting_view import PlotView
from Muon.GUI.ElementalAnalysis.Plotting.plotting_presenter import PlotPresenter
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
        self.ptable.register_table_lclicked(self.table_left_clicked)
        self.ptable.register_table_rclicked(self.table_right_clicked)

        self.load_widget = LoadPresenter(
            LoadView(), LoadModel(), CoLoadModel())

        self.load_widget.on_loading_finished(self.loading_finished)

        self.widget_list = QtGui.QVBoxLayout()

        self.detectors = DetectorsPresenter(DetectorsView())
        for detector in self.detectors.detectors:
            detector.on_checkbox_checked(self.add_plot)
            detector.on_checkbox_unchecked(self.del_plot)
        self.peaks = PeaksPresenter(PeaksView())

        self.widget_list.addWidget(self.peaks.view)
        self.widget_list.addWidget(self.detectors.view)
        self.widget_list.addWidget(self.load_widget.view)
        self.plotting = PlotPresenter(PlotView())
        self.plotting.view.setMinimumSize(self.plotting.view.sizeHint())

        self.box = QtGui.QHBoxLayout()
        self.box.addWidget(self.ptable.view)
        self.box.addLayout(self.widget_list)
        self.box.addWidget(self.load_widget.view)

        self.setCentralWidget(QtGui.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")

        self.element_widgets = {}
        self.element_data = {}
        self.element_lines = {}
        self._generate_element_widgets()
        self._generate_element_data()

        self.line_colours = cycle(["r", "g", "b", "c", "m", "y"])

    def load_run(self, detector, run):
        name = "{}; Detector {}".format(run, detector[-1])
        subplot = self.plotting.add_subplot(detector)
        self.plotting.call_plot_method(detector, subplot.set_title, detector)
        for plot in mantid.mtd[name]:
            self.plotting.plot(detector, plot)
        if self.plotting.view.isHidden():
            self.plotting.view.show()

    def load_last_run(self, detector):
        self.load_run(detector, self.load_widget.last_loaded_run())

    def loading_finished(self):
        last_run = self.load_widget.last_loaded_run()
        if last_run is None:
            return
        for plot in self.plotting.get_subplots():
            self.plotting.remove_subplot(plot)
        for detector in self.detectors.detectors:
            if detector.isChecked():
                self.load_run(detector.name, last_run)
        for item in self.ptable.selection:
            self._add_element_lines(
                item.symbol, self.element_data[item.symbol])

    def _generate_element_data(self):
        for element in self.ptable.peak_data:
            if element in ["Gammas", "Electrons"]:
                continue
            try:
                self.element_data[element] = self.ptable.peak_data[element]["Primary"].copy(
                )
            except KeyError:
                continue

    def _add_element_lines(self, element, data):
        self.element_lines[element] = {}
        colour = self.line_colours.next()
        for label, x_value in iteritems(data):
            for plot_name in self.plotting.get_subplots():
                line = self.plotting.add_vline(
                    plot_name, x_value, 0, 1, color=colour)
                try:
                    self.element_lines[element][x_value].append(line)
                except KeyError:
                    self.element_lines[element][x_value] = [line]

    def _remove_element_lines(self, element):
        for x_value, lines in iteritems(self.element_lines[element]):
            for line in lines:
                line.remove()
        self.plotting.update_canvas()
        self.element_lines[element] = {}

    def _update_peak_data(self, element, data):
        self.element_data[element] = data

    def _generate_element_widgets(self):
        self.element_widgets = {}
        for element in self.ptable.peak_data:
            if element in ["Gammas", "Electrons"]:
                continue
            data = self.ptable.element_data(element)
            widget = PeakSelectorPresenter(PeakSelectorView(data, element))
            widget.on_finished(self._update_peak_data)
            self.element_widgets[element] = widget

    def table_left_clicked(self, item):
        if self.ptable.is_selected(item.symbol):
            self._add_element_lines(
                item.symbol, self.element_data[item.symbol])
        else:
            self._remove_element_lines(item.symbol)

    def table_right_clicked(self, item):
        self.element_widgets[item.symbol].view.show()

    def select_data_file(self):
        filename = str(QtGui.QFileDialog.getOpenFileName())
        if filename:
            self.ptable.set_peak_datafile(filename)
        self._generate_element_widgets()
        self._generate_element_data()

    def add_plot(self, checkbox):
        detector = checkbox.name
        last_run = self.load_widget.last_loaded_run()
        # not using load_last_run prevents two calls to last_loaded_run()
        if last_run is not None:
            self.load_run(detector, last_run)

    def del_plot(self, checkbox):
        if self.load_widget.last_loaded_run() is not None:
            self.plotting.remove_subplot(checkbox.name)
            if not self.plotting.get_subplots():
                self.plotting.view.close()


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
