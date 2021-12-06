# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Qt, Signal
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.QSequentialTableModel import FIT_STATUS_COLUMN
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableDelegates import FitQualityDelegate
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.QSelectionTableView import QSelectionTableView


class QSequentialTableView(QSelectionTableView):
    keyUpDownPressed = Signal()

    def __init__(self, parent):
        super(QSequentialTableView, self).__init__(parent)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.setItemDelegateForColumn(FIT_STATUS_COLUMN, FitQualityDelegate(self))
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setAlternatingRowColors(True)
