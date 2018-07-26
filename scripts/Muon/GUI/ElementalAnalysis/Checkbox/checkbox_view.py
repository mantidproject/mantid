from functools import partial

from PyQt4 import QtGui, QtCore

from Muon.GUI.ElementalAnalysis.Checkbox.checkbox import Checkbox


class CheckboxView(QtGui.QWidget):
    def __init__(self, names, title=None, parent=None):
        super(CheckboxView, self).__init__(parent)

        self.checkbox_dict = {}

        self.list = QtGui.QVBoxLayout()
        if title is not None:
            t = QtGui.QLabel(title)
            self.list.addWidget(t)
        for name in names:
            checkbox = Checkbox(name)
            self.list.addWidget(checkbox)
            self.checkbox_dict[name] = checkbox
        self.setLayout(self.list)
