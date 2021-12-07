# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView


class PlotDataPaneView(BasePaneView):

    def __init__(self, parent=None):
        super(PlotDataPaneView, self).__init__(parent)
        self._selection_layout = QtWidgets.QHBoxLayout()

        self._selection = QtWidgets.QPushButton("select plot data", self)
        self._selection_layout.addWidget(self._selection)
        self.verticalLayout.insertItem(1, self._selection_layout)

    def set_select_plot_slot(self, slot):
        self._selection.clicked.connect(slot)
