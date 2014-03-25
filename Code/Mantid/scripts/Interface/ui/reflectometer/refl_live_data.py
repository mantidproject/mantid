# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'D:\mantid\windows\Code\Mantid\scripts\Interface\ui\reflectometer/refl_live_data.ui'
#
# Created: Tue Mar 25 15:42:16 2014
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_liveDataDialog(object):
    def setupUi(self, liveDataDialog):
        liveDataDialog.setObjectName(_fromUtf8("liveDataDialog"))
        liveDataDialog.resize(420, 96)
        self.layoutLive = QtGui.QFormLayout(liveDataDialog)
        self.layoutLive.setObjectName(_fromUtf8("layoutLive"))
        self.labelAccMethod = QtGui.QLabel(liveDataDialog)
        self.labelAccMethod.setObjectName(_fromUtf8("labelAccMethod"))
        self.layoutLive.setWidget(0, QtGui.QFormLayout.LabelRole, self.labelAccMethod)
        self.comboAccMethod = QtGui.QComboBox(liveDataDialog)
        self.comboAccMethod.setObjectName(_fromUtf8("comboAccMethod"))
        self.layoutLive.setWidget(0, QtGui.QFormLayout.FieldRole, self.comboAccMethod)
        self.labelFrequency = QtGui.QLabel(liveDataDialog)
        self.labelFrequency.setObjectName(_fromUtf8("labelFrequency"))
        self.layoutLive.setWidget(1, QtGui.QFormLayout.LabelRole, self.labelFrequency)
        self.dspinFrequency = QtGui.QDoubleSpinBox(liveDataDialog)
        self.dspinFrequency.setSingleStep(0.5)
        self.dspinFrequency.setObjectName(_fromUtf8("dspinFrequency"))
        self.layoutLive.setWidget(1, QtGui.QFormLayout.FieldRole, self.dspinFrequency)
        self.buttonsLive = QtGui.QDialogButtonBox(liveDataDialog)
        self.buttonsLive.setOrientation(QtCore.Qt.Horizontal)
        self.buttonsLive.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonsLive.setObjectName(_fromUtf8("buttonsLive"))
        self.layoutLive.setWidget(2, QtGui.QFormLayout.SpanningRole, self.buttonsLive)
        self.labelAccMethod.setBuddy(self.comboAccMethod)
        self.labelFrequency.setBuddy(self.dspinFrequency)

        self.retranslateUi(liveDataDialog)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("accepted()")), liveDataDialog.accept)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("rejected()")), liveDataDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(liveDataDialog)

    def retranslateUi(self, liveDataDialog):
        liveDataDialog.setWindowTitle(QtGui.QApplication.translate("liveDataDialog", "Live Data Options", None, QtGui.QApplication.UnicodeUTF8))
        self.labelAccMethod.setText(QtGui.QApplication.translate("liveDataDialog", "Accumulation Method", None, QtGui.QApplication.UnicodeUTF8))
        self.labelFrequency.setText(QtGui.QApplication.translate("liveDataDialog", "Update Every", None, QtGui.QApplication.UnicodeUTF8))

