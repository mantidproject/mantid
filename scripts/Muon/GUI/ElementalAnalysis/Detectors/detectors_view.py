# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from qtpy import QtWidgets

from collections import OrderedDict


from Muon.GUI.Common.checkbox import Checkbox


class DetectorsView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(DetectorsView, self).__init__(parent)

        self.list = QtWidgets.QVBoxLayout()

        self.widgets = OrderedDict()
        labels = ["GE1", "GE2", "GE3", "GE4"]
        for label in labels:
            self.widgets[label] = Checkbox(label)

        self.list.addWidget(QtWidgets.QLabel("Detectors"))
        for detector in self.widgets.keys():
            self.list.addWidget(self.widgets[detector])
        self.setLayout(self.list)

    def setStateQuietly(self, name, state):
        self.widgets[name].blockSignals(True)
        self.widgets[name].setChecked(state)
        self.widgets[name].blockSignals(False)
