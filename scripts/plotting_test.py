# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from PyQt4 import QtGui

import sys

from itertools import cycle

from six import iteritems


from Muon.GUI.Common import message_box


from MultiPlotting.multiPlotting_widget import MultiPlotWidget
from MultiPlotting.multiPlotting_context import *

import mantid.simpleapi as mantid


class plotTestGui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        super(plotTestGui, self).__init__(parent)
        self._context = PlottingContext()
        self.test = MultiPlotWidget(self._context, self)
        ws = setUpSubplot()
        self.test.add_subplot("test",221)
        self.test.add_subplot("bob",222)
        self.test.add_subplot("moo",223)
        self.test.add_subplot("baa",224)
        self.test.plot("test",ws,specNum = 26)
        self.test.plot("bob",ws,specNum = 1)
        self.test.plot("moo",ws,specNum = 42)
        self.test.plot("baa",ws,specNum = 2)
        self.test.set_all_values()
        self.setCentralWidget(self.test)

        self.setWindowTitle("plot test")

def setUpSubplot():
    ws=mantid.Load("MUSR00015089",OutputWorkspace="ws") 
    return ws 

def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
try:
    window = plotTestGui()
    window.show()
    app.exec_()
except RuntimeError as error:
    message_box.warning(str(error))
