# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Any

from qtpy.QtCore import Qt
from qtpy.QtGui import QStandardItem

from mantid.kernel import V3D


class RevertibleItem(QStandardItem):
    """
    Define an item that stores the data it is initialized with
    so that any failure in updating
    """
    def __init__(self, data: Any):
        super().__init__()
        self.is_v3d = isinstance(data, V3D)
        self.orig_data = data if not self.is_v3d else str(data)
        self.reset()

    def reset(self):
        """
        Reset the item with the original data
        """
        self.setData(self.orig_data, Qt.DisplayRole)

    def sync(self):
        """
        Sync the internal data with the display data
        """
        self.orig_data = self.data(Qt.DisplayRole)


def create_table_item(data: Any, editable: bool = False):
    """
    Factory function to create a relevant Qt-model item type
    for the given parameters. For non-editable items a QStandardItem
    can be used with just a string representation of the data. For
    editable items we must store the original data such that if editing
    fails it can be reset.
    :param data: Data to be stored in the item
    :param editable: Boolean indicating if the item can be edited
    """
    if editable:
        return RevertibleItem(data)
    else:
        item = QStandardItem(str(data))
        item.setFlags(item.flags() & ~Qt.ItemIsEditable)
        return item
