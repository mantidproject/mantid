#pylint: disable=invalid-name,attribute-defined-outside-init,line-too-long,too-many-instance-attributes
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'reduction_main.ui'
#
# Created: Tue Oct  1 13:28:56 2013
#      by: PyQt4 UI code generator 4.10.3
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

class Ui_SANSReduction(object):
    def setupUi(self, SANSReduction):
        SANSReduction.setObjectName(_fromUtf8("SANSReduction"))
        SANSReduction.resize(1062, 989)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(SANSReduction.sizePolicy().hasHeightForWidth())
        SANSReduction.setSizePolicy(sizePolicy)
        self.centralwidget = QtGui.QWidget(SANSReduction)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.centralwidget.sizePolicy().hasHeightForWidth())
        self.centralwidget.setSizePolicy(sizePolicy)
        self.centralwidget.setAutoFillBackground(True)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.tabWidget = QtGui.QTabWidget(self.centralwidget)
        self.tabWidget.setObjectName(_fromUtf8("tabWidget"))
        self.tab = QtGui.QWidget()
        self.tab.setObjectName(_fromUtf8("tab"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.tab)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.label = QtGui.QLabel(self.tab)
        self.label.setObjectName(_fromUtf8("label"))
        self.verticalLayout_2.addWidget(self.label)
        self.tabWidget.addTab(self.tab, _fromUtf8(""))
        self.verticalLayout.addWidget(self.tabWidget)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.interface_chk = QtGui.QCheckBox(self.centralwidget)
        self.interface_chk.setEnabled(True)
        self.interface_chk.setObjectName(_fromUtf8("interface_chk"))
        self.horizontalLayout.addWidget(self.interface_chk)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.progress_bar = QtGui.QProgressBar(self.centralwidget)
        self.progress_bar.setProperty("value", 0)
        self.progress_bar.setObjectName(_fromUtf8("progress_bar"))
        self.horizontalLayout.addWidget(self.progress_bar)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem1)
        self.reduce_button = QtGui.QPushButton(self.centralwidget)
        self.reduce_button.setEnabled(True)
        self.reduce_button.setMinimumSize(QtCore.QSize(95, 0))
        self.reduce_button.setObjectName(_fromUtf8("reduce_button"))
        self.horizontalLayout.addWidget(self.reduce_button)
        self.cluster_button = QtGui.QPushButton(self.centralwidget)
        self.cluster_button.setMinimumSize(QtCore.QSize(95, 0))
        self.cluster_button.setObjectName(_fromUtf8("cluster_button"))
        self.horizontalLayout.addWidget(self.cluster_button)
        self.save_button = QtGui.QPushButton(self.centralwidget)
        self.save_button.setEnabled(True)
        self.save_button.setMinimumSize(QtCore.QSize(95, 0))
        self.save_button.setObjectName(_fromUtf8("save_button"))
        self.horizontalLayout.addWidget(self.save_button)
        self.export_button = QtGui.QPushButton(self.centralwidget)
        self.export_button.setEnabled(True)
        self.export_button.setMinimumSize(QtCore.QSize(95, 0))
        self.export_button.setObjectName(_fromUtf8("export_button"))
        self.horizontalLayout.addWidget(self.export_button)
        self.verticalLayout.addLayout(self.horizontalLayout)
        SANSReduction.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(SANSReduction)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 1062, 23))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        self.file_menu = QtGui.QMenu(self.menubar)
        self.file_menu.setObjectName(_fromUtf8("file_menu"))
        self.tools_menu = QtGui.QMenu(self.menubar)
        self.tools_menu.setObjectName(_fromUtf8("tools_menu"))
        SANSReduction.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(SANSReduction)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        SANSReduction.setStatusBar(self.statusbar)
        self.actionOpen = QtGui.QAction(SANSReduction)
        self.actionOpen.setObjectName(_fromUtf8("actionOpen"))
        self.actionQuit = QtGui.QAction(SANSReduction)
        self.actionQuit.setObjectName(_fromUtf8("actionQuit"))
        self.actionChange_Instrument = QtGui.QAction(SANSReduction)
        self.actionChange_Instrument.setObjectName(_fromUtf8("actionChange_Instrument"))
        self.menubar.addAction(self.file_menu.menuAction())
        self.menubar.addAction(self.tools_menu.menuAction())

        self.retranslateUi(SANSReduction)
        self.tabWidget.setCurrentIndex(0)
        QtCore.QMetaObject.connectSlotsByName(SANSReduction)

    def retranslateUi(self, SANSReduction):
        SANSReduction.setWindowTitle(_translate("SANSReduction", "SANS Reduction", None))
        self.label.setText(_translate("SANSReduction", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"\
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:\'Ubuntu\'; font-size:11pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:14pt; font-weight:600;\">No instrument was selected!</span></p>\n"
"<p align=\"center\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p>\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:12pt;\">You need to select an instrument from the Tools menu</span></p>\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:12pt;\"> or the Instrument Dialog to continue.</span></p></body></html>", None))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab), _translate("SANSReduction", "No Instrument Selected", None))
        self.interface_chk.setText(_translate("SANSReduction", "Advanced interface", None))
        self.reduce_button.setToolTip(_translate("SANSReduction", "Click to execute reduction.", None))
        self.reduce_button.setText(_translate("SANSReduction", "Reduce", None))
        self.cluster_button.setToolTip(_translate("SANSReduction", "Click to send the reduction job to a remote compute resource", None))
        self.cluster_button.setText(_translate("SANSReduction", "Send cluster", None))
        self.save_button.setToolTip(_translate("SANSReduction", "Click to save your reduction parameters.", None))
        self.save_button.setText(_translate("SANSReduction", "Save", None))
        self.export_button.setToolTip(_translate("SANSReduction", "Click to export the reduction parameters to a python script that can be run in MantidPlot.", None))
        self.export_button.setText(_translate("SANSReduction", "Export", None))
        self.file_menu.setTitle(_translate("SANSReduction", "File", None))
        self.tools_menu.setTitle(_translate("SANSReduction", "Tools", None))
        self.actionOpen.setText(_translate("SANSReduction", "Open...", None))
        self.actionOpen.setToolTip(_translate("SANSReduction", "Open a reduction settings file", None))
        self.actionOpen.setShortcut(_translate("SANSReduction", "Ctrl+O", None))
        self.actionQuit.setText(_translate("SANSReduction", "Quit", None))
        self.actionQuit.setShortcut(_translate("SANSReduction", "Ctrl+Q, Ctrl+S", None))
        self.actionChange_Instrument.setText(_translate("SANSReduction", "Change Instrument", None))
        self.actionChange_Instrument.setShortcut(_translate("SANSReduction", "Ctrl+I", None))

