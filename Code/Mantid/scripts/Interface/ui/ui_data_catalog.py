#pylint: disable=invalid-name,attribute-defined-outside-init
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/data_catalog.ui'
#
# Created: Thu Jan 26 12:15:05 2012
#      by: PyQt4 UI code generator 4.7.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_Frame(object):
    def setupUi(self, Frame):
        Frame.setObjectName("Frame")
        Frame.resize(810, 660)
        Frame.setFrameShape(QtGui.QFrame.NoFrame)
        Frame.setFrameShadow(QtGui.QFrame.Plain)
        self.verticalLayout = QtGui.QVBoxLayout(Frame)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName("verticalLayout")
        self.data_set_table = QtGui.QTableWidget(Frame)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.data_set_table.sizePolicy().hasHeightForWidth())
        self.data_set_table.setSizePolicy(sizePolicy)
        self.data_set_table.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.data_set_table.setObjectName("data_set_table")
        self.data_set_table.setColumnCount(0)
        self.data_set_table.setRowCount(0)
        self.verticalLayout.addWidget(self.data_set_table)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.directory_edit = QtGui.QLineEdit(Frame)
        self.directory_edit.setObjectName("directory_edit")
        self.horizontalLayout.addWidget(self.directory_edit)
        self.browse_button = QtGui.QPushButton(Frame)
        self.browse_button.setObjectName("browse_button")
        self.horizontalLayout.addWidget(self.browse_button)
        self.refresh_button = QtGui.QPushButton(Frame)
        self.refresh_button.setObjectName("refresh_button")
        self.horizontalLayout.addWidget(self.refresh_button)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi(Frame)
        QtCore.QMetaObject.connectSlotsByName(Frame)

    def retranslateUi(self, Frame):
        Frame.setWindowTitle(QtGui.QApplication.translate("Frame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.browse_button.setToolTip(QtGui.QApplication.translate("Frame", "Click to browse.", None, QtGui.QApplication.UnicodeUTF8))
        self.browse_button.setText(QtGui.QApplication.translate("Frame", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.refresh_button.setToolTip(QtGui.QApplication.translate("Frame", "Click to refresh.", None, QtGui.QApplication.UnicodeUTF8))
        self.refresh_button.setText(QtGui.QApplication.translate("Frame", "Refresh", None, QtGui.QApplication.UnicodeUTF8))

