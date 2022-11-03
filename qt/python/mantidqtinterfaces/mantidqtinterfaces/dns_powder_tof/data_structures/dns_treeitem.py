# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Custom tree item for DNS which is either a scan or a file in DnsTreeModel.
"""

from enum import Enum


class DNSTreeItem:
    """
    Custom tree item class for DNS, which is either a scan
    or a file in DNSTreeModel.
    """

    def __init__(self, data, parent=None, checked=0):
        self.parent_item = parent
        self.item_data = data
        self.children_items = []
        self._check_state = 0
        self.setChecked(checked)

    def clearChilds(self):
        self.children_items = []

    def appendChild(self, item):
        self.children_items.append(item)
        return item

    def child(self, row):
        return self.children_items[row]

    def removeChild(self, row):
        self.children_items.pop(row)

    def childCount(self):
        return len(self.children_items)

    def get_children_items(self):
        return self.children_items

    def columnCount(self):
        return len(self.item_data)

    def get_tree_item_data(self, column=None):
        if column is not None:
            try:
                return self.item_data[column]
            except IndexError:
                return None
        else:
            return self.item_data

    def get_sample(self):
        if self.hasChildren():  # if it's a scan, get sample from the first datafile
            return self.child(0).get_tree_item_data(TreeItemEnum.sample.value)
        return self.get_tree_item_data(TreeItemEnum.sample.value)

    def get_sample_type(self):
        sample = self.get_sample()
        if 'vanadium' in sample or 'vana' in sample:
            return 'vanadium'
        if 'nicr' in sample or 'NiCr' in sample:
            return 'nicr'
        if 'empty' in sample or 'leer' in sample:
            return 'empty'
        return 'sample'

    def is_type(self, sample_type):
        return sample_type == self.get_sample_type()

    def hasChildren(self):
        return bool(self.childCount() > 0)

    def isChecked(self):
        return self._check_state

    def parent(self):
        return self.parent_item

    def row(self):
        if self.parent_item:
            return self.parent_item.children_items.index(self)
        return 0

    def setChecked(self, checked=2):
        self._check_state = checked

    def setData(self, data, column):
        self.item_data[column] = data


class TreeItemEnum(Enum):
    number = 0
    det_rot = 1
    sample_rot = 2
    field = 3
    temperature = 4
    sample = 5
    time = 6
    tof_channels = 7
    tof_channel_width = 8
    filename = 9
    wavelength = 10
    selector_speed = 11
    scan_number = 12
    scan_command = 13
    scan_points = 14
