# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from copy import deepcopy

from qtpy import QtWidgets

import mantid.simpleapi as mantid
from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from Muon.GUI.Common.load_widget.load_presenter import LoadPresenter
from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.ElementalAnalysis.Detectors.detectors_presenter import DetectorsPresenter
from Muon.GUI.ElementalAnalysis.Detectors.detectors_view import DetectorsView
from Muon.GUI.ElementalAnalysis.LineSelector.LineSelectorPresenter import LineSelectorPresenter
from Muon.GUI.ElementalAnalysis.LineSelector.LineSelectorView import LineSelectorView
from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel
from Muon.GUI.ElementalAnalysis.LoadWidget.load_utils import spectrum_index
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter import PeriodicTableWidgetPresenter
from Muon.GUI.Common import message_box


class ElementalAnalysisGui(QtWidgets.QMainWindow):
    BLUE = 'C0'
    ORANGE = 'C1'
    GREEN = 'C2'

    @staticmethod
    def warning_popup(message):
        message_box.warning(message)

    def __init__(self, parent=None, window_flags=None):
        super(ElementalAnalysisGui, self).__init__(parent)
        if window_flags:
            self.setWindowFlags(window_flags)

        # load stuff
        self.load_widget = LoadPresenter(LoadView(), LoadModel(), CoLoadModel())
        self.load_widget.on_loading_finished(self.loading_finished)

        # detectors
        self.detectors = DetectorsPresenter(DetectorsView())
        for detector in self.detectors.detectors:
            detector.on_checkbox_checked(self.add_plot)
            detector.on_checkbox_unchecked(self.del_plot)

        # plotting
        self._plot_window = None
        self._plotting = None

        # setup periodic table and peaks
        self.ptable_view = PeriodicTableView()
        self.peaks_view = PeaksView()
        self.ptable = PeriodicTableWidgetPresenter(self)

        # set menu
        self.menu = self.menuBar()
        self.menu.addAction("File")
        edit_menu = self.menu.addMenu("Edit")
        edit_menu.addAction("Change Peak Data file", self.ptable.select_data_file)
        self.menu.addAction("Binning")
        self.menu.addAction("Normalise")

        # Line type boxes
        self.lines = LineSelectorPresenter(LineSelectorView())
        self.lines.total.setChecked(True)
        self.lines.total.on_checkbox_checked(self.line_total_changed)
        self.lines.total.on_checkbox_unchecked(self.line_total_changed)
        self.lines.prompt.on_checkbox_checked(self.line_prompt_changed)
        self.lines.prompt.on_checkbox_unchecked(self.line_prompt_changed)
        self.lines.delayed.on_checkbox_checked(self.line_delayed_changed)
        self.lines.delayed.on_checkbox_unchecked(self.line_delayed_changed)

        # set up
        self.widget_list = QtWidgets.QVBoxLayout()
        self.widget_list.addWidget(self.peaks_view)
        self.widget_list.addWidget(self.lines.view)
        self.widget_list.addWidget(self.detectors.view)
        self.widget_list.addWidget(self.load_widget.view)

        # layout
        self.box = QtWidgets.QHBoxLayout()
        self.box.addWidget(self.ptable_view)
        self.box.addLayout(self.widget_list)

        self.setCentralWidget(QtWidgets.QWidget(self))
        self.centralWidget().setLayout(self.box)
        self.setWindowTitle("Elemental Analysis")

    @property
    def plot_window(self):
        return self._plot_window

    @plot_window.setter
    def plot_window(self, plot_window):
        self._plot_window = plot_window
        self.ptable.plot_window = plot_window

    @property
    def plotting(self):
        return self._plotting

    @plotting.setter
    def plotting(self, plotting):
        self._plotting = plotting
        self.ptable.plotting = plotting

    def closeEvent(self, event):
        if self.plot_window is not None:
            self.plot_window.closeEvent(event)
        super(ElementalAnalysisGui, self).closeEvent(event)

    # loading
    def load_run(self, detector, run):
        name = "{}; Detector {}".format(run, detector[-1])
        if self.plot_window is None:
            self.plot_window = MultiPlotWindow(str(run))
            self.plotting = self.plot_window.multi_plot
            self.add_detector_to_plot(detector, name)
            self.plotting.set_all_values()
            self.plotting.remove_subplot_connection(self.subplot_removed)
            self.plotting.remove_line_connection(self.uncheck_on_removed)
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
        # check all detectors are loaded
        num_detectors = self.load_widget.get_run_num_loaded_detectors(last_run)
        for j, detector in enumerate(self.detectors.getNames()):
            if j < num_detectors:
                self.detectors.enableDetector(detector)
            else:
                # disable checkbox and uncheck box
                self.detectors.disableDetector(detector)
                self.detectors.setStateQuietly(detector, False)

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
        ws = mantid.mtd[name]
        ws.setYUnit('Counts')
        # find correct detector number from the workspace group run
        if self.lines.total.isChecked():
            self.plotting.plot(detector, ws.name(), spec_num=spectrum_index["Total"], color=self.BLUE)
        if self.lines.prompt.isChecked():
            self.plotting.plot(detector, ws.name(), spec_num=spectrum_index["Prompt"], color=self.ORANGE)
        if self.lines.delayed.isChecked():
            self.plotting.plot(detector, ws.name(), spec_num=spectrum_index["Delayed"], color=self.GREEN)

        # add current selection of lines
        for element in self.ptable.ptable.selection:
            self.add_peak_data(element.symbol, detector)

    def _unset_detectors(self):
        self.plot_window.windowClosedSignal.disconnect()
        self.plot_window = None
        for name in self.detectors.getNames():
            self.detectors.setStateQuietly(name, False)
        self.uncheck_detectors_if_no_line_plotted()

    def add_plot(self, checkbox):
        detector = checkbox.name
        last_run = self.load_widget.last_loaded_run()
        # not using load_last_run prevents two calls to last_loaded_run()
        if last_run is not None:
            self.load_run(detector, last_run)
            self.ptable._update_checked_data()

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

    def add_line_by_type(self, run, _type):
        # Ensure all detectors are enabled
        last_run = self.load_widget.last_loaded_run()
        for i, detector in enumerate(self.detectors.detectors):
            if i < self.load_widget.get_run_num_loaded_detectors(last_run):
                if not detector.isEnabled():
                    detector.setEnabled(True)

        if self.plot_window is None:
            return

        # Plot the correct line type on all open subplots
        if _type == 'Total':
            color = self.BLUE
        elif _type == 'Prompt':
            color = self.ORANGE
        else:
            color = self.GREEN
        for subplot in self.plotting.get_subplots():
            name = '{}; Detector {}'.format(run, subplot[-1])
            ws = mantid.mtd[name]
            self.plotting.plot(subplot, ws.name(), spec_num=spectrum_index[_type], color=color)

    def remove_line_type(self, run, _type):
        if self.plot_window is None:
            self.uncheck_detectors_if_no_line_plotted()
            return

        # Remove the correct line type on all open subplots
        for subplot in self.plotting.get_subplots():
            name = '{}; Detector {}'.format(run, subplot[-1])
            ws = mantid.mtd[name]
            self.plotting.remove_line(subplot, ws.name(), spec=spectrum_index[_type])

        # If no line type is selected do not allow plotting
        self.uncheck_detectors_if_no_line_plotted()

    def uncheck_detectors_if_no_line_plotted(self):
        last_run = self.load_widget.last_loaded_run()
        if not any([
            self.lines.total.isChecked(),
            self.lines.prompt.isChecked(),
            self.lines.delayed.isChecked()
        ]):
            for i, detector in enumerate(self.detectors.detectors):
                if i < self.load_widget.get_run_num_loaded_detectors(last_run):
                    detector.setEnabled(False)

    # When removing a line with the remove window uncheck the line here
    def uncheck_on_removed(self, removed_lines):
        if sum([1 if detector.isChecked() else 0 for detector in self.detectors.detectors]) > 1:
            return

        for line in removed_lines:
            if 'Total' in line:
                self.lines.total.blockSignals(True)
                self.lines.total.setChecked(False)
                self.lines.total.blockSignals(False)
            if 'Prompt' in line:
                self.lines.prompt.blockSignals(True)
                self.lines.prompt.setChecked(False)
                self.lines.prompt.blockSignals(False)
            if 'Delayed' in line:
                self.lines.delayed.blockSignals(True)
                self.lines.delayed.setChecked(False)
                self.lines.delayed.blockSignals(False)
        self.uncheck_detectors_if_no_line_plotted()

    # Line total
    def line_total_changed(self, line_total):
        self.lines.total.setChecked(line_total.isChecked())
        if line_total.isChecked():
            self.add_line_by_type(self.load_widget.last_loaded_run(), 'Total')
        else:
            self.remove_line_type(self.load_widget.last_loaded_run(), 'Total')

    # Line prompt
    def line_prompt_changed(self, line_prompt):
        self.lines.prompt.setChecked(line_prompt.isChecked())
        if line_prompt.isChecked():
            self.add_line_by_type(self.load_widget.last_loaded_run(), 'Prompt')
        else:
            self.remove_line_type(self.load_widget.last_loaded_run(), 'Prompt')

    # Line delayed
    def line_delayed_changed(self, line_delayed):
        self.lines.delayed.setChecked(line_delayed.isChecked())
        if line_delayed.isChecked():
            self.add_line_by_type(self.load_widget.last_loaded_run(), 'Delayed')
        else:
            self.remove_line_type(self.load_widget.last_loaded_run(), 'Delayed')
