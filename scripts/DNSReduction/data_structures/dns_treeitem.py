# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Custum Tree Item for DNS which is either a Scan or a File in DnsTreeModel
"""
from __future__ import (absolute_import, division, print_function)


class DNSTreeItem(object):
    """
    Custom Tree Item Class for DNS which is either a Scan or a File in DnsTreeModel
    """
    def __init__(self, data, parent=None, checked=0):
        self.parent_item = parent
        self.item_data = data
        self.child_items = []
        self._checkstate = 0
        self.setChecked(checked)

    def clearChilds(self):
        self.child_items = []

    def appendChild(self, item):
        self.child_items.append(item)
        return item

    def child(self, row):
        return self.child_items[row]

    def removeChild(self, row):
        self.child_items.pop(row)

    def childCount(self):
        return len(self.child_items)

    def get_childs(self):
        return self.child_items

    def columnCount(self):
        return len(self.item_data)

    def data(self, column=None):
        if column is not None:
            try:
                return self.item_data[column]
            except IndexError:
                return None
        else:
            return self.item_data

    def hasChildren(self):
        return bool(self.childCount() > 0)

    def isChecked(self):
        return self._checkstate

    def parent(self):
        return self.parent_item

    def row(self):
        if self.parent_item:
            return self.parent_item.child_items.index(self)
        return 0

    def setChecked(self, checked=2):
        self._checkstate = checked

    def setData(self, data, column):
        self.item_data[column] = data
