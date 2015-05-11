#pylint: disable=invalid-name,attribute-defined-outside-init,line-too-long,too-many-instance-attributes
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/trans_direct_beam.ui'
#
# Created: Wed Nov 16 13:57:36 2011
#      by: PyQt4 UI code generator 4.7.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_GroupBox(object):
    def setupUi(self, GroupBox):
        GroupBox.setObjectName("GroupBox")
        GroupBox.resize(822, 293)
        self.gridLayoutWidget = QtGui.QWidget(GroupBox)
        self.gridLayoutWidget.setGeometry(QtCore.QRect(0, 20, 611, 111))
        self.gridLayoutWidget.setObjectName("gridLayoutWidget")
        self.gridLayout = QtGui.QGridLayout(self.gridLayoutWidget)
        self.gridLayout.setSpacing(0)
        self.gridLayout.setObjectName("gridLayout")
        self.label = QtGui.QLabel(self.gridLayoutWidget)
        self.label.setMinimumSize(QtCore.QSize(180, 0))
        self.label.setObjectName("label")
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.sample_edit = QtGui.QLineEdit(self.gridLayoutWidget)
        self.sample_edit.setMaximumSize(QtCore.QSize(300, 16777215))
        self.sample_edit.setObjectName("sample_edit")
        self.gridLayout.addWidget(self.sample_edit, 0, 2, 1, 1)
        self.sample_browse = QtGui.QPushButton(self.gridLayoutWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.sample_browse.sizePolicy().hasHeightForWidth())
        self.sample_browse.setSizePolicy(sizePolicy)
        self.sample_browse.setObjectName("sample_browse")
        self.gridLayout.addWidget(self.sample_browse, 0, 3, 1, 1)
        self.label_2 = QtGui.QLabel(self.gridLayoutWidget)
        self.label_2.setObjectName("label_2")
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.direct_edit = QtGui.QLineEdit(self.gridLayoutWidget)
        self.direct_edit.setMaximumSize(QtCore.QSize(300, 16777215))
        self.direct_edit.setObjectName("direct_edit")
        self.gridLayout.addWidget(self.direct_edit, 1, 2, 1, 1)
        self.direct_browse = QtGui.QPushButton(self.gridLayoutWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.direct_browse.sizePolicy().hasHeightForWidth())
        self.direct_browse.setSizePolicy(sizePolicy)
        self.direct_browse.setObjectName("direct_browse")
        self.gridLayout.addWidget(self.direct_browse, 1, 3, 1, 1)
        self.beam_radius_edit = QtGui.QLineEdit(self.gridLayoutWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.beam_radius_edit.sizePolicy().hasHeightForWidth())
        self.beam_radius_edit.setSizePolicy(sizePolicy)
        self.beam_radius_edit.setMaximumSize(QtCore.QSize(97, 16777215))
        self.beam_radius_edit.setObjectName("beam_radius_edit")
        self.gridLayout.addWidget(self.beam_radius_edit, 2, 2, 1, 1)
        self.label_3 = QtGui.QLabel(self.gridLayoutWidget)
        self.label_3.setObjectName("label_3")
        self.gridLayout.addWidget(self.label_3, 2, 0, 1, 1)

        self.retranslateUi(GroupBox)
        QtCore.QMetaObject.connectSlotsByName(GroupBox)

    def retranslateUi(self, GroupBox):
        GroupBox.setWindowTitle(QtGui.QApplication.translate("GroupBox", "GroupBox", None, QtGui.QApplication.UnicodeUTF8))
        GroupBox.setTitle(QtGui.QApplication.translate("GroupBox", "Direct Beam", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("GroupBox", "Sample direct beam data file:", None, QtGui.QApplication.UnicodeUTF8))
        self.sample_edit.setToolTip(QtGui.QApplication.translate("GroupBox", "Enter a valid data file path.", None, QtGui.QApplication.UnicodeUTF8))
        self.sample_browse.setText(QtGui.QApplication.translate("GroupBox", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("GroupBox", "Empty direct beam data file:", None, QtGui.QApplication.UnicodeUTF8))
        self.direct_edit.setToolTip(QtGui.QApplication.translate("GroupBox", "Enter a valid data file path.", None, QtGui.QApplication.UnicodeUTF8))
        self.direct_browse.setText(QtGui.QApplication.translate("GroupBox", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.beam_radius_edit.setToolTip(QtGui.QApplication.translate("GroupBox", "Radius of the beam in pixels.", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("GroupBox", "Beam radius (pixels)", None, QtGui.QApplication.UnicodeUTF8))

