#pylint: disable=invalid-name
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ErrorMessage.ui'
#
# Created: Fri Jan  2 15:48:09 2015
#      by: PyQt4 UI code generator 4.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName(_fromUtf8("Dialog"))
        Dialog.resize(408, 262)
        Dialog.setMinimumSize(QtCore.QSize(40, 20))
        self.gridLayout = QtGui.QGridLayout(Dialog)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.verticalLayout_3 = QtGui.QVBoxLayout()
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        self.horizontalLayout_5 = QtGui.QHBoxLayout()
        self.horizontalLayout_5.setSizeConstraint(QtGui.QLayout.SetMinimumSize)
        self.horizontalLayout_5.setObjectName(_fromUtf8("horizontalLayout_5"))
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_5.addItem(spacerItem)
        self.label_title = QtGui.QLabel(Dialog)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.label_title.sizePolicy().hasHeightForWidth())
        self.label_title.setSizePolicy(sizePolicy)
        self.label_title.setMinimumSize(QtCore.QSize(80, 40))
        font = QtGui.QFont()
        font.setPointSize(16)
        font.setBold(True)
        font.setWeight(75)
        self.label_title.setFont(font)
        self.label_title.setTextFormat(QtCore.Qt.RichText)
        self.label_title.setAlignment(QtCore.Qt.AlignCenter)
        self.label_title.setObjectName(_fromUtf8("label_title"))
        self.horizontalLayout_5.addWidget(self.label_title)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_5.addItem(spacerItem1)
        self.verticalLayout_3.addLayout(self.horizontalLayout_5)
        spacerItem2 = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_3.addItem(spacerItem2)
        self.horizontalLayout_6 = QtGui.QHBoxLayout()
        self.horizontalLayout_6.setSizeConstraint(QtGui.QLayout.SetMinimumSize)
        self.horizontalLayout_6.setObjectName(_fromUtf8("horizontalLayout_6"))
        spacerItem3 = QtGui.QSpacerItem(41, 146, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem3)
        self.label_errmsg = QtGui.QLabel(Dialog)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.label_errmsg.sizePolicy().hasHeightForWidth())
        self.label_errmsg.setSizePolicy(sizePolicy)
        self.label_errmsg.setMinimumSize(QtCore.QSize(300, 80))
        self.label_errmsg.setTextFormat(QtCore.Qt.RichText)
        self.label_errmsg.setObjectName(_fromUtf8("label_errmsg"))
        self.horizontalLayout_6.addWidget(self.label_errmsg)
        spacerItem4 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem4)
        self.verticalLayout_3.addLayout(self.horizontalLayout_6)
        spacerItem5 = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_3.addItem(spacerItem5)
        self.horizontalLayout_7 = QtGui.QHBoxLayout()
        self.horizontalLayout_7.setSizeConstraint(QtGui.QLayout.SetMinimumSize)
        self.horizontalLayout_7.setObjectName(_fromUtf8("horizontalLayout_7"))
        spacerItem6 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_7.addItem(spacerItem6)
        self.pushButton_quit = QtGui.QPushButton(Dialog)
        self.pushButton_quit.setObjectName(_fromUtf8("pushButton_quit"))
        self.horizontalLayout_7.addWidget(self.pushButton_quit)
        spacerItem7 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_7.addItem(spacerItem7)
        self.verticalLayout_3.addLayout(self.horizontalLayout_7)
        self.gridLayout.addLayout(self.verticalLayout_3, 0, 0, 1, 1)
        spacerItem8 = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.gridLayout.addItem(spacerItem8, 1, 0, 1, 1)

        self.retranslateUi(Dialog)
        QtCore.QMetaObject.connectSlotsByName(Dialog)

    def retranslateUi(self, Dialog):
        Dialog.setWindowTitle(_translate("Dialog", "Dialog", None))
        self.label_title.setText(_translate("Dialog", "<html><head/><body><p><span style=\" color:#ff0000;\">Error</span></p></body></html>", None))
        self.label_errmsg.setText(_translate("Dialog", "TextLabel", None))
        self.pushButton_quit.setText(_translate("Dialog", "Close", None))

