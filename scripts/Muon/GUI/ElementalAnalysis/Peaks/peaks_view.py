from PyQt4 import QtGui

from Muon.GUI.Common.checkbox import Checkbox


class PeaksView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(PeaksView, self).__init__(parent)

        self.list = QtGui.QVBoxLayout()

        self.major = Checkbox("Major Peaks")
        self.minor = Checkbox("Minor Peaks")
        self.gamma = Checkbox("Gamma Peaks")
        self.electron = Checkbox("Electron Peaks")
        for peak_type in [self.major, self.minor, self.gamma, self.electron]:
            self.list.addWidget(peak_type)
        self.setLayout(self.list)
