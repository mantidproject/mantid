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
