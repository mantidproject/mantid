# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtGui
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar


class myToolbar(NavigationToolbar):
    # only display the buttons we need
    toolitems = [tool for tool in NavigationToolbar.toolitems if
                 tool[0] in ("Save", "Pan", "Zoom")]

    def __init__(self, *args, **kwargs):
        super(myToolbar, self).__init__(*args, **kwargs)
        self.layout().takeAt(5)  # or more than 1 if you have more buttons
        pm = QtGui.QPixmap()
        ic = QtGui.QIcon(pm)
        # self.add = self.addAction(ic, "Add line")
        self.rm = self.addAction(ic, "Remove line")
        self.rm_subplot = self.addAction(ic, "Remove subplot")

    def setAddConnection(self, slot):
        self.add.triggered.connect(slot)

    def setRmConnection(self, slot):
        self.rm.triggered.connect(slot)

    def setRmSubplotConnection(self, slot):
        self.rm_subplot.triggered.connect(slot)
