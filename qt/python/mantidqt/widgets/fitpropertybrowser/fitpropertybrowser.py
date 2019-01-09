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

from qtpy.QtCore import Signal

from mantid.simpleapi import mtd
from mantidqt.utils.qt import import_qt
from .interactive_tool import FitInteractiveTool


BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')


class FitPropertyBrowserBase(BaseBrowser):

    def __init__(self, parent=None):
        super(FitPropertyBrowserBase, self).__init__(parent)
        self.init()


class FitPropertyBrowser(FitPropertyBrowserBase):

    closing = Signal()

    def __init__(self, canvas, parent=None):
        super(FitPropertyBrowser, self).__init__(parent)
        self.init()
        self.canvas = canvas
        self.tool = None
        self.fit_result_lines = []
        self.startXChanged.connect(self.move_start_x)
        self.endXChanged.connect(self.move_end_x)
        self.algorithmFinished.connect(self.fitting_done)

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
        self.tool = FitInteractiveTool(self.canvas)
        self.tool.fit_start_x_moved.connect(self.setStartX)
        self.tool.fit_end_x_moved.connect(self.setEndX)
        self.setXRange(self.tool.fit_start_x.x, self.tool.fit_end_x.x)
        super(FitPropertyBrowser, self).show()
        self.canvas.draw()

    def hide(self):
        if self.tool is not None:
            self.tool.fit_start_x_moved.disconnect()
            self.tool.fit_end_x_moved.disconnect()
            self.tool.disconnect()
            self.tool = None
            self.canvas.draw()
        super(FitPropertyBrowser, self).hide()

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

    def get_lines(self):
        return self.canvas.figure.get_axes()[0].get_lines()

    def fitting_done(self, name):
        from workbench.plotting.functions import plot
        ws = mtd[name]
        self.clear_fit_result_lines()
        plot([ws], wksp_indices=[1, 2], fig=self.canvas.figure, overplot=True)
        name += ':'
        for lin in self.get_lines():
            if lin.get_label().startswith(name):
                self.fit_result_lines.append(lin)
