# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/inelastic/dgs_absolute_units.ui'
#
# Created: Thu Oct 18 13:59:20 2012
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
        AbsUnitsFrame.resize(1011, 704)
        AbsUnitsFrame.setFrameShape(QtGui.QFrame.StyledPanel)
        AbsUnitsFrame.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout_6 = QtGui.QVBoxLayout(AbsUnitsFrame)
        self.verticalLayout_6.setObjectName(_fromUtf8("verticalLayout_6"))
        self.absunits_gb = QtGui.QGroupBox(AbsUnitsFrame)
        self.absunits_gb.setCheckable(True)
        self.absunits_gb.setChecked(False)
        self.absunits_gb.setObjectName(_fromUtf8("absunits_gb"))
        self.verticalLayout_5 = QtGui.QVBoxLayout(self.absunits_gb)
        self.verticalLayout_5.setObjectName(_fromUtf8("verticalLayout_5"))
        self.run_files_gb = QtGui.QGroupBox(self.absunits_gb)
        self.run_files_gb.setObjectName(_fromUtf8("run_files_gb"))
        self.verticalLayout = QtGui.QVBoxLayout(self.run_files_gb)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.absunits_van_label = QtGui.QLabel(self.run_files_gb)
        self.absunits_van_label.setMinimumSize(QtCore.QSize(225, 0))
        self.absunits_van_label.setObjectName(_fromUtf8("absunits_van_label"))
        self.horizontalLayout.addWidget(self.absunits_van_label)
        self.absunits_van_edit = QtGui.QLineEdit(self.run_files_gb)
        self.absunits_van_edit.setObjectName(_fromUtf8("absunits_van_edit"))
        self.horizontalLayout.addWidget(self.absunits_van_edit)
        self.absunits_van_browse = QtGui.QPushButton(self.run_files_gb)
        self.absunits_van_browse.setObjectName(_fromUtf8("absunits_van_browse"))
        self.horizontalLayout.addWidget(self.absunits_van_browse)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        self.horizontalLayout_2.setObjectName(_fromUtf8("horizontalLayout_2"))
        self.grouping_file_label = QtGui.QLabel(self.run_files_gb)
        self.grouping_file_label.setMinimumSize(QtCore.QSize(225, 0))
        self.grouping_file_label.setObjectName(_fromUtf8("grouping_file_label"))
        self.horizontalLayout_2.addWidget(self.grouping_file_label)
        self.grouping_file_edit = QtGui.QLineEdit(self.run_files_gb)
        self.grouping_file_edit.setObjectName(_fromUtf8("grouping_file_edit"))
        self.horizontalLayout_2.addWidget(self.grouping_file_edit)
        self.grouping_file_browse = QtGui.QPushButton(self.run_files_gb)
        self.grouping_file_browse.setObjectName(_fromUtf8("grouping_file_browse"))
        self.horizontalLayout_2.addWidget(self.grouping_file_browse)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_2.addItem(spacerItem1)
        self.verticalLayout.addLayout(self.horizontalLayout_2)
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.horizontalLayout_3.setObjectName(_fromUtf8("horizontalLayout_3"))
        self.absunits_detvan_label = QtGui.QLabel(self.run_files_gb)
        self.absunits_detvan_label.setMinimumSize(QtCore.QSize(225, 0))
        self.absunits_detvan_label.setObjectName(_fromUtf8("absunits_detvan_label"))
        self.horizontalLayout_3.addWidget(self.absunits_detvan_label)
        self.absunits_detvan_edit = QtGui.QLineEdit(self.run_files_gb)
        self.absunits_detvan_edit.setObjectName(_fromUtf8("absunits_detvan_edit"))
        self.horizontalLayout_3.addWidget(self.absunits_detvan_edit)
        self.absunits_detvan_browse = QtGui.QPushButton(self.run_files_gb)
        self.absunits_detvan_browse.setObjectName(_fromUtf8("absunits_detvan_browse"))
        self.horizontalLayout_3.addWidget(self.absunits_detvan_browse)
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_3.addItem(spacerItem2)
        self.verticalLayout.addLayout(self.horizontalLayout_3)
        self.verticalLayout_5.addWidget(self.run_files_gb)
        self.integration_gb = QtGui.QGroupBox(self.absunits_gb)
        self.integration_gb.setObjectName(_fromUtf8("integration_gb"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.integration_gb)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.horizontalLayout_4 = QtGui.QHBoxLayout()
        self.horizontalLayout_4.setObjectName(_fromUtf8("horizontalLayout_4"))
        self.ei_label = QtGui.QLabel(self.integration_gb)
        self.ei_label.setMinimumSize(QtCore.QSize(225, 0))
        self.ei_label.setObjectName(_fromUtf8("ei_label"))
        self.horizontalLayout_4.addWidget(self.ei_label)
        self.ei_edit = QtGui.QLineEdit(self.integration_gb)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ei_edit.sizePolicy().hasHeightForWidth())
        self.ei_edit.setSizePolicy(sizePolicy)
        self.ei_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.ei_edit.setObjectName(_fromUtf8("ei_edit"))
        self.horizontalLayout_4.addWidget(self.ei_edit)
        spacerItem3 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_4.addItem(spacerItem3)
        self.verticalLayout_2.addLayout(self.horizontalLayout_4)
        self.horizontalLayout_5 = QtGui.QHBoxLayout()
        self.horizontalLayout_5.setObjectName(_fromUtf8("horizontalLayout_5"))
        self.emin_label = QtGui.QLabel(self.integration_gb)
        self.emin_label.setMinimumSize(QtCore.QSize(225, 0))
        self.emin_label.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.emin_label.setObjectName(_fromUtf8("emin_label"))
        self.horizontalLayout_5.addWidget(self.emin_label)
        self.emin_edit = QtGui.QLineEdit(self.integration_gb)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.emin_edit.sizePolicy().hasHeightForWidth())
        self.emin_edit.setSizePolicy(sizePolicy)
        self.emin_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.emin_edit.setObjectName(_fromUtf8("emin_edit"))
        self.horizontalLayout_5.addWidget(self.emin_edit)
        spacerItem4 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_5.addItem(spacerItem4)
        self.emax_label = QtGui.QLabel(self.integration_gb)
        self.emax_label.setObjectName(_fromUtf8("emax_label"))
        self.horizontalLayout_5.addWidget(self.emax_label)
        self.emax_edit = QtGui.QLineEdit(self.integration_gb)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.emax_edit.sizePolicy().hasHeightForWidth())
        self.emax_edit.setSizePolicy(sizePolicy)
        self.emax_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.emax_edit.setObjectName(_fromUtf8("emax_edit"))
        self.horizontalLayout_5.addWidget(self.emax_edit)
        spacerItem5 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_5.addItem(spacerItem5)
        self.verticalLayout_2.addLayout(self.horizontalLayout_5)
        self.verticalLayout_5.addWidget(self.integration_gb)
        self.masses_gb = QtGui.QGroupBox(self.absunits_gb)
        self.masses_gb.setObjectName(_fromUtf8("masses_gb"))
        self.verticalLayout_3 = QtGui.QVBoxLayout(self.masses_gb)
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        self.horizontalLayout_6 = QtGui.QHBoxLayout()
        self.horizontalLayout_6.setObjectName(_fromUtf8("horizontalLayout_6"))
        self.van_mass_label = QtGui.QLabel(self.masses_gb)
        self.van_mass_label.setMinimumSize(QtCore.QSize(225, 0))
        self.van_mass_label.setObjectName(_fromUtf8("van_mass_label"))
        self.horizontalLayout_6.addWidget(self.van_mass_label)
        self.van_mass_edit = QtGui.QLineEdit(self.masses_gb)
        self.van_mass_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.van_mass_edit.setObjectName(_fromUtf8("van_mass_edit"))
        self.horizontalLayout_6.addWidget(self.van_mass_edit)
        spacerItem6 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem6)
        self.verticalLayout_3.addLayout(self.horizontalLayout_6)
        self.horizontalLayout_7 = QtGui.QHBoxLayout()
        self.horizontalLayout_7.setObjectName(_fromUtf8("horizontalLayout_7"))
        self.sample_mass_label = QtGui.QLabel(self.masses_gb)
        self.sample_mass_label.setMinimumSize(QtCore.QSize(225, 0))
        self.sample_mass_label.setObjectName(_fromUtf8("sample_mass_label"))
        self.horizontalLayout_7.addWidget(self.sample_mass_label)
        self.sample_mass_edit = QtGui.QLineEdit(self.masses_gb)
        self.sample_mass_edit.setMinimumSize(QtCore.QSize(0, 0))
        self.sample_mass_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.sample_mass_edit.setObjectName(_fromUtf8("sample_mass_edit"))
        self.horizontalLayout_7.addWidget(self.sample_mass_edit)
        spacerItem7 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_7.addItem(spacerItem7)
        self.verticalLayout_3.addLayout(self.horizontalLayout_7)
        self.horizontalLayout_8 = QtGui.QHBoxLayout()
        self.horizontalLayout_8.setObjectName(_fromUtf8("horizontalLayout_8"))
        self.sample_rmm_label = QtGui.QLabel(self.masses_gb)
        self.sample_rmm_label.setMinimumSize(QtCore.QSize(225, 0))
        self.sample_rmm_label.setObjectName(_fromUtf8("sample_rmm_label"))
        self.horizontalLayout_8.addWidget(self.sample_rmm_label)
        self.sample_rmm_edit = QtGui.QLineEdit(self.masses_gb)
        self.sample_rmm_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.sample_rmm_edit.setObjectName(_fromUtf8("sample_rmm_edit"))
        self.horizontalLayout_8.addWidget(self.sample_rmm_edit)
        spacerItem8 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_8.addItem(spacerItem8)
        self.verticalLayout_3.addLayout(self.horizontalLayout_8)
        self.verticalLayout_5.addWidget(self.masses_gb)
        self.diag_gb = QtGui.QGroupBox(self.absunits_gb)
        self.diag_gb.setObjectName(_fromUtf8("diag_gb"))
        self.verticalLayout_4 = QtGui.QVBoxLayout(self.diag_gb)
        self.verticalLayout_4.setObjectName(_fromUtf8("verticalLayout_4"))
        self.horizontalLayout_10 = QtGui.QHBoxLayout()
        self.horizontalLayout_10.setObjectName(_fromUtf8("horizontalLayout_10"))
        self.median_test_out_high_label = QtGui.QLabel(self.diag_gb)
        self.median_test_out_high_label.setMinimumSize(QtCore.QSize(137, 0))
        self.median_test_out_high_label.setObjectName(_fromUtf8("median_test_out_high_label"))
        self.horizontalLayout_10.addWidget(self.median_test_out_high_label)
        self.median_test_out_high_edit = QtGui.QLineEdit(self.diag_gb)
        self.median_test_out_high_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.median_test_out_high_edit.setObjectName(_fromUtf8("median_test_out_high_edit"))
        self.horizontalLayout_10.addWidget(self.median_test_out_high_edit)
        self.median_test_out_low_label = QtGui.QLabel(self.diag_gb)
        self.median_test_out_low_label.setMinimumSize(QtCore.QSize(132, 0))
        self.median_test_out_low_label.setObjectName(_fromUtf8("median_test_out_low_label"))
        self.horizontalLayout_10.addWidget(self.median_test_out_low_label)
        self.median_test_out_low_edit = QtGui.QLineEdit(self.diag_gb)
        self.median_test_out_low_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.median_test_out_low_edit.setObjectName(_fromUtf8("median_test_out_low_edit"))
        self.horizontalLayout_10.addWidget(self.median_test_out_low_edit)
        spacerItem9 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_10.addItem(spacerItem9)
        self.verticalLayout_4.addLayout(self.horizontalLayout_10)
        self.horizontalLayout_11 = QtGui.QHBoxLayout()
        self.horizontalLayout_11.setObjectName(_fromUtf8("horizontalLayout_11"))
        self.median_test_high_label = QtGui.QLabel(self.diag_gb)
        self.median_test_high_label.setMinimumSize(QtCore.QSize(137, 0))
        self.median_test_high_label.setObjectName(_fromUtf8("median_test_high_label"))
        self.horizontalLayout_11.addWidget(self.median_test_high_label)
        self.median_test_high_edit = QtGui.QLineEdit(self.diag_gb)
        self.median_test_high_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.median_test_high_edit.setObjectName(_fromUtf8("median_test_high_edit"))
        self.horizontalLayout_11.addWidget(self.median_test_high_edit)
        self.median_test_low_label = QtGui.QLabel(self.diag_gb)
        self.median_test_low_label.setMinimumSize(QtCore.QSize(132, 0))
        self.median_test_low_label.setObjectName(_fromUtf8("median_test_low_label"))
        self.horizontalLayout_11.addWidget(self.median_test_low_label)
        self.median_test_low_edit = QtGui.QLineEdit(self.diag_gb)
        self.median_test_low_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.median_test_low_edit.setObjectName(_fromUtf8("median_test_low_edit"))
        self.horizontalLayout_11.addWidget(self.median_test_low_edit)
        spacerItem10 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_11.addItem(spacerItem10)
        self.verticalLayout_4.addLayout(self.horizontalLayout_11)
        self.horizontalLayout_12 = QtGui.QHBoxLayout()
        self.horizontalLayout_12.setObjectName(_fromUtf8("horizontalLayout_12"))
        self.errorbar_crit_label = QtGui.QLabel(self.diag_gb)
        self.errorbar_crit_label.setMinimumSize(QtCore.QSize(137, 0))
        self.errorbar_crit_label.setObjectName(_fromUtf8("errorbar_crit_label"))
        self.horizontalLayout_12.addWidget(self.errorbar_crit_label)
        self.errorbar_crit_edit = QtGui.QLineEdit(self.diag_gb)
        self.errorbar_crit_edit.setMaximumSize(QtCore.QSize(100, 16777215))
        self.errorbar_crit_edit.setObjectName(_fromUtf8("errorbar_crit_edit"))
        self.horizontalLayout_12.addWidget(self.errorbar_crit_edit)
        spacerItem11 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_12.addItem(spacerItem11)
        self.verticalLayout_4.addLayout(self.horizontalLayout_12)
        self.verticalLayout_5.addWidget(self.diag_gb)
        self.verticalLayout_6.addWidget(self.absunits_gb)
        spacerItem12 = QtGui.QSpacerItem(962, 81, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_6.addItem(spacerItem12)

        self.retranslateUi(AbsUnitsFrame)
        QtCore.QMetaObject.connectSlotsByName(AbsUnitsFrame)

    def retranslateUi(self, AbsUnitsFrame):
        AbsUnitsFrame.setWindowTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Frame", None, QtGui.QApplication.UnicodeUTF8))
        self.absunits_gb.setTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Perform Absolute Normalisation", None, QtGui.QApplication.UnicodeUTF8))
        self.run_files_gb.setTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Run Files", None, QtGui.QApplication.UnicodeUTF8))
        self.absunits_van_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "AbsUnits Vanadium", None, QtGui.QApplication.UnicodeUTF8))
        self.absunits_van_browse.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.grouping_file_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Grouping File", None, QtGui.QApplication.UnicodeUTF8))
        self.grouping_file_browse.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.absunits_detvan_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Detector Vanadium (Abs Units)", None, QtGui.QApplication.UnicodeUTF8))
        self.absunits_detvan_browse.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Browse", None, QtGui.QApplication.UnicodeUTF8))
        self.integration_gb.setTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Integration (meV)", None, QtGui.QApplication.UnicodeUTF8))
        self.ei_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Incident Energy", None, QtGui.QApplication.UnicodeUTF8))
        self.emin_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Energy range                             E Min", None, QtGui.QApplication.UnicodeUTF8))
        self.emax_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "E Max", None, QtGui.QApplication.UnicodeUTF8))
        self.masses_gb.setTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Masses for Absolute Units", None, QtGui.QApplication.UnicodeUTF8))
        self.van_mass_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Vanadium Mass", None, QtGui.QApplication.UnicodeUTF8))
        self.sample_mass_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Sample Mass", None, QtGui.QApplication.UnicodeUTF8))
        self.sample_rmm_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Sample RMM", None, QtGui.QApplication.UnicodeUTF8))
        self.diag_gb.setTitle(QtGui.QApplication.translate("AbsUnitsFrame", "Diagnostic Parameters", None, QtGui.QApplication.UnicodeUTF8))
        self.median_test_out_high_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Median high outliers", None, QtGui.QApplication.UnicodeUTF8))
        self.median_test_out_low_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Median low outliers", None, QtGui.QApplication.UnicodeUTF8))
        self.median_test_high_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Median test high", None, QtGui.QApplication.UnicodeUTF8))
        self.median_test_low_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Median test low", None, QtGui.QApplication.UnicodeUTF8))
        self.errorbar_crit_label.setText(QtGui.QApplication.translate("AbsUnitsFrame", "Errorbar criterion", None, QtGui.QApplication.UnicodeUTF8))

