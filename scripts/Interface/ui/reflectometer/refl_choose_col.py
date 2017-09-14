#pylint: disable=invalid-name
#This is an extension of refl_columns.py as that is a auto-generated script form pyqt and shouldn't be edited
#so this file provides any extra GUI tweaks not easily doable in the designer
#for the time being this also includes non-GUI behaviour
from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtCore, QtGui
from ui.reflectometer.ui_refl_columns import Ui_chooseColumnsDialog

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s


class ReflChoose(QtGui.QDialog, Ui_chooseColumnsDialog):

    visiblestates = {}

    def __init__(self, col_headers, table):
        """
        Initialise the interface
        """
        super(QtGui.QDialog, self).__init__()
        self.setupUi(self)
        self.visiblestates.clear()
        self.listColumns.itemChanged.connect(self.on_listColumns_itemChanged)
        self.buttonsColumns.clicked.connect(self.on_buttonsColumns_Clicked)
        for key, value in col_headers.iteritems():
            header = table.horizontalHeaderItem(key).text()
            item = QtGui.QListWidgetItem(header)
            if value:
                item.setCheckState(2)
            else:
                item.setCheckState(0)
            self.listColumns.insertItem(key, item)

    def on_listColumns_itemChanged(self, item):
        colno=self.listColumns.row(item)
        self.visiblestates[colno] = (item.checkState() > 0)

    def on_buttonsColumns_Clicked(self, button):
        if self.buttonsColumns.button(QtGui.QDialogButtonBox.RestoreDefaults) == button:
            for i in range(self.listColumns.count()):
                self.listColumns.item(i).setCheckState(2)
