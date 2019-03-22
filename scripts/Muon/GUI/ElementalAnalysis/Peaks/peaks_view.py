# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
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

        self.peak_checkboxes = [
            self.major,
            self.minor,
            self.gamma,
            self.electron]
        for peak_type in self.peak_checkboxes:
            self.list.addWidget(peak_type)
        self.setLayout(self.list)
