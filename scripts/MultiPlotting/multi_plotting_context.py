# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from MultiPlotting.gridspec_engine import gridspecEngine
from copy import deepcopy
from MultiPlotting.subplot.subplot_context import subplotContext

from mantid.api import AnalysisDataServiceObserver
from functools import wraps

import mantid.simpleapi as mantid


xBounds = "xBounds"
yBounds = "yBounds"

def _catch_exceptions(func):
    """
    Catch all exceptions in method and print a traceback to stderr
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception:
            sys.stderr.write("Error occurred in handler:\n")
            import traceback
            traceback.print_exc()

    return wrapper

class ManagerADSObserver(AnalysisDataServiceObserver):

    def __init__(self, context):
        super(ManagerADSObserver, self).__init__()
        self._subplots= context
        self._canvas = None
        print("set ADS")
        #self.observeClear(True)
        self.observeDelete(True)
        #self.observeReplace(True)

    def add_canvas(self,canvas):
        self._canvas = canvas

    @_catch_exceptions
    def deleteHandle(self, _, workspace):
        """
        Called when the ADS has deleted a workspace. Checks the
        attached axes for any hold a plot from this workspace. If removing
        this leaves empty axes then the parent window is triggered for
        closer
        :param _: The name of the workspace. Unused
        :param workspace: A pointer to the workspace
        """
        print("mooooo!!!! "+workspace.name(),self._subplots.keys() )

        for subplot in self._subplots.keys():
             #print(subplot)
             try:
                  print("fdsafd",workspace)
                  mm = self._subplots[subplot].get_ws_by_name(workspace.name())
                  print(mm)
                  labels = self._subplots[subplot].ws[mm]
                  #print(labels,mm)
                  for label in labels:
                      print("hi")
                      # is it even going in the function? Might be better to just throw a signal up ... 
                      tmp = deepcopy(label)
                      self._subplots[subplot].removePlotLine(tmp)
                      self._canvas.draw()
             except:
                  continue

class PlottingContext(object):

    def __init__(self, gridspec_engine=gridspecEngine()):
        self.context = {}
        self.subplots = {}
        self.context[xBounds] = [0., 0.]
        self.context[yBounds] = [0., 0.]
        self._gridspec_engine = gridspec_engine
        self._gridspec = None
        self._ADSObserver = ManagerADSObserver(self.subplots)
        self._canvas = None

    def add_canvas(self,canvas):
        self._canvas = canvas
        self._ADSObserver.add_canvas(self._canvas)

    def addSubplot(self, name, subplot):
        self.subplots[name] = subplotContext(name, subplot)

    def addLine(self, subplotName, workspace, specNum):
        try:
            if isinstance(workspace, str):
                ws = mantid.AnalysisDataService.retrieve(workspace)
                self.subplots[subplotName].addLine(ws, specNum)

            elif len(workspace) > 1:
                self.subplots[subplotName].addLine(
                    workspace.OutputWorkspace, specNum)
            else:
                self.subplots[subplotName].addLine(workspace, specNum)
        except:
            return

    def add_annotate(self, subplotName, label):
        self.subplots[subplotName].add_annotate(label)

    def add_vline(self, subplotName, xvalue, name):
        self.subplots[subplotName].add_vline(xvalue, name)

    def removeLabel(self, subplotName, name):
        try:
            self.subplots[subplotName].removeLabel(name)
        except:
            return

    def removeVLine(self, subplotName, name):
        try:
            self.subplots[subplotName].removeVLine(name)
        except:
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
            self.subplots[name].update_gridspec(
                self._gridspec, figure, counter)

    def delete(self, name):
        self.subplots[name].delete()
        del self.subplots[name]
