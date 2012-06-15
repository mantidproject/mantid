# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/inelastic/dgs_absolute_units.ui'
#
# Created: Fri Jun 15 11:50:04 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_AbsUnitsFrame(object):
    def setupUi(self, AbsUnitsFrame):
        AbsUnitsFrame.setObjectName(_fromUtf8("AbsUnitsFrame"))
        AbsUnitsFrame.resize(645, 585)
        AbsUnitsFrame.setFrameShape(QtGui.QFrame.StyledPanel)
        AbsUnitsFrame.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(AbsUnitsFrame)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.absunits_gb = QtGui.QGroupBox(AbsUnitsFrame)
        self.absunits_gb.setCheckable(True)
        self.absunits_gb.setChecked(False)
        self.absunits_gb.setObjectName(_fromUtf8("absunits_gb"))
        self.verticalLayout.addWidget(self.absunits_gb)

        self.retranslateUi(AbsUnitsFrame)
        QtCore.QMetaObject.connectSlotsByName(AbsUnitsFrame)

    def retranslateUi(self, AbsUnitsFrame):
        AbsUnitsFrame.setWindowTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.absunits_gb.setTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Perform Absolute Normalsation", None, QtGui.QApplication.UnicodeUTF8))

