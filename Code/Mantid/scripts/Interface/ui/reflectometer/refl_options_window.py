#pylint: disable=invalid-name
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'refl_options_window.ui'
#
# Created: Mon Sep  1 14:27:58 2014
#      by: PyQt4 UI code generator 4.10.4
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

class Ui_OptionsDialog(object):
    def setupUi(self, OptionsDialog):
        OptionsDialog.setObjectName(_fromUtf8("OptionsDialog"))
        OptionsDialog.resize(330, 194)
        self.layoutLive = QtGui.QFormLayout(OptionsDialog)
        self.layoutLive.setFieldGrowthPolicy(QtGui.QFormLayout.AllNonFixedFieldsGrow)
        self.layoutLive.setObjectName(_fromUtf8("layoutLive"))
        self.labelAccMethod = QtGui.QLabel(OptionsDialog)
        self.labelAccMethod.setObjectName(_fromUtf8("labelAccMethod"))
        self.layoutLive.setWidget(0, QtGui.QFormLayout.LabelRole, self.labelAccMethod)
        self.comboAccMethod = QtGui.QComboBox(OptionsDialog)
        self.comboAccMethod.setObjectName(_fromUtf8("comboAccMethod"))
        self.layoutLive.setWidget(0, QtGui.QFormLayout.FieldRole, self.comboAccMethod)
        self.labelFrequency = QtGui.QLabel(OptionsDialog)
        self.labelFrequency.setObjectName(_fromUtf8("labelFrequency"))
        self.layoutLive.setWidget(1, QtGui.QFormLayout.LabelRole, self.labelFrequency)
        self.dspinFrequency = QtGui.QDoubleSpinBox(OptionsDialog)
        self.dspinFrequency.setSingleStep(0.5)
        self.dspinFrequency.setObjectName(_fromUtf8("dspinFrequency"))
        self.layoutLive.setWidget(1, QtGui.QFormLayout.FieldRole, self.dspinFrequency)
        self.checkAlg = QtGui.QCheckBox(OptionsDialog)
        self.checkAlg.setObjectName(_fromUtf8("checkAlg"))
        self.layoutLive.setWidget(2, QtGui.QFormLayout.SpanningRole, self.checkAlg)
        self.checkICATDownload = QtGui.QCheckBox(OptionsDialog)
        self.checkICATDownload.setObjectName(_fromUtf8("checkICATDownload"))
        self.layoutLive.setWidget(3, QtGui.QFormLayout.SpanningRole, self.checkICATDownload)
        self.checkGroupTOFWorkspaces = QtGui.QCheckBox(OptionsDialog)
        self.checkGroupTOFWorkspaces.setObjectName(_fromUtf8("checkGroupTOFWorkspaces"))
        self.layoutLive.setWidget(4, QtGui.QFormLayout.SpanningRole, self.checkGroupTOFWorkspaces)
        self.buttonsLive = QtGui.QDialogButtonBox(OptionsDialog)
        self.buttonsLive.setOrientation(QtCore.Qt.Horizontal)
        self.buttonsLive.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonsLive.setObjectName(_fromUtf8("buttonsLive"))
        self.layoutLive.setWidget(5, QtGui.QFormLayout.SpanningRole, self.buttonsLive)
        self.labelAccMethod.setBuddy(self.comboAccMethod)
        self.labelFrequency.setBuddy(self.dspinFrequency)

        self.retranslateUi(OptionsDialog)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("accepted()")), OptionsDialog.accept)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("rejected()")), OptionsDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(OptionsDialog)

    def retranslateUi(self, OptionsDialog):
        OptionsDialog.setWindowTitle(_translate("OptionsDialog", "Refl Gui Options", None))
        self.labelAccMethod.setText(_translate("OptionsDialog", "Accumulation Method", None))
        self.labelFrequency.setText(_translate("OptionsDialog", "Update Every", None))
        self.checkAlg.setText(_translate("OptionsDialog", "Use ReflectometryReductionOneAuto Algorithm", None))
        self.checkICATDownload.setText(_translate("OptionsDialog", "Download Files Using ICAT", None))
        self.checkGroupTOFWorkspaces.setText(_translate("OptionsDialog", "Group TOF Workspaces", None))

