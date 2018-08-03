from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore
from PyQt4 import QtGui


class DockView(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(DockView, self).__init__(parent)
        self.widgets = []
        self.docks = []

    def addDock(self, widget, name):
        # add widget
        self.widgets.append(widget)
        # make an empty dock for widget
        self.docks.append(QtGui.QDockWidget(name))
        # add widget to dock
        self.docks[-1].setWidget(self.widgets[-1])
        # add dock to view
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, self.docks[-1])
        # set layout
        self.widgets[-1].setLayout(self.widgets[-1].getLayout())

    def makeTabs(self):
        # convert docks into tabs
        self.tabifyDockWidget(self.docks[0], self.docks[1])
        for j in range(2, len(self.docks), 1):
            self.tabifyDockWidget(self.docks[0], self.docks[j])
        # put tabs on the top of the page
        self.setTabPosition(
            QtCore.Qt.LeftDockWidgetArea,
            QtGui.QTabWidget.North)
        # open to first tab
        self.docks[0].show()
        self.docks[0].raise_()

    def keepDocksOpen(self):
        for j in range(0, len(self.docks), 1):
            self.docks[j].setFeatures(
                QtGui.QDockWidget.DockWidgetClosable and QtGui.QDockWidget.DockWidgetFloatable)

    def closeEvent(self, event):
        for j in range(len(self.docks) - 1, -1, -1):
            self.docks[j].close()
            self.widgets[j].close()
