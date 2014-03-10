# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'D:\mantid\windows\Code\Mantid\scripts\Interface\ui\reflectometer/refl_columns.ui'
#
# Created: Mon Mar 10 10:15:17 2014
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_ChooseColumnsDialog(object):
    def setupUi(self, ChooseColumnsDialog):
        ChooseColumnsDialog.setObjectName(_fromUtf8("ChooseColumnsDialog"))
        ChooseColumnsDialog.resize(231, 300)
        ChooseColumnsDialog.setSizeGripEnabled(False)
        self.verticalLayout = QtGui.QVBoxLayout(ChooseColumnsDialog)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.labelColumns = QtGui.QLabel(ChooseColumnsDialog)
        self.labelColumns.setObjectName(_fromUtf8("labelColumns"))
        self.verticalLayout.addWidget(self.labelColumns)
        self.layoutListButtons = QtGui.QHBoxLayout()
        self.layoutListButtons.setObjectName(_fromUtf8("layoutListButtons"))
        self.listColumns = QtGui.QListWidget(ChooseColumnsDialog)
        self.listColumns.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.listColumns.setSelectionMode(QtGui.QAbstractItemView.NoSelection)
        self.listColumns.setObjectName(_fromUtf8("listColumns"))
        self.layoutListButtons.addWidget(self.listColumns)
        self.buttonsColumns = QtGui.QDialogButtonBox(ChooseColumnsDialog)
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

        self.retranslateUi(ChooseColumnsDialog)
        QtCore.QObject.connect(self.buttonsColumns, QtCore.SIGNAL(_fromUtf8("accepted()")), ChooseColumnsDialog.accept)
        QtCore.QObject.connect(self.buttonsColumns, QtCore.SIGNAL(_fromUtf8("rejected()")), ChooseColumnsDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(ChooseColumnsDialog)

    def retranslateUi(self, ChooseColumnsDialog):
        ChooseColumnsDialog.setWindowTitle(QtGui.QApplication.translate("ChooseColumnsDialog", "Choose Columns...", None, QtGui.QApplication.UnicodeUTF8))
        self.labelColumns.setText(QtGui.QApplication.translate("ChooseColumnsDialog", "Choose columns to display", None, QtGui.QApplication.UnicodeUTF8))

