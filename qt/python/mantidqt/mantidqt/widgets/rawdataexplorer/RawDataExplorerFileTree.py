# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from qtpy.QtWidgets import QTreeView
from qtpy.QtCore import *


class RawDataExplorerFileTree(QTreeView):

    sig_new_current = Signal(QModelIndex)

    def __init__(self, parent=None):
        super(RawDataExplorerFileTree, self).__init__(parent)

    def currentChanged(self, current_index, previous_index):
        self.sig_new_current.emit(current_index)
        super(RawDataExplorerFileTree, self).currentChanged(current_index, previous_index)

    def selectionChanged(self, selected, deselected):
        super(RawDataExplorerFileTree, self).selectionChanged(selected, deselected)
        if len(selected.indexes()) == 0 and len(deselected.indexes()) != 0:
            # checking if something was deselected

            first_row = deselected.indexes()[0].row()
            # if multiple lines have been deselected, then we are clearing the widget, which is ok
            for index in deselected.indexes():
                if index.row() != first_row:
                    return

            # but it is not allowed for the user to deselect a line, so we select it back
            for index in deselected.indexes():
                self.selectionModel().select(index, QItemSelectionModel.Select | QItemSelectionModel.Rows)
