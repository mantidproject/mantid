from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui


class TransformView(QtGui.QWidget):

    """
    Creates the view for the transformation tab.
    At the top is the transform selection widget
    and below it is the selected GUI (FFT or MaxEnt)
    """

    def __init__(self, selectorView, groupedViews, parent=None):
        super(TransformView, self).__init__(parent)
        # set selector
        self.selection = selectorView
        self.Layout = QtGui.QGridLayout()
        self.Layout.addWidget(self.selection, 1, 0)
        # add the transform widgets to the tab
        self.methods = groupedViews
        for key in self.methods:
            self.Layout.addWidget(self.methods[key])
        self.setLayout(self.Layout)
        self.hideAll()
        methods = list(self.methods.keys())
        self.show(methods[0])

    def getLayout(self):
        return self.grid

    def getMethods(self):
        return [key for key in self.methods]

    def hideAll(self):
        for key in self.methods:
            self.methods[key].hide()

    def show(self, name):
        self.methods[name].show()

    def getView(self, name):
        return self.methods[name]
