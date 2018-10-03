# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
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
        self.peaks.major.on_checkbox_checked(self.major_peaks_checked)
        self.peaks.major.on_checkbox_unchecked(self.major_peaks_unchecked)
        self.peaks.minor.on_checkbox_checked(self.minor_peaks_checked)
        self.peaks.minor.on_checkbox_unchecked(self.minor_peaks_unchecked)
        self.peaks.gamma.on_checkbox_checked(self.gammas_checked)
        self.peaks.gamma.on_checkbox_unchecked(self.gammas_unchecked)
        self.peaks.electron.on_checkbox_checked(self.electrons_checked)
        self.peaks.electron.on_checkbox_unchecked(self.electrons_unchecked)

        self.widget_list.addWidget(self.peaks.view)
        self.widget_list.addWidget(self.detectors.view)
        self.widget_list.addWidget(self.load_widget.view)
        self.plotting = PlotPresenter(PlotView())
        self.plotting.view.setMinimumSize(self.plotting.view.sizeHint())

        self.box = QtGui.QHBoxLayout()
        self.box.addWidget(self.ptable.view)
        self.box.addLayout(self.widget_list)

        self.setCentralWidget(QtGui.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")

        self.element_widgets = {}
        self.element_data = {}
        self.element_lines = {}
        self.gamma_lines = []
        self.gamma_peaks = self.ptable.peak_data["Gammas"]
        self.electron_peaks = self._get_electron_peaks()
        self.electron_lines = []
        self._generate_element_widgets()
        self._generate_element_data()

        self.line_colours = cycle(["r", "g", "b", "c", "m", "y"])

    def iterate_over_selectors(self, check_state, primary_checkboxes=True):
        """
        Iterates over element popups.

        :param check_state: True or False - i.e. check boxes or not
        :param primary_checkboxes: True if Primary, False if Secondary
        """
        for element, selector in iteritems(self.element_widgets):
            for checkbox in selector.primary_checkboxes if primary_checkboxes else selector.secondary_checkboxes:
                checkbox.setChecked(check_state)
            selector.finish_selection()

    def major_peaks_checked(self):
        self.iterate_over_selectors(True, primary_checkboxes=True)

    def major_peaks_unchecked(self):
        self.iterate_over_selectors(False, primary_checkboxes=True)
        self.plotting.update_canvas()

    def minor_peaks_checked(self):
        self.iterate_over_selectors(True, primary_checkboxes=False)

    def minor_peaks_unchecked(self):
        self.iterate_over_selectors(False, primary_checkboxes=False)
        self.plotting.update_canvas()

    def _plot_gammas(self, subplot, colour=None):
        if colour is None:
            colour = self.line_colours.next()
        for element, peaks in iteritems(self.gamma_peaks):
            for peak_type, peak in iteritems(peaks):
                if peak is None:
                    continue
                self.gamma_lines.append(
                    subplot.axvline(
                        peak, 0, 1, color=colour))
        self.plotting.update_canvas()

    def _iterate_over_gamma_selectors(self, check_state):
        for element, selector in iteritems(self.element_widgets):
            for checkbox in selector.gamma_checkboxes:
                checkbox.setChecked(check_state)
            selector.finish_selection()

    def gammas_checked(self):
        self._iterate_over_gamma_selectors(True)

    def gammas_unchecked(self):
        self._iterate_over_gamma_selectors(False)
        for line in self.gamma_lines:
            line.remove()
            del line
        self.gamma_lines = []
        self.plotting.update_canvas()

    def _get_electron_peaks(self):
        return self.ptable.peak_data["Electrons"].copy()

    def _plot_electrons(self, subplot, colour=None):
        if colour is None:
            colour = self.line_colours.next()
        for peak, intensity in iteritems(self.electron_peaks):
            # intensity will be used in the future for labelling lines
            self.electron_lines.append(
                subplot.axvline(
                    float(peak), 0, 1, color=colour))
        self.plotting.update_canvas()

    def electrons_checked(self):
        colour = self.line_colours.next()
        for subplot_name, subplot in iteritems(self.plotting.get_subplots()):
            self._plot_electrons(subplot, colour=colour)

    def electrons_unchecked(self):
        for line in self.electron_lines:
            line.remove()
            del line
        self.electron_lines = []
        self.plotting.update_canvas()

    def load_run(self, detector, run):
        name = "{}; Detector {}".format(run, detector[-1])
        subplot = self.plotting.add_subplot(detector)
        subplot.set_title(detector)
        for plot in mantid.mtd[name]:
            self.plotting.plot(detector, plot)
        if self.plotting.view.isHidden():
            self.plotting.view.show()
        if self.peaks.gamma.isChecked():
            self._plot_gammas(subplot)
        if self.peaks.electron.isChecked():
            self._plot_electrons(subplot)
        self.plotting.update_canvas()

    def load_last_run(self, detector):
        self.load_run(detector, self.load_widget.last_loaded_run())

    def loading_finished(self):
        last_run = self.load_widget.last_loaded_run()
        if last_run is None:
            return
        self.plotting.view.setWindowTitle(str(last_run))
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

    def _add_element_line(self, x_value, element, colour="b"):
        if x_value is None:
            return
        for plot_name in self.plotting.get_subplots():
            line = self.plotting.get_subplot(
                plot_name).axvline(x_value, 0, 1, color=colour)
            try:
                self.element_lines[element][x_value].append(line)
            except KeyError:
                self.element_lines[element][x_value] = [line]
        self.plotting.update_canvas()

    def _add_element_lines(self, element, data):
        self.element_lines[element] = {}
        colour = self.line_colours.next()
        for label, x_value in iteritems(data):
            # label will be used in the future for labelling lines
            self._add_element_line(x_value, element, colour=colour)

    def _remove_element_lines(self, element):
        for x_value, lines in iteritems(self.element_lines[element]):
            for line in lines:
                line.remove()
                del line
        self.plotting.update_canvas()
        self.element_lines[element] = {}

    def _update_element_lines(self, element, current_dict, new_dict):
        # can be split up: this section removes lines
        if len(current_dict) > len(new_dict):  # i.e. item removed
            dict_difference = {k: current_dict[k]
                               for k in set(current_dict) - set(new_dict)}
            for label, x_value in iteritems(dict_difference):
                # label will be used in the future for labelling lines
                for line in self.element_lines[element][x_value]:
                    line.remove()
                    del line
                self.element_lines[element][x_value] = []
                del current_dict[label]
            self.plotting.update_canvas()
        # can be split up: this section adds lines
        elif current_dict != new_dict:  # i.e. item added or not closed without changes
            colour = self.line_colours.next()
            dict_difference = {k: new_dict[k]
                               for k in set(new_dict) - set(current_dict)}
            for label, x_value in iteritems(dict_difference):
                # label will be used in the future for labelling lines
                self._add_element_line(x_value, element, colour)
            current_dict.update(dict_difference)

    def _update_peak_data(self, element, data):
        if self.ptable.is_selected(element):
            self._update_element_lines(
                element, self.element_data[element], data)
        else:
            self.element_data[element] = data.copy()

    def _generate_element_widgets(self):
        self.element_widgets = {}
        for element in self.ptable.peak_data:
            if element in ["Gammas", "Electrons"]:
                continue
            data = self.ptable.element_data(element)
            try:
                data["Gammas"] = self.ptable.peak_data["Gammas"][element]
            except KeyError:
                pass
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

    def _clear_lines(self, lines):
        for line in lines:
            line.remove()
            del line
        return []

    def _clear_lines_after_data_file_selected(self):
        for element in self.element_lines.keys():
            self._remove_element_lines(element)
        self.electron_lines = self._clear_lines(self.electron_lines)
        self.gamma_lines = self._clear_lines(self.gamma_lines)
        for checkbox in self.peaks.peak_checkboxes:
            checkbox.setChecked(False)

    def select_data_file(self):
        filename = str(QtGui.QFileDialog.getOpenFileName())
        if filename:
            self.ptable.set_peak_datafile(filename)
        self._clear_lines_after_data_file_selected()
        self._generate_element_widgets()
        self._generate_element_data()

    def add_plot(self, checkbox):
        detector = checkbox.name
        last_run = self.load_widget.last_loaded_run()
        # not using load_last_run prevents two calls to last_loaded_run()
        if last_run is not None:
            self.load_run(detector, last_run)
        colour = self.line_colours.next()
        for element in self.ptable.selection:
            for label, x_value in iteritems(self.element_data[element.symbol]):
                # label will be used in the future for labelling lines
                line = self.plotting.get_subplot(detector).axvline(
                    x_value, 1, 0, color=colour)
                try:
                    self.element_lines[element.symbol][x_value].append(line)
                except KeyError:
                    self.element_lines[element.symbol][x_value] = [line]
        self.plotting.update_canvas()

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
