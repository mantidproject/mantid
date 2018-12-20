from PyQt4 import QtGui

from Muon.GUI.Common.checkbox import Checkbox


class DetectorsView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(DetectorsView, self).__init__(parent)

        self.list = QtGui.QVBoxLayout()

        self.GE1 = Checkbox("GE1")
        self.GE2 = Checkbox("GE2")
        self.GE3 = Checkbox("GE3")
        self.GE4 = Checkbox("GE4")

        self.list.addWidget(QtGui.QLabel("Detectors"))
        for detector in [self.GE1, self.GE2, self.GE3, self.GE4]:
            self.list.addWidget(detector)
        self.setLayout(self.list)
