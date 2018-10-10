from __future__ import (absolute_import, division, print_function)
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.ElementalAnalysis.Plotting.edit_windows.remove_plot_window import RemovePlotWindowView
from Muon.GUI.ElementalAnalysis.Plotting.edit_windows.select_subplot import SelectSubplot


class PlotPresenter(object):

    def __init__(self, view):
        self.view = view
        # self.view.setAddConnection(self.add)
        self.view.setRmConnection(self.rm)
        self.view.plotCloseConnection(self.close)
        self.rmWindow = None
        self.selectorWindow = None

    def update_canvas(self):
        """ Redraws the canvas. """
        self.view.canvas.draw()

    def get_subplot(self, name):
        """
        Returns the subplot with the given name.

        :param name: the name of the subplot
        :returns: a matplotlib subplot object
        :raises KeyError: if the subplot name doesn't exist
        """
        return self.view.get_subplot(name)

    def get_subplots(self):
        """ Returns a dictionary of subplots {name: subplot} """
        return self.view.get_subplots()

    def add_subplot(self, name):
        """
        Adds a subplot to the plotting window.

        :param name: the name to assign to the new add_subplot
        :returns: a matplotlib subplot object
        """
        return self.view.add_subplot(name)

    def plot(self, name, workspace):
        """
        Plots a workspace to a subplot.

        :param name: the name of the subplot to plot on
        :param workspace: the workspace to plot
        :raises KeyError: if the subplot name doesn't exist
        """
        self.view.plot(name, workspace)

    def remove_subplot(self, name):
        """ Removes the subplot corresponding to 'name' from the plotting window """
        self.view.remove_subplot(name)

    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass

    def removeSubplotConnection(self, slot):
        self.view.subplotRemovedSignal.connect(slot)

    def close(self):
        if self.selectorWindow is not None:
            self.closeSelectorWindow()
        if self.rmWindow is not None:
            self.closeRmWindow()

    def add(self):
        print("to do")

    def rm(self):
        names = self.view.subplot_names
        # if the remove window is not visable
        if self.rmWindow is not None:
            self.raiseRmWindow()
        # if the selector is not visable
        elif self.selectorWindow is not None:
            self.raiseSelectorWindow()
        # if only one subplot just skip selector
        elif len(names) == 1:
            self.getRmWindow(names[0])
        # if no selector and no remove window -> let user pick which subplot to
        # change
        else:
            self.selectorWindow = self.createSelectWindow(names)
            self.selectorWindow.subplotSelectorSignal.connect(self.getRmWindow)
            self.selectorWindow.closeEventSignal.connect(
                self.closeSelectorWindow)
            self.selectorWindow.setMinimumSize(300, 100)
            self.selectorWindow.show()

    def createSelectWindow(self, names):
        return SelectSubplot(names)

    def raiseRmWindow(self):
        self.rmWindow.raise_()

    def raiseSelectorWindow(self):
        self.selectorWindow.raise_()

    def closeSelectorWindow(self):
        if self.selectorWindow is not None:
            self.selectorWindow.close
            self.selectorWindow = None

    def createRmWindow(self, subplot):
        return RemovePlotWindowView(lines=self.view.line_labels(subplot), subplot=subplot, parent=self)

    def getRmWindow(self, subplot):
        # always close selector after making a selection
        self.closeSelectorWindow()
        # create the remove window
        self.rmWindow = self.createRmWindow(subplot=subplot)
        self.rmWindow.applyRemoveSignal.connect(self.applyRm)
        self.rmWindow.closeEventSignal.connect(self.closeRmWindow)
        self.rmWindow.setMinimumSize(200, 200)
        self.rmWindow.show()

    def applyRm(self, names):
        remove_subplot = True
        # remove the lines from the subplot
        for name in names:
            if self.rmWindow.getState(name):
                line = self.rmWindow.getLine(name)
                # self.view.get_subplot(self.rmWindow.subplot).lines.remove(line)
                self.view.removeLine(self.rmWindow.subplot, line)
            else:
                remove_subplot = False
        # if all of the lines have been removed -> delete subplot
        if remove_subplot:
            # add a signal to this method - so we can catch it
            self.remove_subplot(self.rmWindow.subplot)
        self.update_canvas()
        # if no subplots then close plotting window
        if not self.get_subplots():
            self.closeRmWindow()
            self.view.close()
        else:
            self.closeRmWindow()

    def closeRmWindow(self):
        self.rmWindow.close
        self.rmWindow = None
