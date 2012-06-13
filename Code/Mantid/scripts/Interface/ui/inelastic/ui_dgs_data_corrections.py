# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/inelastic/dgs_data_corrections.ui'
#
# Created: Wed Jun 13 11:57:33 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_DataCorrsFrame(object):
    def setupUi(self, DataCorrsFrame):
        DataCorrsFrame.setObjectName(_fromUtf8("DataCorrsFrame"))
        DataCorrsFrame.resize(586, 192)
        DataCorrsFrame.setFrameShape(QtGui.QFrame.StyledPanel)
        DataCorrsFrame.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(DataCorrsFrame)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.filter_bad_pulses_chkbox = QtGui.QCheckBox(DataCorrsFrame)
        self.filter_bad_pulses_chkbox.setObjectName(_fromUtf8("filter_bad_pulses_chkbox"))
        self.horizontalLayout.addWidget(self.filter_bad_pulses_chkbox)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        self.horizontalLayout_2.setObjectName(_fromUtf8("horizontalLayout_2"))
        self.incident_beam_norm_gb = QtGui.QGroupBox(DataCorrsFrame)
        self.incident_beam_norm_gb.setObjectName(_fromUtf8("incident_beam_norm_gb"))
        self.gridLayout = QtGui.QGridLayout(self.incident_beam_norm_gb)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.none_rb = QtGui.QRadioButton(self.incident_beam_norm_gb)
        self.none_rb.setChecked(True)
        self.none_rb.setObjectName(_fromUtf8("none_rb"))
        self.gridLayout.addWidget(self.none_rb, 0, 0, 1, 1)
        self.current_rb = QtGui.QRadioButton(self.incident_beam_norm_gb)
        self.current_rb.setObjectName(_fromUtf8("current_rb"))
        self.gridLayout.addWidget(self.current_rb, 0, 1, 1, 2)
        self.monitor1_rb = QtGui.QRadioButton(self.incident_beam_norm_gb)
        self.monitor1_rb.setObjectName(_fromUtf8("monitor1_rb"))
        self.gridLayout.addWidget(self.monitor1_rb, 1, 0, 1, 1)
        self.monint_label = QtGui.QLabel(self.incident_beam_norm_gb)
        self.monint_label.setObjectName(_fromUtf8("monint_label"))
        self.gridLayout.addWidget(self.monint_label, 1, 1, 1, 1)
        self.lineEdit = QtGui.QLineEdit(self.incident_beam_norm_gb)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.gridLayout.addWidget(self.lineEdit, 1, 2, 1, 1)
        self.lineEdit_2 = QtGui.QLineEdit(self.incident_beam_norm_gb)
        self.lineEdit_2.setObjectName(_fromUtf8("lineEdit_2"))
        self.gridLayout.addWidget(self.lineEdit_2, 1, 3, 1, 1)
        self.horizontalLayout_2.addWidget(self.incident_beam_norm_gb)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem1)
        self.verticalLayout.addLayout(self.horizontalLayout_2)
        spacerItem2 = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem2)

        self.retranslateUi(DataCorrsFrame)
        QtCore.QMetaObject.connectSlotsByName(DataCorrsFrame)

    def retranslateUi(self, DataCorrsFrame):
        DataCorrsFrame.setWindowTitle(QtGui.QApplication.translate("DataCorrsFrame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.filter_bad_pulses_chkbox.setText(QtGui.QApplication.translate("DataCorrsFrame", "Filter Bad Pulses", None, QtGui.QApplication.UnicodeUTF8))
        self.incident_beam_norm_gb.setTitle(QtGui.QApplication.translate("DataCorrsFrame", "Incident Beam Normalisation", None, QtGui.QApplication.UnicodeUTF8))
        self.none_rb.setText(QtGui.QApplication.translate("DataCorrsFrame", "None", None, QtGui.QApplication.UnicodeUTF8))
        self.current_rb.setToolTip(QtGui.QApplication.translate("DataCorrsFrame", "Current (aka Proton Charge) Normalisation", None, QtGui.QApplication.UnicodeUTF8))
        self.current_rb.setText(QtGui.QApplication.translate("DataCorrsFrame", "Current", None, QtGui.QApplication.UnicodeUTF8))
        self.monitor1_rb.setText(QtGui.QApplication.translate("DataCorrsFrame", "Monitor1", None, QtGui.QApplication.UnicodeUTF8))
        self.monint_label.setText(QtGui.QApplication.translate("DataCorrsFrame", "Integration Range:", None, QtGui.QApplication.UnicodeUTF8))

