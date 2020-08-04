# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common.checkbox import Checkbox


class PeaksView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(PeaksView, self).__init__(parent)

        self.list = QtWidgets.QVBoxLayout()

        self.major = Checkbox("Major Peaks")
        self.minor = Checkbox("Minor Peaks")
        self.gamma = Checkbox("Gamma Peaks")
        self.electron = Checkbox("Electron Peaks")
        self.deselect_elements = QtWidgets.QPushButton("Deselect All Elements",self)
        self.peak_checkboxes = [self.major, self.minor, self.gamma, self.electron]
        for peak_type in self.peak_checkboxes:
            self.list.addWidget(peak_type)
        self.list.addWidget(self.deselect_elements)
        self.setLayout(self.list)

    def set_deselect_elements_slot(self,slot):
        self.deselect_elements.clicked.connect(slot)

    def enable_deselect_elements_btn(self):
        self.deselect_elements.setEnabled(True)

    def disable_deselect_elements_btn(self):
        self.deselect_elements.setEnabled(False)