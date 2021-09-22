# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.MultiPlotting.gridspec_engine import gridspecEngine
from mantidqtinterfaces.MultiPlotting.subplot.subplot_context import subplotContext

import mantid.simpleapi as mantid

xBounds = "xBounds"
yBounds = "yBounds"


class PlottingContext(object):
    def __init__(self, gridspec_engine=gridspecEngine()):
        self.context = {}
        self.subplots = {}
        self.context[xBounds] = [0., 0.]
        self.context[yBounds] = [0., 0.]
        self._gridspec_engine = gridspec_engine
        self._gridspec = None

    def addSubplot(self, name, subplot):
        self.subplots[name] = subplotContext(name, subplot)

    def addLine(self, subplot_name, workspace, spec_num, color=None):
        try:
            ws = None
            if isinstance(workspace, str):
                ws = mantid.AnalysisDataService.retrieve(workspace)
            elif len(workspace) > 1:
                ws = workspace.OutputWorkspace
            else:
                ws = workspace

            if ws is not None:
                self.subplots[subplot_name].addLine(ws, spec_num, color=color)
        except:
            return

    def add_annotate(self, subplot_name, label):
        self.subplots[subplot_name].add_annotate(label)

    def add_vline(self, subplot_name, xvalue, name, color):
        self.subplots[subplot_name].add_vline(xvalue, name, color=color)

    def remove_line(self, subplot_name, name):
        try:
            self.subplots[subplot_name].removeLine(name)
        except KeyError:
            return

    def removePlotLine(self, subplot_name, name):
        try:
            self.subplots[subplot_name].removePlotLine(name)
        except KeyError:
            return

    def removeLabel(self, subplot_name, name):
        try:
            self.subplots[subplot_name].removeLabel(name)
        except KeyError:
            return

    def removeVLine(self, subplot_name, name):
        try:
            self.subplots[subplot_name].removeVLine(name)
        except KeyError:
            return

    def get_xBounds(self):
        return self.context[xBounds]

    def get_yBounds(self):
        return self.context[yBounds]

    def set_xBounds(self, values):
        self.context[xBounds] = values

    def set_yBounds(self, values):
        self.context[yBounds] = values

    def update_gridspec(self, number):
        self._gridspec = self._gridspec_engine.getGridSpec(number)

    @property
    def gridspec(self):
        return self._gridspec

    def update_layout(self, figure):
        keys = list(self.subplots.keys())
        for counter, name in zip(range(len(keys)), keys):
            self.subplots[name].update_gridspec(self._gridspec, figure, counter)

    def delete(self, name):
        self.subplots[name].delete()
        del self.subplots[name]

    def get_lines_from_WS(self, subplot, workspace_name):
        return self.subplots[subplot].get_lines_from_WS_name(workspace_name)

    def is_subplot_empty(self, subplot):
        return self.subplots[subplot].size == 0

    def get_lines(self, subplot_name):
        try:
            return self.subplots[subplot_name].lines
        except KeyError:
            return []
