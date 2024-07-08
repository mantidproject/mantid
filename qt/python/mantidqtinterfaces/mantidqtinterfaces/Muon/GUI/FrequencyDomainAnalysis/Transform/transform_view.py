# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets


class TransformView(QtWidgets.QWidget):
    """
    Creates the view for the transformation tab.
    At the top is the transform selection widget
    and below it is the selected GUI (FFT or MaxEnt)
    """

    def __init__(self, selectorView, groupedViews, parent=None):
        super(TransformView, self).__init__(parent)
        # set selector
        self.selection = selectorView
        self.Layout = QtWidgets.QGridLayout()
        self.Layout.addWidget(self.selection, 1, 0)
        # add the transform widgets to the tab
        self.methods = groupedViews
        for key in self.methods:
            self.Layout.addWidget(self.methods[key])
        self.setLayout(self.Layout)
        self.hideAll()
        methods = list(self.methods.keys())
        self.showMethod(methods[0])

    def getLayout(self):
        return self.grid

    def getMethods(self):
        return [key for key in self.methods]

    def hideAll(self):
        for key in self.methods:
            self.methods[key].hide()

    def showMethod(self, name):
        self.methods[name].show()

    def getView(self, name):
        return self.methods[name]
