# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from PyQt4 import QtGui

import sys
from copy import deepcopy

from itertools import cycle

from six import iteritems

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel

from MultiPlotting.multi_plotting_widget import MultiPlotWidget, MultiPlotWindow
from MultiPlotting.multi_plotting_context import *
from MultiPlotting.label import Label

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

offset = 0.9       

def gen_name(element,name):
    if element in name:
       return name
    return element+ " " + name

class ElementalAnalysisGui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(ElementalAnalysisGui, self).__init__(parent)
        self.menu = self.menuBar()
        self.menu.addAction("File")
        edit_menu = self.menu.addMenu("Edit")
        edit_menu.addAction("Change Peak Data file", self.select_data_file)
        self.menu.addAction("Binning")
        self.menu.addAction("Normalise")

        # periodic table stuff
        self.ptable = PeriodicTablePresenter(
            PeriodicTableView(), PeriodicTableModel())
        self.ptable.register_table_lclicked(self.table_left_clicked)
        self.ptable.register_table_rclicked(self.table_right_clicked)


        # load stuff
        self.load_widget = LoadPresenter(
            LoadView(), LoadModel(), CoLoadModel())

        self.load_widget.on_loading_finished(self.loading_finished)



        self.widget_list = QtGui.QVBoxLayout()


        # detectors
        self.detectors = DetectorsPresenter(DetectorsView())
        for detector in self.detectors.detectors:
            detector.on_checkbox_checked(self.add_plot)
            detector.on_checkbox_unchecked(self.del_plot)

        # peaks boxes
        self.peaks = PeaksPresenter(PeaksView())
        self.peaks.major.on_checkbox_checked(self.major_peaks_checked)
        self.peaks.major.on_checkbox_unchecked(self.major_peaks_unchecked)
        self.peaks.minor.on_checkbox_checked(self.minor_peaks_checked)
        self.peaks.minor.on_checkbox_unchecked(self.minor_peaks_unchecked)
        self.peaks.gamma.on_checkbox_checked(self.gammas_checked)
        self.peaks.gamma.on_checkbox_unchecked(self.gammas_unchecked)
        self.peaks.electron.on_checkbox_checked(self.electrons_checked)
        self.peaks.electron.on_checkbox_unchecked(self.electrons_unchecked)


        # set up
        self.widget_list.addWidget(self.peaks.view)
        self.widget_list.addWidget(self.detectors.view)
        self.widget_list.addWidget(self.load_widget.view)


        # plotting
        self.plot_context = PlottingContext()
        self.plotting = MultiPlotWidget(self.plot_context, self)
        self.plot_window = None
        self.plotting.removeSubplotConnection(self.subplotRemoved)


        # layout
        self.box = QtGui.QHBoxLayout()
        self.box.addWidget(self.ptable.view)
        self.box.addLayout(self.widget_list)

        self.setCentralWidget(QtGui.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")

        self.element_widgets = {}
        self.element_lines = {}
        self.electron_peaks = self._get_electron_peaks()
        self._generate_element_widgets()

   # general functions
    def _plot_line(self,name,x_value_in,element = None): 
        if element is None or name in self.element_lines[element]:
            return
        # check x value is a float
        x_value = 0.0
        try:
            x_value = float(x_value_in)
        except:
            return

        self.element_lines[element].append(name)
        # make sure the names are strings and x values are numbers
        label = Label(str(name),float(x_value),False,offset,True,rotation=-90,protected=True)
        for subplot in self.plotting.get_subplots():
            self.plotting.add_vline_and_annotate(subplot, float(x_value), label)

    def _rm_line(self,name):
        for subplot in self.plotting.get_subplots():
            self.plotting.rm_vline_and_annotate(subplot,name)

    #setup element pop up
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

    def _update_peak_data(self,element,data):
        if self.ptable.is_selected(element):
            # remove all of the lines for the element
            self._remove_element_lines(element)
            # add the ticked lines to the plot
            self._add_element_lines(element)
        else:
            self._remove_element_lines(element)

    # interact with periodic table
    def table_right_clicked(self, item):
        self.element_widgets[item.symbol].view.show()

    def table_left_clicked(self, item):
        if self.ptable.is_selected(item.symbol):
            self._add_element_lines(
                item.symbol)
        else:
            self._remove_element_lines(item.symbol)

    def _add_element_lines(self, element, data =None):
        if data is None:
            data = self.element_widgets[element].get_checked()
        if element not in self.element_lines:
            self.element_lines[element] = []
        for name, x_value in iteritems(data):
                full_name = gen_name(element,name)
                self._plot_line(full_name,x_value, element)

    def _remove_element_lines(self,element):
        if element not in self.element_lines:
           return
        names = deepcopy(self.element_lines[element])
        for name in names:
            try:
                self.element_lines[element].remove(name)
                self._rm_line(name)
            except:
                 continue


    # loading
    def load_run(self, detector, run):
        name = "{}; Detector {}".format(run, detector[-1])
        subplot = self.plotting.add_subplot(detector)
        for ws in mantid.mtd[name]:
            self.plotting.plot(detector, ws.getName())
        if self.plot_window is None:
           self.plotting.set_all_values()
           self.plot_window = MultiPlotWindow(self.plotting,str(run))
           # untick detectors if plot window is closed
           self.plot_window.windowClosedSignal.connect(self._unset_detectors)
           self.plot_window.show() 
        else:
            self.plot_window.show()
            self.plot_window.raise_()

    def loading_finished(self):
        last_run = self.load_widget.last_loaded_run()
        if last_run is None:
            return
        for plot in self.plotting.get_subplots():
            self.plotting.remove_subplot(plot)
        for detector in self.detectors.detectors:
            if detector.isChecked():
                self.load_run(detector.name, last_run)
  
  
    # plotting
    def add_plot(self, checkbox):
        detector = checkbox.name
        last_run = self.load_widget.last_loaded_run()
        # not using load_last_run prevents two calls to last_loaded_run()
        if last_run is not None:
            self.load_run(detector, last_run)
            if self.peaks.electron.isChecked():
                self.electrons_checked()

    def del_plot(self, checkbox):
        if self.load_widget.last_loaded_run() is not None:
            self.plotting.remove_subplot(checkbox.name)
            if not self.plotting.get_subplots():
                self.plot_window.hide()

    def _unset_detectors(self):
         self.plot_window.windowClosedSignal.disconnect()
         self.plot_window = None
         for name in self.detectors.getNames():
             self.detectors.setStateQuietly(name,False)       

    def subplotRemoved(self, name):
        # need to change the state without sending signal
        # as the plot has already been removed
        print(name, "subplot rm")
        self.detectors.setStateQuietly(name, False)
        if not self.plotting.get_subplots():
            self.plot_window.hide()

    # sets data file for periodic table
    def select_data_file(self):
        filename = QtGui.QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.ptable.set_peak_datafile(filename)
        self._clear_lines_after_data_file_selected()
        self._generate_element_widgets()
        self._generate_element_data()


    # electron Peaks
    def _get_electron_peaks(self):
        # make a dict so we can label the peaks
        electron_dict = {}
        electron_peaks = self.ptable.peak_data["Electrons"].copy()
        for peak, intensity in iteritems(electron_peaks):
            electron_dict[str(peak) ] = peak
        return electron_dict

    def electrons_checked(self):
        self._add_element_lines("e-",self.electron_peaks)

    def electrons_unchecked(self):
        self._remove_element_lines("e-")

    # general checked data
    def checked_data(self, element, selection, state):
        for checkbox in selection:
            checkbox.setChecked(state)##
        self._update_peak_data(element,self.element_widgets[element].get_checked() )

    # gamma Peaks

    def gammas_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element,selector.gamma_checkboxes,True)

    def gammas_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element,selector.gamma_checkboxes,False)

    #major peaks
    def major_peaks_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element,selector.primary_checkboxes,True)

    def major_peaks_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element,selector.primary_checkboxes,False)

    #minor peaks
    def minor_peaks_checked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element,selector.secondary_checkboxes,True)

    def minor_peaks_unchecked(self):
        for element, selector in iteritems(self.element_widgets):
            self.checked_data(element,selector.secondary_checkboxes,False)


