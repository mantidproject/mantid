# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'refl_options_window.ui'
#
# Created: Thu Jul  3 10:27:05 2014
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_OptionsDialog(object):
    def setupUi(self, OptionsDialog):
        OptionsDialog.setObjectName(_fromUtf8("OptionsDialog"))
        OptionsDialog.resize(377, 227)
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
        self.layoutLive.setWidget(3, QtGui.QFormLayout.SpanningRole, self.checkAlg)
        self.checkADS = QtGui.QCheckBox(OptionsDialog)
        self.checkADS.setObjectName(_fromUtf8("checkADS"))
        self.layoutLive.setWidget(4, QtGui.QFormLayout.SpanningRole, self.checkADS)
        self.checkICATSearch = QtGui.QCheckBox(OptionsDialog)
        self.checkICATSearch.setObjectName(_fromUtf8("checkICATSearch"))
        self.layoutLive.setWidget(6, QtGui.QFormLayout.SpanningRole, self.checkICATSearch)
        self.buttonsLive = QtGui.QDialogButtonBox(OptionsDialog)
        self.buttonsLive.setOrientation(QtCore.Qt.Horizontal)
        self.buttonsLive.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonsLive.setObjectName(_fromUtf8("buttonsLive"))
        self.layoutLive.setWidget(8, QtGui.QFormLayout.SpanningRole, self.buttonsLive)
        self.checkICATDownload = QtGui.QCheckBox(OptionsDialog)
        self.checkICATDownload.setObjectName(_fromUtf8("checkICATDownload"))
        self.layoutLive.setWidget(7, QtGui.QFormLayout.SpanningRole, self.checkICATDownload)
        self.labelAccMethod.setBuddy(self.comboAccMethod)
        self.labelFrequency.setBuddy(self.dspinFrequency)

        self.retranslateUi(OptionsDialog)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("accepted()")), OptionsDialog.accept)
        QtCore.QObject.connect(self.buttonsLive, QtCore.SIGNAL(_fromUtf8("rejected()")), OptionsDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(OptionsDialog)

    def retranslateUi(self, OptionsDialog):
        OptionsDialog.setWindowTitle(QtGui.QApplication.translate("OptionsDialog", "Refl Gui Options", None, QtGui.QApplication.UnicodeUTF8))
        self.labelAccMethod.setText(QtGui.QApplication.translate("OptionsDialog", "Accumulation Method", None, QtGui.QApplication.UnicodeUTF8))
        self.labelFrequency.setText(QtGui.QApplication.translate("OptionsDialog", "Update Every", None, QtGui.QApplication.UnicodeUTF8))
        self.checkAlg.setText(QtGui.QApplication.translate("OptionsDialog", "Use ReflectometryReductionOneAuto Algorithm", None, QtGui.QApplication.UnicodeUTF8))
        self.checkADS.setText(QtGui.QApplication.translate("OptionsDialog", "Load workspaces from mantid into the Runs list", None, QtGui.QApplication.UnicodeUTF8))
        self.checkICATSearch.setText(QtGui.QApplication.translate("OptionsDialog", "RB Search Using ICAT", None, QtGui.QApplication.UnicodeUTF8))
        self.checkICATDownload.setText(QtGui.QApplication.translate("OptionsDialog", "Download Files Using ICAT", None, QtGui.QApplication.UnicodeUTF8))

