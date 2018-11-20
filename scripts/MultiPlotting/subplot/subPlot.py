from __future__ import (absolute_import, division, print_function)
from six import iteritems

from qtpy import QtWidgets, QtCore
import numpy as np
from matplotlib.figure import Figure
from matplotlib import gridspec
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas

from mantid import plots
from MultiPlotting.navigation_toolbar import myToolbar
from MultiPlotting.multiPlotting_context import *

# use this to manage lines and workspaces directly

# visualises multiple plots
class subPlot(QtWidgets.QWidget):
    quickEditSignal = QtCore.Signal()

    def __init__(self, name, context):
        super(subPlot,self).__init__()
        self._context = context
        self.figure = Figure()
        self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        # update quick edit from tool bar
        self.canvas.mpl_connect("draw_event",self.draw_event_callback)

        grid = QtWidgets.QGridLayout()
        # add toolbar
        self.toolbar = myToolbar(self.canvas, self)
        self.toolbar.update()
        grid.addWidget(self.toolbar, 0, 0)
        # add plot
        self.plotObjects = {}
        grid.addWidget(self.canvas, 1, 0)
        self.setLayout(grid)

    """ this is called when the zoom
    or pan are used. We want to send a 
    signal to update the axis ranges """
    def draw_event_callback(self,event):
        for subplot in self.plotObjects.keys():
            self.emit_subplot_range(subplot)

    # plot a workspace, if a new subplot create it.
    def plot_with_errors(self, subplotName, label, workspace,specNum=1):
        new = False
        if subplotName not in self._context.subplots.keys():
           self.add_subplot(subplotName)
           new = True
        line, cap_lines, bar_lines = plots.plotfunctions.errorbar(self.plotObjects[subplotName],workspace,specNum=specNum)
        # make a tmp plot to get auto generated legend name
        tmp, = plots.plotfunctions.plot(self.plotObjects[subplotName], workspace, specNum=1)
        label = tmp.get_label()
        # remove the tmp line
        tmp.remove()
        del tmp

        if new:
             self.emit_subplot_range(subplotName)

        # collect results
        all_lines = [line]
        all_lines.extend(cap_lines)
        all_lines.extend(bar_lines)
        self._add_plotted_line(subplotName, label, all_lines, workspace)

    # plot a workspace, if a new subplot create it.
    def plot(self, subplotName, label, workspace,specNum=1):
        new = False
        if subplotName not in self._context.subplots.keys():
           self.add_subplot(subplotName)
           new = True
        line, = plots.plotfunctions.plot(self.plotObjects[subplotName],workspace,specNum=specNum)
        if new:
             self.emit_subplot_range(subplotName)
        self._add_plotted_line(subplotName,line.get_label(), [line], workspace)

    def change_errors(self,state, subplotName):
        pass#lines = self._context.subplots[subplotName].lines
        #ws = self._context.subplots[subplotName].ws
        # get the limits before replotting, so they appear unchanged.
        #x, y = plot.get_xlim(), plot.get_ylim()
		#if state:
           
        
        #self._modify_errors_list(name, state)
        ## get a copy of all the workspaces
        #workspaces = copy(self.plot_storage[name].ws)
        ## get the limits before replotting, so they appear unchanged.
        #x, y = plot.get_xlim(), plot.get_ylim()
        ## clear out the old container
        #self.plot_storage[name].delete()
        #for workspace in workspaces:
        #    self.plot(name, workspace)
        #plot.set_xlim(x)
        #plot.set_ylim(y)
        #self._set_bounds(name)  # set AxisChanger bounds again.


    # adds plotted line to context and updates GUI
    def _add_plotted_line(self, subplotName, label, lines, workspace):
        """ Appends plotted lines to the related subplot list. """
        self._context.addLine(subplotName,label, lines, workspace)
        self.canvas.draw()

    def add_subplot(self,subplotName):
     self.plotObjects[subplotName] = self.figure.add_subplot(111)
     self._context.addSubplot(subplotName)

    def emit_subplot_range(self,subplotName):
     self._context.set(xBounds,self.plotObjects[subplotName].get_xlim())
     self._context.set(yBounds,self.plotObjects[subplotName].get_ylim())
     self.quickEditSignal.emit()
 
    def set_plot_x_range(self,subplotName,range):
        self.plotObjects[subplotName].set_xlim(range)
        self.canvas.draw() 

    def set_plot_y_range(self,subplotName,range):
        self.plotObjects[subplotName].set_ylim(range)
        self.canvas.draw()

    def connect_quick_edit_signal(self,slot):
        self.quickEditSignal.connect(slot)

    def disconnect_quick_edit_signal(self,slot):
        self.quickEditSignal.disconnect(slot)