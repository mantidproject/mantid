# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Custom Tree Model for DNS to store list of Scans with files as children
"""
import numpy as np
from qtpy.QtCore import QAbstractItemModel, QModelIndex, Qt
from mantidqtinterfaces.dns.data_structures.dns_treeitem import DNSTreeItem


class DNSTreeModel(QAbstractItemModel):
    # pylint: disable=too-many-public-methods   # redefinition of QT methods
    """
    QT Model to store DNS scan structure consisting of scans with files as
    children
    """

    def __init__(self, data=None, parent=None):
        super().__init__(parent)
        self._scan = None
        self.rootItem = DNSTreeItem(
            ('number', 'det_rot', 'sample_rot', 'field', 'temperature',
             'sample', 'time', 'tof_channels', 'tof_channelwidth', 'filepath'))
        self._lastscan_number = None
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
        parent_item = child_item.parent()
        if parent_item == self.rootItem:
            return QModelIndex()
        return self.createIndex(parent_item.row(), 0, parent_item)

    def headerData(self, section, orientation, role):  # overrides QT function
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self.rootItem.data(section)
        return None

    def columnCount(self, parent):  # overrides QT function
        """
        returns number fo columns
        """
        if parent.isValid():
            return parent.internalPointer().columnCount()
        return self.rootItem.columnCount()

    def _scan_command_from_row(self, row):
        return self.rootItem.child(row).data(0)

    def _scan_expected_points_from_row(self, row):
        """
        returns the number of the scanpoints which are expected from the scan
        command can be smaller than number of childs if scan did not run
        completly
        """
        scancommand = self._scan_command_from_row(row)
        scancommand = scancommand.split('#')[1]
        if '/' in scancommand:
            scancommand = scancommand.split('/')[1]
        if scancommand:
            return int(scancommand.strip())
        return 0

    def number_of_scans(self):
        return self.rootItem.childCount()

    def _scan_range(self):
        return range(self.number_of_scans())

    @staticmethod
    def _get_sampletype(sample):
        if 'vanadium' in sample or 'vana' in sample:
            return 'vana'
        if 'nicr' in sample or 'NiCr' in sample:
            return 'nicr'
        if 'empty' in sample or 'leer' in sample:
            return 'empty'
        return sample

    # complex getting

    def _get_scan_rows(self):
        return [
            row for row in self._scan_range()
            if 'scan' in self._scan_command_from_row(row)
        ]

    def _is_scan_complete(self, row):
        return (self.scan_from_row(row).childCount() >=
                self._scan_expected_points_from_row(row))

    def text_in_scan(self, row, text):
        return text in self._scan_command_from_row(row)

    def _get_last_row(self):
        return self.number_of_scans() - 1

    def data(self, index, role):  # overrides QT function
        """
        returning either data or checkstate of items
        """
        item = self._item_from_index(index)
        if role == Qt.DisplayRole:
            return item.data(index.column())
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
            return (Qt.ItemIsEnabled
                    | Qt.ItemIsSelectable
                    | Qt.ItemIsUserCheckable)
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
        if not item.hasChildren() and item is not None:
            return item.data(9)
        return ''

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
        tof = scan.child(0).data(7) > 1
        return tof

    def get_checked(self, fullinfo=False):
        """
        returns a list of all checked items which do not have children
        List of dns datafiles
        """
        checked = self.match(self.index(0, 0, QModelIndex()),
                             Qt.CheckStateRole, Qt.Checked, -1,
                             Qt.MatchExactly | Qt.MatchRecursive)
        nchecked = []
        for index in checked:
            item = self._item_from_index(index)
            if not item.hasChildren():
                if fullinfo:
                    nchecked.append({
                        'filenumber': int(item.data(0)),
                        'det_rot': float(item.data(1)),
                        'sample_rot': float(item.data(2)),
                        'field': item.data(3),
                        'temperature': float(item.data(4)),
                        'samplename': item.data(5),
                        'tofchannels': int(item.data(7)),
                        'channelwidth': float(item.data(8)),
                        'filename': item.data(9),
                        'wavelength': float(item.data(10)) * 10,
                        'sampletype': self._get_sampletype(item.data(5)),
                        'selector_speed': float(item.data(11))
                    })
                else:
                    nchecked.append(int(item.data(0)))
        return nchecked

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
        Checking of a specific item
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
        self.setData(self._scan_index_from_row(row),
                     value,
                     role=Qt.CheckStateRole)

    def set_checked_from_index(self, index, value=2):
        self.setData(index, value, role=Qt.CheckStateRole)

    def _item_checked(self, index):
        """
        Cheking of all childs if item is checked and parent if all childs are
        checked and oposite
        """
        item = self._item_from_index(index)
        if item.hasChildren():
            for row in range(item.childCount()):
                child = item.child(row)
                child.setChecked(item.isChecked())
                childindex = self.index(row, 0, index)
                self.dataChanged.emit(childindex, childindex)
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
            parentindex = self._index_from_scan(parent)
            self.dataChanged.emit(parentindex, parentindex)

    def uncheck_all_scans(self):
        for row in self._scan_range():
            self.set_checked_scan(row, 0)

    def clear_scans(self):
        """
        Removing of all scans from model
        """
        self.beginRemoveRows(QModelIndex(), 0, self.number_of_scans() - 1)
        self.rootItem.clearChilds()
        self.endRemoveRows()
        self._lastscan_number = None

    def _new_scan_check(self, dnsfile):
        # seperates scans and measurements with different tof-channels
        # or samplenames
        return (dnsfile.scannumber != self._lastscan_number
                or self._last_tof != dnsfile.tofchannels
                or self._last_tof_time != dnsfile.channelwidth
                or self._last_sample != dnsfile.sample)

    @staticmethod
    def _get_scantext(dnsfile):
        return [
            f'{dnsfile.scannumber} {dnsfile.sample} {dnsfile.scancommand}'
            f' #{dnsfile.scanpoints}'
        ] + 9 * ['']

    @staticmethod
    def _get_data_from_dnsfile(dnsfile):
        return [
            dnsfile.filenumber, dnsfile.det_rot, dnsfile.sample_rot,
            dnsfile.field, dnsfile.temp_samp, dnsfile.sample, dnsfile.endtime,
            dnsfile.tofchannels, dnsfile.channelwidth, dnsfile.filename,
            dnsfile.wavelength, dnsfile.selector_speed, dnsfile.scannumber,
            dnsfile.scancommand, dnsfile.scanpoints
        ]

    @staticmethod
    def _check_child_if_scan_is_checked(scan, child):
        if scan.isChecked():
            child.setChecked()

    def check_fn_range(self, start, end):
        fndict = self.get_filenumber_dict()
        for i in np.arange(start, end + 1, 1):
            self.set_checked_from_index(fndict[i])

    def setup_model_data(self, dnsfiles):
        """
        Adding data to the model accepts a list of dnsfile objects
        """
        rootitem = self.rootItem
        for dnsfile in dnsfiles:
            if self._new_scan_check(dnsfile):
                self._scan = DNSTreeItem(self._get_scantext(dnsfile), rootitem)
                self.beginInsertRows(QModelIndex(), self.number_of_scans(),
                                     self.number_of_scans())
                rootitem.appendChild(self._scan)
                self.endInsertRows()
                self._lastscan_number = dnsfile.scannumber
            file_data = self._get_data_from_dnsfile(dnsfile)
            self._last_tof = dnsfile.tofchannels
            self._last_tof_time = dnsfile.channelwidth
            self._last_sample = dnsfile.sample
            self.beginInsertRows(self._index_from_scan(self._scan),
                                 self._scan.childCount(),
                                 self._scan.childCount())
            child = self._scan.appendChild(DNSTreeItem(file_data, self._scan))
            self._check_child_if_scan_is_checked(self._scan, child)
            self.endInsertRows()

    def add_number_of_childs(self):
        """
        Adding the number of present dns datafiles to a scan
        """
        total_files = 0
        for row in range(self.number_of_scans()):
            scan = self.scan_from_row(row)
            prefix, postfix = scan.data(0).split('#')
            prefix = prefix.strip()
            if '/' in postfix:  # multiple run
                postfix = postfix.split('/')[1]
            scan.setData(
                f'{prefix} #{scan.childCount()}/{postfix}', 0)
            total_files += scan.childCount()
        return total_files

    def get_txt(self):
        """
        return a list of dnsdatfiles used for quick loading
        """
        txt = []
        for row in range(self.number_of_scans()):
            scan = self.scan_from_row(row)
            for crow in range(scan.childCount()):
                child = scan.child(crow)
                txt.append(" ; ".join([str(x) for x in child.data()]) + "\n")
        return txt

    def get_filenumber_dict(self):
        """
        return a dictionary with  filnumbers as keys and modelindex as value
        used to mark loaded filenumbers are in the model
        """
        filenb_dict = {}
        for scannb in self._scan_range():
            scan = self.scan_from_row(scannb)
            scanindex = self._index_from_scan(scan)
            for row in range(scan.childCount()):
                filenb = int(scan.child(row).data(0))
                index = self.index(row, 0, scanindex)
                filenb_dict[filenb] = index
        return filenb_dict
