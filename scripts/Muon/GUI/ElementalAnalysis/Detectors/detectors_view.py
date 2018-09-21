from PyQt4 import QtGui

from collections import OrderedDict


from Muon.GUI.Common.checkbox import Checkbox


class DetectorsView(QtGui.QWidget):

    def __init__(self, parent=None):
        super(DetectorsView, self).__init__(parent)

        self.list = QtGui.QVBoxLayout()

        self.widgets = OrderedDict()
        labels = ["GE1", "GE2", "GE3", "GE4"]
        for label in labels:
            self.widgets[label] = Checkbox(label)

        self.list.addWidget(QtGui.QLabel("Detectors"))
        for detector in self.widgets.keys():
            self.list.addWidget(self.widgets[detector])
        self.setLayout(self.list)

    def setStateQuietly(self, name, state):
        self.widgets[name].blockSignals(True)
        self.widgets[name].setChecked(state)
        self.widgets[name].blockSignals(False)
