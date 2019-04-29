# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import sys

import PyQt4.QtGui as QtGui

from Muon.GUI.FrequencyDomainAnalysis.Transform.transform_widget import TransformWidget
from Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget import FFTWidget
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget import MaxEntWidget
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.Common import message_box


class FrequencyDomainAnalysisGui(QtGui.QMainWindow):
    def __init__(self,parent=None):
        super(FrequencyDomainAnalysisGui,self).__init__(parent)

        load = load_utils.LoadUtils()
        if not load.MuonAnalysisExists:
            return
        self.transform = TransformWidget(load, FFTWidget, MaxEntWidget, parent = self)

        self.setCentralWidget(self.transform.widget)
        self.setWindowTitle("Frequency Domain Analysis")

    # cancel algs if window is closed
    def closeEvent(self,event):
        self.transform.closeEvent(event)


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
try:
    ex= FrequencyDomainAnalysisGui()
    ex.resize(700,700)
    ex.show()
    app.exec_()
except RuntimeError as error:
    message_box.warning(str(error))
