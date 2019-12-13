# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Custom Tree Model for DNS to store list of Scans with files as children
"""
from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import QAbstractItemModel, QModelIndex, Qt

from DNSReduction.data_structures.dns_treeitem import DNSTreeItem


class DNSTreeModel(QAbstractItemModel):
    """
    QT Model to store DNS scan structure consisting of scans with files as
    children this is not a model in MVP sense
    """
    def __init__(self, data=None, parent=None):
        super(DNSTreeModel, self).__init__(parent)

        self.rootItem = DNSTreeItem(
            ('number', 'det_rot', 'sample_rot', 'field', 'temperature',
             'sample', 'time', 'tof_channels', 'tof_channelwidth', 'filepath'))
        self.lastscan_number = None
        self.last_tof_time = None
        self.last_tof = None
        self.last_sample = None
        if data is not None:
            self.setupModelData(data)

### unchanged from siple treemodel example

    def index(self, row, column, parent):
        if not self.hasIndex(row, column, parent):
            return QModelIndex()
        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()
        childItem = parentItem.child(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        return QModelIndex()

    def parent(self, index):
        if not index.isValid():
            return QModelIndex()
        childItem = index.internalPointer()
        parentItem = childItem.parent()
        if parentItem == self.rootItem:
            return QModelIndex()
        return self.createIndex(parentItem.row(), 0, parentItem)

    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self.rootItem.data(section)
        return None

    def columnCount(self, parent):
        """
        returns number fo columns
        """
        if parent.isValid():
            return parent.internalPointer().columnCount()
        return self.rootItem.columnCount()

#### special functions

### getting stuff - unproblematic

    def scanCommandFromRow(self, row):
        return self.rootItem.child(row).data(0)

    def scanExpectedPointsFromRow(self, row):
        """
        returns the number of the scanpoints which are expected from the scan
        command can be smaller than number of childs if scan did not run
        completly
        """
        scancommand = self.scanCommandFromRow(row)
        scancommand = scancommand.split('#')[1]
        if '/' in scancommand:
            scancommand = scancommand.split('/')[1]
        return int(scancommand.strip())

    def numberOfScans(self):
        return self.rootItem.childCount()

    def scanRange(self):
        return range(self.numberOfScans())

    def get_sampletype(self, sample):
        if 'vanadium' in sample or 'vana' in sample:
            return 'vana'
        if 'nicr' in sample or 'NiCr' in sample:
            return 'nicr'
        if 'empty' in sample or 'leer' in sample:
            return 'empty'
        return sample
### complex getting

    def data(self, index, role):
        """
        returning either data or checkstate of items
        """
        item = self.itemFromIndex(index)
        if role == Qt.DisplayRole:
            return item.data(index.column())
        if role == Qt.CheckStateRole and index.column() == 0:
            if item.isChecked() == 2:
                return Qt.Checked
            elif item.isChecked() == 1:
                return Qt.PartiallyChecked
            return Qt.Unchecked
        else:
            return None

    def flags(self, index):
        if not index.isValid():
            return Qt.NoItemFlags
        if index.column() == 0:
            return (Qt.ItemIsEnabled
                    | Qt.ItemIsSelectable
                    | Qt.ItemIsUserCheckable)
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def scanIndexFromRow(self, row):
        return self.index(row, 0, QModelIndex())

    def IndexFromScan(self, scan):
        return self.scanIndexFromRow(scan.row())

    def itemFromIndex(self, index):
        return index.internalPointer()

    def itemFromRow(self, row, parent):
        index = self.index(row, 0, parent)
        item = self.itemFromIndex(index)
        return item

    def scanFromRow(self, row):
        return self.rootItem.child(row)

    def rowCount(self, parent=None):
        if parent is None or not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        if parent is not None and parent.column() > 0:
            return 0

        return parentItem.childCount()

    def is_scan_tof(self, row):
        scan = self.scanFromRow(row)
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
            item = self.itemFromIndex(index)
            if not item.hasChildren():
                if fullinfo:
                    nchecked.append({
                        'filenumber' : int(item.data(0)),
                        'det_rot' : float(item.data(1)),
                        'sample_rot' : float(item.data(2)),
                        'field' : item.data(3),
                        'temperature' : float(item.data(4)),
                        'samplename' : item.data(5),
                        'tofchannels' : int(item.data(7)),
                        'channelwidth' : float(item.data(8)),
                        'filename' : item.data(9),
                        'wavelength': float(item.data(10)) * 10,
                        'sampletype' : self.get_sampletype(item.data(5)),
                        'selector_speed' : float(item.data(11))
                        })
                else:
                    nchecked.append(int(item.data(0)))
        return nchecked

    def get_filenames(self):
        mylist = []
        for row in range(self.numberOfScans()):
            scan = self.scanFromRow(row)
            #txt += scan.data(0) + "\n"
            for crow in range(scan.childCount()):
                child = scan.child(crow)
                mylist.append(child.data[9])
        return mylist

### setting stuff
    def setData(self, index, value, role=Qt.EditRole):
        """
        Checking of a specific item
        """
        if index.column() == 0:
            if role == Qt.EditRole:
                return False
            if role == Qt.CheckStateRole:
                item = self.itemFromIndex(index)
                item.setChecked(value)
                self.item_checked(index)
                self.dataChanged.emit(index, index)
        return True

    def setCheckedScan(self, row, value):
        self.setData(self.scanIndexFromRow(row), value, role=Qt.CheckStateRole)

    def setCheckedFromIndex(self, index, value=2):
        self.setData(index, value, role=Qt.CheckStateRole)

    def item_checked(self, index):
        """
        Cheking of all childs if item is checked and parent if all childs are
        checked and oposite
        """
        item = self.itemFromIndex(index)
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
                if status == 0:  #unchecked
                    parent.setChecked(0)
                elif status < 2 * (parent.childCount()):  # partially checked
                    parent.setChecked(1)
                else:  # all checked
                    parent.setChecked(2)
            parentindex = self.IndexFromScan(parent)
            self.dataChanged.emit(parentindex, parentindex)
        return

    def clearScans(self):
        """
        Removing of all scans from model
        """
        self.beginRemoveRows(QModelIndex(), 0, self.numberOfScans() - 1)
        self.rootItem.clearChilds()
        self.endRemoveRows()
        self.lastscan_number = None

    def setupModelData(self, dnsfiles):
        """
        Addinhg data to the model accepts a list of dnsfile objects
        """

        rootitem = self.rootItem
        for dnsfile in dnsfiles:
            ## seperates scans and measurements with different tof-channels
            if (dnsfile.scannumber != self.lastscan_number
                    or self.last_tof != dnsfile.tofchannels
                    or self.last_tof_time != dnsfile.channelwidth
                    or self.last_sample != dnsfile.sample):
                scantext = [
                    '{} {} {} #{}'.format(dnsfile.scannumber, dnsfile.sample,
                                          dnsfile.scancommand,
                                          dnsfile.scanpoints), '', '', '', '',
                    '', '', '', '', ''
                ]
                self.scan = DNSTreeItem(scantext, rootitem)
                self.beginInsertRows(QModelIndex(),
                                     self.numberOfScans(),
                                     self.numberOfScans())
                rootitem.appendChild(self.scan)
                self.endInsertRows()
                self.lastscan_number = dnsfile.scannumber
            file_data = [
                dnsfile.filenumber, dnsfile.det_rot, dnsfile.sample_rot,
                dnsfile.field, dnsfile.temp_samp, dnsfile.sample,
                dnsfile.endtime, dnsfile.tofchannels, dnsfile.channelwidth,
                dnsfile.filename, dnsfile.wavelength, dnsfile.selector_speed,
                dnsfile.scannumber, dnsfile.scancommand, dnsfile.scanpoints
            ]
            self.last_tof = dnsfile.tofchannels
            self.last_tof_time = dnsfile.channelwidth
            self.last_sample = dnsfile.sample
            self.beginInsertRows(self.IndexFromScan(self.scan),
                                 self.scan.childCount(),
                                 self.scan.childCount())
            child = self.scan.appendChild(DNSTreeItem(file_data, self.scan))
            self.endInsertRows()
            if self.scan.isChecked():
                child.setChecked()

    def add_number_of_childs(self):
        """
        Adding the number of present dns datafiles to a scan
        """
        total_files = 0
        for row in range(self.numberOfScans()):
            scan = self.scanFromRow(row)
            prefix, postfix = scan.data(0).split('#')
            prefix = prefix.strip()
            if '/' in postfix:  ## multiple run
                postfix = postfix.split('/')[1]
            scan.setData(
                '{} #{}/{}'.format(prefix, scan.childCount(), postfix), 0)
            total_files += scan.childCount()
        return total_files

    def get_txt(self):
        txt = []
        for row in range(self.numberOfScans()):
            scan = self.scanFromRow(row)
            for crow in range(scan.childCount()):
                child = scan.child(crow)
                txt.append(" ; ".join([str(x) for x in child.data()]) + "\n")
        return txt
