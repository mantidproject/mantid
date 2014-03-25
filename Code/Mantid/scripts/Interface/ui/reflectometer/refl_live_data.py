# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'D:\mantid\windows\Code\Mantid\scripts\Interface\ui\reflectometer/refl_live_data.ui'
#
# Created: Tue Mar 25 11:38:14 2014
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_LiveDataDialog(object):
    def setupUi(self, LiveDataDialog):
        LiveDataDialog.setObjectName(_fromUtf8("LiveDataDialog"))
        LiveDataDialog.resize(420, 96)
        self.layoutLive = QtGui.QFormLayout(LiveDataDialog)
        self.layoutLive.setObjectName(_fromUtf8("layoutLive"))
        self.labelAccMethod = QtGui.QLabel(LiveDataDialog)
        self.labelAccMethod.setObjectName(_fromUtf8("labelAccMethod"))
        self.layoutLive.setWidget(0, QtGui.QFormLayout.LabelRole, self.labelAccMethod)
        self.comboAccMethod = QtGui.QComboBox(LiveDataDialog)
        self.comboAccMethod.setObjectName(_fromUtf8("comboAccMethod"))
        self.layoutLive.setWidget(0, QtGui.QFormLayout.FieldRole, self.comboAccMethod)
        self.labelFrequency = QtGui.QLabel(LiveDataDialog)
        self.labelFrequency.setObjectName(_fromUtf8("labelFrequency"))
        self.layoutLive.setWidget(1, QtGui.QFormLayout.LabelRole, self.labelFrequency)
        self.dspinFrequency = QtGui.QDoubleSpinBox(LiveDataDialog)
        self.dspinFrequency.setSingleStep(0.5)
        self.dspinFrequency.setObjectName(_fromUtf8("dspinFrequency"))
        self.layoutLive.setWidget(1, QtGui.QFormLayout.FieldRole, self.dspinFrequency)
        self.buttonsLive = QtGui.QDialogButtonBox(LiveDataDialog)
        self.buttonsLive.setOrientation(QtCore.Qt.Horizontal)
        self.buttonsLive.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonsLive.setObjectName(_fromUtf8("buttonsLive"))
        self.layoutLive.setWidget(2, QtGui.QFormLayout.SpanningRole, self.buttonsLive)
        self.labelAccMethod.setBuddy(self.comboAccMethod)
        self.labelFrequency.setBuddy(self.dspinFrequency)

        self.retranslateUi(LiveDataDialog)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("accepted()")), LiveDataDialog.accept)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("rejected()")), LiveDataDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(LiveDataDialog)

    def retranslateUi(self, LiveDataDialog):
        LiveDataDialog.setWindowTitle(QtGui.QApplication.translate("LiveDataDialog", "Live Data Options", None, QtGui.QApplication.UnicodeUTF8))
        self.labelAccMethod.setText(QtGui.QApplication.translate("LiveDataDialog", "Accumulation Method", None, QtGui.QApplication.UnicodeUTF8))
        self.labelFrequency.setText(QtGui.QApplication.translate("LiveDataDialog", "Update Every", None, QtGui.QApplication.UnicodeUTF8))

