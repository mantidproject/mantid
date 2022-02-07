# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets


class MainPlotWidgetView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self._panes = None
        self.setMinimumSize(600,600)
        self.setEnabled(True)
        self._plot_mode = QtWidgets.QComboBox()
        self._layout = QtWidgets.QVBoxLayout()
        self._layout.addWidget(self._plot_mode)
        self.setLayout(self._layout)

    def add_panes(self, panes):
        for name in panes:
            self._layout.addWidget(panes[name].view)
            self._plot_mode.addItem(name)
        self.setLayout(self._layout)

    @property
    def get_plot_mode(self):
        return self._plot_mode.currentText()

    def set_plot_mode(self, index):
        self._plot_mode.setCurrentIndex(index)

    def get_index(self, mode):
        return self._plot_mode.findText(mode)

    def plot_mode_change_connect(self, slot):
        self._plot_mode.currentIndexChanged.connect(slot)
