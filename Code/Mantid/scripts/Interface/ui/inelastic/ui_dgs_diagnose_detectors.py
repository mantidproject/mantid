# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/inelastic/dgs_diagnose_detectors.ui'
#
# Created: Thu Jun 14 14:18:29 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_DiagDetsFrame(object):
    def setupUi(self, DiagDetsFrame):
        DiagDetsFrame.setObjectName(_fromUtf8("DiagDetsFrame"))
        DiagDetsFrame.resize(592, 743)
        DiagDetsFrame.setFrameShape(QtGui.QFrame.StyledPanel)
        DiagDetsFrame.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(DiagDetsFrame)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.find_bad_det_gb = QtGui.QGroupBox(DiagDetsFrame)
        self.find_bad_det_gb.setCheckable(True)
        self.find_bad_det_gb.setChecked(False)
        self.find_bad_det_gb.setObjectName(_fromUtf8("find_bad_det_gb"))
        self.verticalLayout.addWidget(self.find_bad_det_gb)

        self.retranslateUi(DiagDetsFrame)
        QtCore.QMetaObject.connectSlotsByName(DiagDetsFrame)

    def retranslateUi(self, DiagDetsFrame):
        DiagDetsFrame.setWindowTitle(QtGui.QApplication.translate("DiagDetsFrame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.find_bad_det_gb.setTitle(QtGui.QApplication.translate("DiagDetsFrame", "Find Bad Detectors", None, QtGui.QApplication.UnicodeUTF8))

