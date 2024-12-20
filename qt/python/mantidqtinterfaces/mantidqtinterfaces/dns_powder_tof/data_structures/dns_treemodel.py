# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Custom tree model for DNS to store list of scans with files as children.
"""

import numpy as np
from qtpy.QtCore import QAbstractItemModel, QModelIndex, Qt
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_treeitem import DNSTreeItem, TreeItemEnum
import decimal


decimal.getcontext().rounding = decimal.ROUND_HALF_UP


class DNSTreeModel(QAbstractItemModel):
    # pylint: disable=too-many-public-methods   # redefinition of QT methods
    """
    QT Model to store DNS scan structure consisting of scans with files as
    children.
    """

    def __init__(self, data=None, parent=None):
        super().__init__(parent)
        self._scan = None
        # this as a QT type argument of QAbstractItemModel therefore not root_item
        self.rootItem = DNSTreeItem(
            # display only relevant (first 10) values
            tuple([column.name for column in TreeItemEnum if column.value < 10])
        )
        self._last_scan_number = None
        self._last_tof_time = None
        self._last_tof = None
        self._last_sample = None
        if data is not None:
            self.setup_model_data(data)

    def index(self, row, column, parent=None):  # overrides QT function
        if not self.hasIndex(row, column, parent):
            return QModelIndex()
        parent_item = self._get_or_create_parent_item(parent)
        child_item = parent_item.child(row)
        if child_item:
            return self.createIndex(row, column, child_item)
        return QModelIndex()

    def parent(self, index):  # overrides QT function
        if not index.isValid():
            return QModelIndex()
        child_item = index.internalPointer()
        # determine the parent of child_item
        # using QT's parent() method
        parent_item = child_item.parent()
        if parent_item == self.rootItem:
            return QModelIndex()
        return self.createIndex(parent_item.row(), 0, parent_item)

    def headerData(self, section, orientation, role):  # overrides QT function
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self.rootItem.get_tree_item_data(section)
        return None

    def columnCount(self, parent):  # overrides QT function
        """
        Returns number of columns.
        """
        if parent.isValid():
            return parent.internalPointer().columnCount()
        return self.rootItem.columnCount()

    def _scan_command_from_row(self, row):
        return self.rootItem.child(row).get_tree_item_data(TreeItemEnum.number.value)

    def _scan_expected_points_from_row(self, row):
        """
        Returns the number of the scan points which are expected from the scan
        command can be smaller than number of children if scan did not run
        completely.
        """
        scan_command = self._scan_command_from_row(row)
        scan_command = scan_command.split("#")[1]
        if "/" in scan_command:
            scan_command = scan_command.split("/")[1]
        if scan_command:
            return int(scan_command.strip())
        return 0

    def number_of_scans(self):
        return self.rootItem.childCount()

    def _scan_range(self):
        return range(self.number_of_scans())

    @staticmethod
    def _get_sample_type(sample):
        if "vanadium" in sample or "vana" in sample:
            return "vana"
        if "nicr" in sample or "NiCr" in sample:
            return "nicr"
        if "empty" in sample or "leer" in sample:
            return "empty"
        return sample

    # complex getting
    def _get_scan_rows(self):
        return [row for row in self._scan_range() if "scan" in self._scan_command_from_row(row)]

    def _is_scan_complete(self, row):
        return self.scan_from_row(row).childCount() >= self._scan_expected_points_from_row(row)

    def text_in_scan(self, row, text):
        return text in self._scan_command_from_row(row)

    def _get_last_row(self):
        return self.number_of_scans() - 1

    def data(self, index, role):  # overrides QT function
        """
        Returns either data or check state of items.
        """
        item = self._item_from_index(index)
        parent_item = item.hasChildren()
        if role == Qt.DisplayRole:
            return item.get_tree_item_data(index.column())
        if role == Qt.TextAlignmentRole and not parent_item:  # center align child items
            return Qt.AlignHCenter
        if role == Qt.CheckStateRole and index.column() == 0:
            if item.isChecked() == 2:
                return Qt.Checked
            if item.isChecked() == 1:
                return Qt.PartiallyChecked
            return Qt.Unchecked
        return None

    def flags(self, index):  # pylint: disable=no-self-use
        # overrides QT function
        if not index.isValid():
            return Qt.NoItemFlags
        if index.column() == 0:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsUserCheckable
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def _scan_index_from_row(self, row):
        return self.index(row, 0, QModelIndex())

    def _index_from_scan(self, scan):
        return self._scan_index_from_row(scan.row())

    @staticmethod
    def _item_from_index(index):
        return index.internalPointer()

    def get_filename_from_index(self, index):
        item = self._item_from_index(index)
        if item is not None and not item.hasChildren():
            return item.get_tree_item_data(TreeItemEnum.filename.value)
        return ""

    def _index_from_row(self, row, parent):
        return self.index(row, 0, parent)

    def scan_from_row(self, row):
        return self.rootItem.child(row)

    def rowCount(self, parent=None):  # overrides QT function
        parent_item = self._get_or_create_parent_item(parent)
        if parent is not None and parent.column() > 0:
            return 0
        return parent_item.childCount()

    def _get_or_create_parent_item(self, parent):
        if parent is None or not parent.isValid():
            return self.rootItem
        return parent.internalPointer()

    def is_scan_tof(self, row):
        scan = self.scan_from_row(row)
        tof = scan.child(0).get_tree_item_data(TreeItemEnum.tof_channels.value) > 1
        return tof

    def get_checked(self, full_info=False):
        """
        Returns a list of all checked items which do not have children
        list of DNS datafiles.
        """
        checked = self.match(self.index(0, 0, QModelIndex()), Qt.CheckStateRole, Qt.Checked, -1, Qt.MatchExactly | Qt.MatchRecursive)
        n_checked = []
        for index in checked:
            item = self._item_from_index(index)
            if not item.hasChildren():
                if full_info:
                    n_checked.append(
                        {
                            "file_number": int(item.get_tree_item_data(TreeItemEnum.number.value)),
                            "det_rot": float(round(decimal.Decimal(item.get_tree_item_data(TreeItemEnum.det_rot.value)), 1)),
                            "sample_rot": float(round(decimal.Decimal(item.get_tree_item_data(TreeItemEnum.sample_rot.value)), 1)),
                            "field": item.get_tree_item_data(TreeItemEnum.field.value),
                            "temperature": float(item.get_tree_item_data(TreeItemEnum.temperature.value)),
                            "sample_name": item.get_tree_item_data(TreeItemEnum.sample.value),
                            "tof_channels": int(item.get_tree_item_data(TreeItemEnum.tof_channels.value)),
                            "channel_width": float(item.get_tree_item_data(TreeItemEnum.tof_channel_width.value)),
                            "filename": item.get_tree_item_data(TreeItemEnum.filename.value),
                            "wavelength": float(item.get_tree_item_data(TreeItemEnum.wavelength.value)) * 10,
                            "sample_type": self._get_sample_type(item.get_tree_item_data(TreeItemEnum.sample.value)),
                            "selector_speed": float(item.get_tree_item_data(TreeItemEnum.selector_speed.value)),
                        }
                    )
                else:
                    n_checked.append(int(item.get_tree_item_data(TreeItemEnum.number.value)))
        return n_checked

    def check_scans_by_indexes(self, indexes):
        self.uncheck_all_scans()
        for index in indexes:
            self.set_checked_from_index(index, 2)

    def check_scans_by_rows(self, rows):
        self.uncheck_all_scans()
        for row in rows:
            self.set_checked_scan(row, 2)

    def get_complete_scan_rows(self, not_hidden_rows):
        return [row for row in not_hidden_rows if self._is_scan_complete(row)]

    def setData(self, index, value, role=Qt.EditRole):  # overrides QT function
        """
        Checks a specific item.
        """
        if index.column() == 0:
            if role == Qt.EditRole:
                return False
            if role == Qt.CheckStateRole:
                item = self._item_from_index(index)
                item.setChecked(value)
                self._item_checked(index)
                self.dataChanged.emit(index, index)
        return True

    def set_checked_scan(self, row, value):
        self.setData(self._scan_index_from_row(row), value, role=Qt.CheckStateRole)

    def set_checked_from_index(self, index, value=2):
        self.setData(index, value, role=Qt.CheckStateRole)

    def _item_checked(self, index):
        """
        Checks all children items if item is checked and parent items if all
        children are checked and vice versa.
        """
        item = self._item_from_index(index)
        if item.hasChildren():
            for row in range(item.childCount()):
                child = item.child(row)
                child.setChecked(item.isChecked())
                child_index = self.index(row, 0, index)
                self.dataChanged.emit(child_index, child_index)
        else:
            parent = item.parent()
            status = 0
            for row in range(parent.childCount()):
                child = parent.child(row)
                status = status + child.isChecked()
                if status == 0:  # unchecked
                    parent.setChecked(0)
                elif status < 2 * (parent.childCount()):  # partially checked
                    parent.setChecked(1)
                else:  # all checked
                    parent.setChecked(2)
            parent_index = self._index_from_scan(parent)
            self.dataChanged.emit(parent_index, parent_index)

    def uncheck_all_scans(self):
        for row in self._scan_range():
            self.set_checked_scan(row, 0)

    def clear_scans(self):
        """
        Removes all scans from model.
        """
        self.beginRemoveRows(QModelIndex(), 0, self.number_of_scans() - 1)
        self.rootItem.clearChilds()
        self.endRemoveRows()
        self._last_scan_number = None

    def _new_scan_check(self, dns_file):
        """
        Separates scans and measurements with different tof-channels
        or sample names.
        """
        return (
            dns_file.scan_number != self._last_scan_number
            or self._last_tof != dns_file.tof_channels
            or self._last_tof_time != dns_file.channel_width
            or self._last_sample != dns_file.sample
        )

    @staticmethod
    def _get_scan_text(dns_file):
        return [f"{dns_file.scan_number} {dns_file.sample}" f" {dns_file.scan_command}" f" #{dns_file.scan_points}"] + 9 * [""]

    @staticmethod
    def _get_data_from_dns_file(dns_file):
        return [
            dns_file.file_number,
            dns_file.det_rot,
            dns_file.sample_rot,
            dns_file.field,
            dns_file.temp_sample,
            dns_file.sample,
            dns_file.end_time,
            dns_file.tof_channels,
            dns_file.channel_width,
            dns_file.filename,
            dns_file.wavelength,
            dns_file.selector_speed,
            dns_file.scan_number,
            dns_file.scan_command,
            dns_file.scan_points,
        ]

    @staticmethod
    def _check_child_if_scan_is_checked(scan, child):
        if scan.isChecked():
            child.setChecked()

    def check_fn_range(self, start, end):
        file_number_dict = self.get_file_number_dict()
        for i in np.arange(start, end + 1, 1):
            self.set_checked_from_index(file_number_dict[i])

    def setup_model_data(self, dns_files):
        """
        Adding data to the model accepts a list of dns file objects.
        """
        root_item = self.rootItem
        for dns_file in dns_files:
            if self._new_scan_check(dns_file):
                self._scan = DNSTreeItem(self._get_scan_text(dns_file), root_item)
                self.beginInsertRows(QModelIndex(), self.number_of_scans(), self.number_of_scans())
                root_item.appendChild(self._scan)
                self.endInsertRows()
                self._last_scan_number = dns_file.scan_number
            file_data = self._get_data_from_dns_file(dns_file)
            self._last_tof = dns_file.tof_channels
            self._last_tof_time = dns_file.channel_width
            self._last_sample = dns_file.sample
            self.beginInsertRows(self._index_from_scan(self._scan), self._scan.childCount(), self._scan.childCount())
            child = self._scan.appendChild(DNSTreeItem(file_data, self._scan))
            self._check_child_if_scan_is_checked(self._scan, child)
            self.endInsertRows()

    def add_number_of_children(self):
        """
        Adding the number of present dns datafiles to a scan.
        """
        total_files = 0
        for row in range(self.number_of_scans()):
            scan = self.scan_from_row(row)
            prefix, postfix = scan.get_tree_item_data(0).split("#")
            prefix = prefix.strip()
            if "/" in postfix:  # multiple run
                postfix = postfix.split("/")[1]
            scan.setData(f"{prefix} #{scan.childCount()}/{postfix}", 0)
            total_files += scan.childCount()
        return total_files

    def get_txt(self):
        """
        Returns a list of dns datafiles used for quick loading.
        """
        txt = []
        for row in range(self.number_of_scans()):
            scan = self.scan_from_row(row)
            for crow in range(scan.childCount()):
                child = scan.child(crow)
                txt.append(" ; ".join([str(x) for x in child.get_tree_item_data()]) + "\n")
        return txt

    def get_file_number_dict(self):
        """
        Return a dictionary with  file numbers as keys and modelindex as value
        used to mark loaded file numbers are in the model.
        """
        file_number_dict = {}
        for scan_number in self._scan_range():
            scan = self.scan_from_row(scan_number)
            scan_index = self._index_from_scan(scan)
            for row in range(scan.childCount()):
                file_number = int(scan.child(row).get_tree_item_data(0))
                index = self.index(row, 0, scan_index)
                file_number_dict[file_number] = index
        return file_number_dict
