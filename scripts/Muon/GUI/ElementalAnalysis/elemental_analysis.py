# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from qtpy import QtWidgets
from copy import deepcopy
import matplotlib as mpl
from six import iteritems
import sys

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel

from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from MultiPlotting.label import Label

from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel
from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.Common.load_widget.load_presenter import LoadPresenter

from Muon.GUI.ElementalAnalysis.Detectors.detectors_presenter import DetectorsPresenter
from Muon.GUI.ElementalAnalysis.Detectors.detectors_view import DetectorsView
from Muon.GUI.ElementalAnalysis.Peaks.peaks_presenter import PeaksPresenter
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView

from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_presenter import PeakSelectorPresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view import PeakSelectorView

from Muon.GUI.Common import message_box

import mantid.simpleapi as mantid

offset = 0.9


def is_string(value):
    if isinstance(value, str):
        return True
    elif sys.version_info[:2] < (3, 0):
        if isinstance(value, unicode):
            return True

    return False


def gen_name(element, name):
    msg = None
    if not is_string(element):
        msg = "'{}' expected element to be 'str', found '{}' instead".format(str(element), type(element))
    if not is_string(name):
        msg = "'{}' expected name to be 'str', found '{}' instead".format(str(name), type(name))

    if msg is not None:
        raise TypeError(msg)

    if element in name:
        return name
    return element + " " + name


class ElementalAnalysisGui(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super(ElementalAnalysisGui, self).__init__(parent)
        # set menu
        self.menu = self.menuBar()
        self.menu.addAction("File")
        edit_menu = self.menu.addMenu("Edit")
        edit_menu.addAction("Change Peak Data file", self.select_data_file)
        self.menu.addAction("Binning")
        self.menu.addAction("Normalise")

        # periodic table stuff
        self.ptable = PeriodicTablePresenter(PeriodicTableView(), PeriodicTableModel())
        self.ptable.register_table_lclicked(self.table_left_clicked)
        self.ptable.register_table_rclicked(self.table_right_clicked)

        # load stuff
        self.load_widget = LoadPresenter(LoadView(), LoadModel(), CoLoadModel())
        self.load_widget.on_loading_finished(self.loading_finished)

        # detectors
        self.detectors = DetectorsPresenter(DetectorsView())
        for detector in self.detectors.detectors:
            detector.on_checkbox_checked(self.add_plot)
            detector.on_checkbox_unchecked(self.del_plot)

        # peaks boxes
        self.peaks = PeaksPresenter(PeaksView())
        self.peaks.major.setChecked(True)
        self.peaks.major.on_checkbox_checked(self.major_peaks_checked)
        self.peaks.major.on_checkbox_unchecked(self.major_peaks_unchecked)
        self.peaks.minor.on_checkbox_checked(self.minor_peaks_checked)
        self.peaks.minor.on_checkbox_unchecked(self.minor_peaks_unchecked)
        self.peaks.gamma.on_checkbox_checked(self.gammas_checked)
        self.peaks.gamma.on_checkbox_unchecked(self.gammas_unchecked)
        self.peaks.electron.on_checkbox_checked(self.electrons_checked)
        self.peaks.electron.on_checkbox_unchecked(self.electrons_unchecked)

        # set up
        self.widget_list = QtWidgets.QVBoxLayout()
        self.widget_list.addWidget(self.peaks.view)
        self.widget_list.addWidget(self.detectors.view)
        self.widget_list.addWidget(self.load_widget.view)

        # plotting
        self.plot_window = None
        self.num_colors = len(mpl.rcParams['axes.prop_cycle'])
        self.used_colors = {}

        # layout
        self.box = QtWidgets.QHBoxLayout()
        self.box.addWidget(self.ptable.view)
        self.box.addLayout(self.widget_list)

        self.setCentralWidget(QtWidgets.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")

        self.element_widgets = {}
        self.element_lines = {}
        self.electron_peaks = {}
        self._generate_element_widgets()

    def get_color(self, element):
        """
        When requesting the colour for a new element, return the first unused colour of the matplotlib
        default colour cycle (i.e. mpl.rcParams['axes.prop_cycle']).
        If all colours are used, return the first among the least used ones.
        That is if C0-4 are all used twice and C5-9 are used once, C5 will be returned.

        When requesting the colour for an element that is already plotted return the colour of that element.
        This prevents the same element from being displayed in different colours in separate plots

        :param element: Chemical symbol of the element that one wants the colour of
        :return: Matplotlib colour string: C0, C1, ..., C9 to be used as plt.plot(..., color='C3')
        """
        if element in self.used_colors:
            return self.used_colors[element]

        occurrences = [list(self.used_colors.values()).count('C{}'.format(i)) for i in range(self.num_colors)]

        color_index = occurrences.index(min(occurrences))

        color = "C{}".format(color_index)
        self.used_colors[element] = color

        return color

    def closeEvent(self, event):
        if self.plot_window is not None:
            self.plot_window.closeEvent(event)
        super(ElementalAnalysisGui, self).closeEvent(event)

    # general functions
    def _gen_label(self, name, x_value_in, element=None):
        if element is None:
            return
        # check x value is a float
        try:
            x_value = float(x_value_in)
        except:
            return
        if name not in self.element_lines[element]:
            self.element_lines[element].append(name)
        # make sure the names are strings and x values are numbers
        return Label(str(name), x_value, False, offset, True, rotation=-90, protected=True)

    def _plot_line(self, name, x_value_in, color, element=None):
        label = self._gen_label(name, x_value_in, element)
        if self.plot_window is None:
            return
        for subplot in self.plotting.get_subplots():
            self._plot_line_once(subplot, x_value_in, label, color)

    def _plot_line_once(self, subplot, x_value, label, color):
        self.plotting.add_vline_and_annotate(subplot, x_value, label, color)

    def _rm_line(self, name):
        if self.plot_window is None:
            return
        for subplot in self.plotting.get_subplots():
            self.plotting.rm_vline_and_annotate(subplot, name)

    # setup element pop up
    def _generate_element_widgets(self):
        self.element_widgets = {}
        for element in self.ptable.peak_data:
            data = self.ptable.element_data(element)
            widget = PeakSelectorPresenter(PeakSelectorView(data, element))
            widget.on_finished(self._update_peak_data)
            self.element_widgets[element] = widget

    # interact with periodic table
    def table_right_clicked(self, item):
        self.element_widgets[item.symbol].view.show()

    def table_left_clicked(self, item):
        if self.ptable.is_selected(item.symbol):
            self._add_element_lines(item.symbol)
        else:
            self._remove_element_lines(item.symbol)

    def _add_element_lines(self, element, data=None):
        if data is None:
            data = self.element_widgets[element].get_checked()
        if element not in self.element_lines:
            self.element_lines[element] = []

        # Select a different color, if all used then use the first
        color = self.get_color(element)

        for name, x_value in iteritems(data):
            try:
                x_value = float(x_value)
            except ValueError:
                continue
            full_name = gen_name(element, name)
            if full_name not in self.element_lines[element]:
                self.element_lines[element].append(full_name)
            self._plot_line(full_name, x_value, color, element)

    def _remove_element_lines(self, element):
        if element not in self.element_lines:
            return
        names = deepcopy(self.element_lines[element])
        for name in names:
            try:
                self.element_lines[element].remove(name)
                self._rm_line(name)
                if not self.element_lines[element]:
                    del self.element_lines[element]
                    if element in self.used_colors:
                        del self.used_colors[element]
            except:
                continue

        if element in self.element_lines and not self.element_lines[element]:
            del self.element_lines[element]

    # loading
    def load_run(self, detector, run):
        name = "{}; Detector {}".format(run, detector[-1])
        if self.plot_window is None:
            self.plot_window = MultiPlotWindow(str(run))
            self.plotting = self.plot_window.multi_plot
            self.add_detector_to_plot(detector, name)
            self.plotting.set_all_values()
            self.plotting.removeSubplotConnection(self.subplot_removed)
            self.plot_window.show()
            # untick detectors if plot window is closed
            self.plot_window.windowClosedSignal.connect(self._unset_detectors)
        else:
            self.add_detector_to_plot(detector, name)
            self.plot_window.show()
            self.plot_window.raise_()

    def loading_finished(self):
        last_run = self.load_widget.last_loaded_run()
        if last_run is None:
            return
        to_plot = deepcopy([det.isChecked() for det in self.detectors.detectors])
        if self.plot_window is None and any(to_plot) is False:
            return
        # generate plots - if new run clear old plot(s) and replace it
        if self.plot_window is None:
            for name in self.detectors.getNames():
                self.detectors.setStateQuietly(name, False)
        else:
            for plot in self.plotting.get_subplots():
                self.plotting.remove_subplot(plot)
        for j in range(len(to_plot)):
            if to_plot[j]:
                self.detectors.detectors[j].setChecked(True)

    # detectors
    def add_detector_to_plot(self, detector, name):
        self.plotting.add_subplot(detector)
        for ws in mantid.mtd[name]:
            ws.setYUnit('Counts')
            self.plotting.plot(detector, ws.getName())
        # add current selection of lines
        for element in self.ptable.selection:
            self.add_peak_data(element.symbol, detector)

    def _unset_detectors(self):
        self.plot_window.windowClosedSignal.disconnect()
        self.plot_window = None
        for name in self.detectors.getNames():
            self.detectors.setStateQuietly(name, False)

    # plotting
    def add_peak_data(self, element, subplot, data=None):
        # if already selected add to just new plot
        if data is None:
            data = self.element_widgets[element].get_checked()
        color = self.get_color(element)
        for name, x_value in iteritems(data):
            try:
                x_value = float(x_value)
            except ValueError:
                continue
            full_name = gen_name(element, name)
            label = self._gen_label(full_name, x_value, element)
            self._plot_line_once(subplot, x_value, label, color)

    def _update_peak_data(self, element):
        if self.ptable.is_selected(element):
            # remove all of the lines for the element
            self._remove_element_lines(element)
            # add the ticked lines to the plot
            self._add_element_lines(element)
        else:
            self._remove_element_lines(element)

    def add_plot(self, checkbox):
        detector = checkbox.name
        last_run = self.load_widget.last_loaded_run()
        # not using load_last_run prevents two calls to last_loaded_run()
        if last_run is not None:
            self.load_run(detector, last_run)

    def del_plot(self, checkbox):
        if self.load_widget.last_loaded_run() is not None:
            self.plotting.remove_subplot(checkbox.name)
            if not self.plotting.get_subplots() and self.plot_window is not None:
                self.plot_window.close()
                self.plot_window = None

    def subplot_removed(self, name):
        # need to change the state without sending signal
        # as the plot has already been removed
        self.detectors.setStateQuietly(name, False)
        if not self.plotting.get_subplots():
            self.plot_window.close()
            self.plot_window = None

    # sets data file for periodic table
    def select_data_file(self):
        old_lines = deepcopy(list(self.element_lines.keys()))

        filename = QtWidgets.QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.ptable.set_peak_datafile(filename)

        try:
            self._generate_element_widgets()
        except ValueError:
            message_box.warning(
                'The file does not contain correctly formatted data, resetting to default data file.'
                'See "https://docs.mantidproject.org/nightly/interfaces/'
                'Muon%20Elemental%20Analysis.html" for more information.')
            self.ptable.set_peak_datafile(None)
            self._generate_element_widgets()

        for element in old_lines:
            if element in self.element_widgets.keys():
                self.ptable.select_element(element)
            else:
                self._remove_element_lines(element)
        self._update_checked_data()

    def _update_checked_data(self):
        if self.peaks.major.isChecked():
            self.major_peaks_checked()
        else:
            self.major_peaks_unchecked()

        if self.peaks.minor.isChecked():
            self.minor_peaks_checked()
        else:
            self.minor_peaks_unchecked()

        if self.peaks.gamma.isChecked():
            self.gammas_checked()
        else:
            self.gammas_unchecked()

        if self.peaks.electron.isChecked():
            self.electrons_checked()
        else:
            self.electrons_unchecked()

    # general checked data
    def checked_data(self, element, selection, state):
        for checkbox in selection:
            checkbox.setChecked(state)
        self._update_peak_data(element)

    def electrons_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.electron_checkboxes, True)

    def electrons_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.electron_checkboxes, False)

    # gamma Peaks
    def gammas_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.gamma_checkboxes, True)

    def gammas_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.gamma_checkboxes, False)

    # major peaks
    def major_peaks_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.primary_checkboxes, True)

    def major_peaks_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.primary_checkboxes, False)

    # minor peaks
    def minor_peaks_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.secondary_checkboxes, True)

    def minor_peaks_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element, selector.secondary_checkboxes, False)
