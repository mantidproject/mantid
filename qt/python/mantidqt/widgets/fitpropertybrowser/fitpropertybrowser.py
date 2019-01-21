# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (print_function, absolute_import, unicode_literals)

import re

from qtpy.QtCore import Qt, Signal, Slot

from mantid.simpleapi import mtd, EvaluateFunction
from mantidqt.utils.qt import import_qt
from .interactive_tool import FitInteractiveTool


BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')


class FitPropertyBrowserBase(BaseBrowser):

    def __init__(self, parent=None):
        super(FitPropertyBrowserBase, self).__init__(parent)
        self.init()


class FitPropertyBrowser(FitPropertyBrowserBase):

    closing = Signal()

    def __init__(self, canvas, toolbar_state_checker, parent=None):
        super(FitPropertyBrowser, self).__init__(parent)
        self.init()
        self.canvas = canvas
        self.toolbar_state_checker = toolbar_state_checker
        self.tool = None
        self.fit_result_lines = []
        self.guess_line = None
        self.peak_ids = {}
        self.startXChanged.connect(self.move_start_x)
        self.endXChanged.connect(self.move_end_x)
        self.algorithmFinished.connect(self.fitting_done_slot)
        self.changedParameterOf.connect(self.peak_changed_slot)
        self.removeFitCurves.connect(self.clear_fit_result_lines_slot, Qt.QueuedConnection)
        self.plotGuess.connect(self.plot_guess_slot, Qt.QueuedConnection)
        self.setFeatures(self.DockWidgetMovable)

    def closeEvent(self, event):
        self.closing.emit()
        BaseBrowser.closeEvent(self, event)

    def show(self):
        allowed_spectra = {}
        pattern = re.compile('(.+?): spec (\d+)')
        for label in [lin.get_label() for lin in self.get_lines()]:
            a_match = re.match(pattern, label)
            name, spec = a_match.group(1), int(a_match.group(2))
            spec_list = allowed_spectra.get(name, [])
            spec_list.append(spec)
            allowed_spectra[name] = spec_list
        for name, spec_list in allowed_spectra.items():
            self.addAllowedSpectra(name, spec_list)
        self.tool = FitInteractiveTool(self.canvas, self.toolbar_state_checker,
                                       current_peak_type=self.defaultPeakType())
        self.tool.fit_start_x_moved.connect(self.setStartX)
        self.tool.fit_end_x_moved.connect(self.setEndX)
        self.tool.peak_added.connect(self.peak_added_slot)
        self.tool.peak_moved.connect(self.peak_moved_slot)
        self.tool.peak_fwhm_changed.connect(self.peak_fwhm_changed_slot)
        self.tool.peak_type_changed.connect(self.setDefaultPeakType)
        self.tool.add_background_requested.connect(self.add_function_slot)
        self.tool.add_other_requested.connect(self.add_function_slot)
        self.setXRange(self.tool.fit_start_x.x, self.tool.fit_end_x.x)
        super(FitPropertyBrowser, self).show()
        self.setPeakToolOn(True)
        self.canvas.draw()

    def hide(self):
        if self.tool is not None:
            self.tool.fit_start_x_moved.disconnect()
            self.tool.fit_end_x_moved.disconnect()
            self.tool.disconnect()
            self.tool = None
            self.canvas.draw()
        super(FitPropertyBrowser, self).hide()
        self.setPeakToolOn(False)

    def move_start_x(self, xd):
        if self.tool is not None:
            self.tool.move_start_x(xd)

    def move_end_x(self, xd):
        if self.tool is not None:
            self.tool.move_end_x(xd)

    def clear_fit_result_lines(self):
        for lin in self.fit_result_lines:
            lin.remove()
        self.fit_result_lines = []
        self.update_legend()

    def get_lines(self):
        return self.get_axes().get_lines()

    def get_axes(self):
        return self.canvas.figure.get_axes()[0]

    def update_legend(self):
        axes = self.get_axes()
        if axes.legend_ is not None:
            axes.legend()

    @Slot()
    def clear_fit_result_lines_slot(self):
        self.clear_fit_result_lines()
        if self.tool is not None:
            self.canvas.draw()

    @Slot(str)
    def fitting_done_slot(self, name):
        from workbench.plotting.functions import plot
        ws = mtd[name]
        self.clear_fit_result_lines()
        plot([ws], wksp_indices=[1, 2], fig=self.canvas.figure, overplot=True)
        name += ':'
        for lin in self.get_lines():
            if lin.get_label().startswith(name):
                self.fit_result_lines.append(lin)

    @Slot(int, float, float, float)
    def peak_added_slot(self, peak_id, centre, height, fwhm):
        fun = self.addFunction(self.defaultPeakType())
        self.setPeakCentreOf(fun, centre)
        self.setPeakHeightOf(fun, height)
        self.setPeakFwhmOf(fun, fwhm)
        self.peak_ids[peak_id] = fun

    @Slot(int, float, float)
    def peak_moved_slot(self, peak_id, centre, height):
        fun = self.peak_ids[peak_id]
        self.setPeakCentreOf(fun, centre)
        self.setPeakHeightOf(fun, height)

    @Slot(int, float)
    def peak_fwhm_changed_slot(self, peak_id, fwhm):
        fun = self.peak_ids[peak_id]
        self.setPeakFwhmOf(fun, fwhm)

    @Slot(str)
    def peak_changed_slot(self, fun):
        for peak_id, prefix in self.peak_ids.items():
            if prefix == fun:
                self.tool.update_peak(peak_id, self.getPeakCentreOf(prefix),
                                      self.getPeakHeightOf(prefix),
                                      self.getPeakFwhmOf(prefix))

    @Slot(str)
    def add_function_slot(self, fun_name):
        self.addFunction(fun_name)

    @Slot()
    def show_canvas_context_menu(self):
        if self.tool is not None:
            self.tool.show_context_menu(peak_names=self.registeredPeaks(),
                                        current_peak_type=self.defaultPeakType(),
                                        background_names=self.registeredBackgrounds(),
                                        other_names=self.registeredOthers())

    @Slot()
    def plot_guess_slot(self):
        if self.guess_line is None:
            self.plot_guess()
        else:
            self.remove_guess()

    def plot_guess(self):
        from workbench.plotting.functions import plot
        fun = self.getFittingFunction()
        ws_name = self.workspaceName()
        if fun == '' or ws_name == '':
            return
        ws_index = self.workspaceIndex()
        out_ws_name = '{}guess'.format(ws_name)
        EvaluateFunction(fun, ws_name, WorkspaceIndex=ws_index, OutputWorkspace=out_ws_name)
        plot([mtd[out_ws_name]], wksp_indices=[1], fig=self.canvas.figure, overplot=True)
        out_ws_name += ':'
        for lin in self.get_lines():
            if lin.get_label().startswith(out_ws_name):
                self.guess_line = lin
                self.setTextPlotGuess('Remove Guess')
        self.canvas.draw()

    def remove_guess(self):
        if self.guess_line is None:
            return
        self.guess_line.remove()
        self.guess_line = None
        self.update_legend()
        self.setTextPlotGuess('Plot Guess')
        self.canvas.draw()
