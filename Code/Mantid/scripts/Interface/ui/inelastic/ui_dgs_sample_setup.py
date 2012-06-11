# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/inelastic/dgs_sample_setup.ui'
#
# Created: Mon Jun 11 10:35:34 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_Frame(object):
    def setupUi(self, Frame):
        Frame.setObjectName(_fromUtf8("Frame"))
        Frame.resize(783, 300)
        Frame.setFrameShape(QtGui.QFrame.StyledPanel)
        Frame.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(Frame)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.sample_label = QtGui.QLabel(Frame)
        self.sample_label.setObjectName(_fromUtf8("sample_label"))
        self.horizontalLayout.addWidget(self.sample_label)
        self.sample_edit = QtGui.QLineEdit(Frame)
        self.sample_edit.setObjectName(_fromUtf8("sample_edit"))
        self.horizontalLayout.addWidget(self.sample_edit)
        self.sample_browse = QtGui.QPushButton(Frame)
        self.sample_browse.setObjectName(_fromUtf8("sample_browse"))
        self.horizontalLayout.addWidget(self.sample_browse)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        self.horizontalLayout_2.setObjectName(_fromUtf8("horizontalLayout_2"))
        self.ei_label = QtGui.QLabel(Frame)
        self.ei_label.setObjectName(_fromUtf8("ei_label"))
        self.horizontalLayout_2.addWidget(self.ei_label)
        self.ei_edit = QtGui.QLineEdit(Frame)
        self.ei_edit.setObjectName(_fromUtf8("ei_edit"))
        self.horizontalLayout_2.addWidget(self.ei_edit)
        self.ei_units_label = QtGui.QLabel(Frame)
        self.ei_units_label.setObjectName(_fromUtf8("ei_units_label"))
        self.horizontalLayout_2.addWidget(self.ei_units_label)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem1)
        self.fixed_ei_chkbox = QtGui.QCheckBox(Frame)
        self.fixed_ei_chkbox.setObjectName(_fromUtf8("fixed_ei_chkbox"))
        self.horizontalLayout_2.addWidget(self.fixed_ei_chkbox)
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem2)
        self.verticalLayout.addLayout(self.horizontalLayout_2)
        self.et_range_box = QtGui.QGroupBox(Frame)
        self.et_range_box.setObjectName(_fromUtf8("et_range_box"))
        self.horizontalLayout_3 = QtGui.QHBoxLayout(self.et_range_box)
        self.horizontalLayout_3.setObjectName(_fromUtf8("horizontalLayout_3"))
        self.etr_low_label = QtGui.QLabel(self.et_range_box)
        self.etr_low_label.setObjectName(_fromUtf8("etr_low_label"))
        self.horizontalLayout_3.addWidget(self.etr_low_label)
        self.etr_low_edit = QtGui.QLineEdit(self.et_range_box)
        self.etr_low_edit.setObjectName(_fromUtf8("etr_low_edit"))
        self.horizontalLayout_3.addWidget(self.etr_low_edit)
        self.etr_width_label = QtGui.QLabel(self.et_range_box)
        self.etr_width_label.setObjectName(_fromUtf8("etr_width_label"))
        self.horizontalLayout_3.addWidget(self.etr_width_label)
        self.etr_width_edit = QtGui.QLineEdit(self.et_range_box)
        self.etr_width_edit.setObjectName(_fromUtf8("etr_width_edit"))
        self.horizontalLayout_3.addWidget(self.etr_width_edit)
        self.etr_high_label = QtGui.QLabel(self.et_range_box)
        self.etr_high_label.setObjectName(_fromUtf8("etr_high_label"))
        self.horizontalLayout_3.addWidget(self.etr_high_label)
        self.etr_high_edit = QtGui.QLineEdit(self.et_range_box)
        self.etr_high_edit.setObjectName(_fromUtf8("etr_high_edit"))
        self.horizontalLayout_3.addWidget(self.etr_high_edit)
        self.verticalLayout.addWidget(self.et_range_box)
        spacerItem3 = QtGui.QSpacerItem(20, 242, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem3)

        self.retranslateUi(Frame)
        QtCore.QMetaObject.connectSlotsByName(Frame)

    def retranslateUi(self, Frame):
        Frame.setWindowTitle(QtGui.QApplication.translate("Frame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.sample_label.setText(QtGui.QApplication.translate("Frame", "Sample Data", None, QtGui.QApplication.UnicodeUTF8))
        self.sample_browse.setText(QtGui.QApplication.translate("Frame", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.ei_label.setText(QtGui.QApplication.translate("Frame", "Incident Energy", None, QtGui.QApplication.UnicodeUTF8))
        self.ei_units_label.setText(QtGui.QApplication.translate("Frame", "meV", None, QtGui.QApplication.UnicodeUTF8))
        self.fixed_ei_chkbox.setText(QtGui.QApplication.translate("Frame", "Fixed Ei", None, QtGui.QApplication.UnicodeUTF8))
        self.et_range_box.setTitle(QtGui.QApplication.translate("Frame", "Energy Transfer Range (meV)", None, QtGui.QApplication.UnicodeUTF8))
        self.etr_low_label.setText(QtGui.QApplication.translate("Frame", "Low", None, QtGui.QApplication.UnicodeUTF8))
        self.etr_width_label.setText(QtGui.QApplication.translate("Frame", "Width", None, QtGui.QApplication.UnicodeUTF8))
        self.etr_high_label.setText(QtGui.QApplication.translate("Frame", "High", None, QtGui.QApplication.UnicodeUTF8))

