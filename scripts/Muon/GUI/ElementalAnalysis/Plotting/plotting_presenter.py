from __future__ import (absolute_import, division, print_function)
from Muon.GUI.ElementalAnalysis.Plotting.edit_windows.remove_plot_window import RemovePlotWindowView
from Muon.GUI.ElementalAnalysis.Plotting.edit_windows.select_subplot import SelectSubplot


class PlotPresenter(object):
    def __init__(self, view):
        self.view = view
        self.view.setAddConnection(self.add)
        self.view.setRmConnection(self.rm)
        self.view.plotCloseConnection(self.close)
        self.rmWidget = None
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

    def removeSubplotConnection(self,slot):
        self.view.subplotRemovedSignal.connect(slot)

    def close(self):
        if  self.selectorWindow is not None:
            self.closeSelectorWindow()
        if self.rmWidget is not None:
            self.closeRmWindow()
            

    def add(self):
        print("hi")

    def rm(self):
        # if only one subplot just skip selector
        if len(self.view.workspaces.keys()) == 1:
            self.createRmWindow(self.view.workspaces.keys()[0])
        # if no selector and no remove window -> let user pick which subplot to change
        elif self.selectorWindow is None and self.rmWidget is None:
            self.selectorWindow = SelectSubplot(self.view.workspaces.keys())
            self.selectorWindow.subplotSelectorSignal.connect(self.createRmWindow)
            self.selectorWindow.closeEventSignal.connect(self.closeSelectorWindow)
            self.selectorWindow.setMinimumSize(300,100)
            self.selectorWindow.show()
        # if the remove window is not visable
        elif self.selectorWindow is None:
            self.rmWidget.raise_()
        # if the selector is not visable
        else:
            self.selectorWindow.raise_()

    def closeSelectorWindow(self):
        if self.selectorWindow is not None:
            self.selectorWindow.close
            self.selectorWindow = None

    def createRmWindow(self,subplot):
        # always close selector after making a selection
        self.closeSelectorWindow()
        # create the remove window
        self.rmWidget = RemovePlotWindowView(lines=self.view.get_subplot(subplot).lines,subplot=subplot,parent=self)
        self.rmWidget.applyRemoveSignal.connect(self.applyRm)
        self.rmWidget.closeEventSignal.connect(self.closeRmWindow)
        self.rmWidget.setMinimumSize(200,200)
        self.rmWidget.show()

    def applyRm(self, names):
        self.closeRmWindow()
        remove_subplot = True
        # remove the lines from the subplot
        for name in names:
            if self.rmWidget.getState(name):
                 line = self.rmWidget.getLine(name)
                 #self.view.get_subplot(self.rmWidget.subplot).lines.remove(line)
                 self.view.removeLine(self.rmWidget.subplot,line)
            else:
                 remove_subplot = False
        # if all of the lines have been removed -> delete subplot
        if remove_subplot:
             # add a signal to this method - so we can catch it
             self.remove_subplot(self.rmWidget.subplot)
        self.update_canvas()
        # if no subplots then close plotting window
        if not self.get_subplots():
            self.view.close()

    def closeRmWindow(self):
        self.rmWidget.close
        self.rmWidget = None