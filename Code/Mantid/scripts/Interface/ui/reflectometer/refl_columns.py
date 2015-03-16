#pylint: disable=invalid-name
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'D:\mantid\windows\Code\Mantid\scripts\Interface\ui\reflectometer/refl_columns.ui'
#
# Created: Tue Mar 25 15:42:15 2014
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_chooseColumnsDialog(object):
    def setupUi(self, chooseColumnsDialog):
        chooseColumnsDialog.setObjectName(_fromUtf8("chooseColumnsDialog"))
        chooseColumnsDialog.resize(307, 300)
        chooseColumnsDialog.setSizeGripEnabled(False)
        self.verticalLayout = QtGui.QVBoxLayout(chooseColumnsDialog)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.labelColumns = QtGui.QLabel(chooseColumnsDialog)
        self.labelColumns.setObjectName(_fromUtf8("labelColumns"))
        self.verticalLayout.addWidget(self.labelColumns)
        self.layoutListButtons = QtGui.QHBoxLayout()
        self.layoutListButtons.setObjectName(_fromUtf8("layoutListButtons"))
        self.listColumns = QtGui.QListWidget(chooseColumnsDialog)
        self.listColumns.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.listColumns.setSelectionMode(QtGui.QAbstractItemView.NoSelection)
        self.listColumns.setObjectName(_fromUtf8("listColumns"))
        self.layoutListButtons.addWidget(self.listColumns)
        self.buttonsColumns = QtGui.QDialogButtonBox(chooseColumnsDialog)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.buttonsColumns.sizePolicy().hasHeightForWidth())
        self.buttonsColumns.setSizePolicy(sizePolicy)
        self.buttonsColumns.setLayoutDirection(QtCore.Qt.LeftToRight)
        self.buttonsColumns.setOrientation(QtCore.Qt.Vertical)
        self.buttonsColumns.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok|QtGui.QDialogButtonBox.RestoreDefaults)
        self.buttonsColumns.setCenterButtons(False)
        self.buttonsColumns.setObjectName(_fromUtf8("buttonsColumns"))
        self.layoutListButtons.addWidget(self.buttonsColumns)
        self.verticalLayout.addLayout(self.layoutListButtons)

        self.retranslateUi(chooseColumnsDialog)
        QtCore.QObject.connect(self.buttonsColumns, QtCore.SIGNAL(_fromUtf8("accepted()")), chooseColumnsDialog.accept)
        QtCore.QObject.connect(self.buttonsColumns, QtCore.SIGNAL(_fromUtf8("rejected()")), chooseColumnsDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(chooseColumnsDialog)

    def retranslateUi(self, chooseColumnsDialog):
        chooseColumnsDialog.setWindowTitle(QtGui.QApplication.translate("chooseColumnsDialog", "Choose Columns...", None, QtGui.QApplication.UnicodeUTF8))
        self.labelColumns.setText(QtGui.QApplication.translate("chooseColumnsDialog", "Choose columns to display", None, QtGui.QApplication.UnicodeUTF8))

