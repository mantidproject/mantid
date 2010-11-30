# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'reduction_main.ui'
#
# Created: Tue Nov 30 14:20:32 2010
#      by: PyQt4 UI code generator 4.7.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_SANSReduction(object):
    def setupUi(self, SANSReduction):
        SANSReduction.setObjectName("SANSReduction")
        SANSReduction.resize(872, 819)
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
        self.centralwidget.setObjectName("centralwidget")
        self.tabWidget = QtGui.QTabWidget(self.centralwidget)
        self.tabWidget.setGeometry(QtCore.QRect(20, 10, 831, 701))
        self.tabWidget.setObjectName("tabWidget")
        self.tab = QtGui.QWidget()
        self.tab.setObjectName("tab")
        self.tabWidget.addTab(self.tab, "")
        self.tab_2 = QtGui.QWidget()
        self.tab_2.setObjectName("tab_2")
        self.tabWidget.addTab(self.tab_2, "")
        self.reduce_button = QtGui.QPushButton(self.centralwidget)
        self.reduce_button.setGeometry(QtCore.QRect(650, 730, 93, 27))
        self.reduce_button.setObjectName("reduce_button")
        self.export_button = QtGui.QPushButton(self.centralwidget)
        self.export_button.setGeometry(QtCore.QRect(750, 730, 93, 27))
        self.export_button.setObjectName("export_button")
        SANSReduction.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(SANSReduction)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 872, 23))
        self.menubar.setObjectName("menubar")
        self.file_menu = QtGui.QMenu(self.menubar)
        self.file_menu.setObjectName("file_menu")
        self.tools_menu = QtGui.QMenu(self.menubar)
        self.tools_menu.setObjectName("tools_menu")
        SANSReduction.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(SANSReduction)
        self.statusbar.setObjectName("statusbar")
        SANSReduction.setStatusBar(self.statusbar)
        self.actionOpen = QtGui.QAction(SANSReduction)
        self.actionOpen.setObjectName("actionOpen")
        self.actionQuit = QtGui.QAction(SANSReduction)
        self.actionQuit.setObjectName("actionQuit")
        self.actionChange_Instrument = QtGui.QAction(SANSReduction)
        self.actionChange_Instrument.setObjectName("actionChange_Instrument")
        self.menubar.addAction(self.file_menu.menuAction())
        self.menubar.addAction(self.tools_menu.menuAction())

        self.retranslateUi(SANSReduction)
        self.tabWidget.setCurrentIndex(0)
        QtCore.QMetaObject.connectSlotsByName(SANSReduction)

    def retranslateUi(self, SANSReduction):
        SANSReduction.setWindowTitle(QtGui.QApplication.translate("SANSReduction", "SANS Reduction", None, QtGui.QApplication.UnicodeUTF8))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab), QtGui.QApplication.translate("SANSReduction", "Tab 1", None, QtGui.QApplication.UnicodeUTF8))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_2), QtGui.QApplication.translate("SANSReduction", "Tab 2", None, QtGui.QApplication.UnicodeUTF8))
        self.reduce_button.setToolTip(QtGui.QApplication.translate("SANSReduction", "Click to execute reduction.", None, QtGui.QApplication.UnicodeUTF8))
        self.reduce_button.setText(QtGui.QApplication.translate("SANSReduction", "Reduce", None, QtGui.QApplication.UnicodeUTF8))
        self.export_button.setToolTip(QtGui.QApplication.translate("SANSReduction", "Click to export the reduction parameters to a python script that can be run in MantidPlot.", None, QtGui.QApplication.UnicodeUTF8))
        self.export_button.setText(QtGui.QApplication.translate("SANSReduction", "Export", None, QtGui.QApplication.UnicodeUTF8))
        self.file_menu.setTitle(QtGui.QApplication.translate("SANSReduction", "File", None, QtGui.QApplication.UnicodeUTF8))
        self.tools_menu.setTitle(QtGui.QApplication.translate("SANSReduction", "Tools", None, QtGui.QApplication.UnicodeUTF8))
        self.actionOpen.setText(QtGui.QApplication.translate("SANSReduction", "Open...", None, QtGui.QApplication.UnicodeUTF8))
        self.actionOpen.setToolTip(QtGui.QApplication.translate("SANSReduction", "Open a reduction settings file", None, QtGui.QApplication.UnicodeUTF8))
        self.actionOpen.setShortcut(QtGui.QApplication.translate("SANSReduction", "Ctrl+O", None, QtGui.QApplication.UnicodeUTF8))
        self.actionQuit.setText(QtGui.QApplication.translate("SANSReduction", "Quit", None, QtGui.QApplication.UnicodeUTF8))
        self.actionQuit.setShortcut(QtGui.QApplication.translate("SANSReduction", "Ctrl+Q, Ctrl+S", None, QtGui.QApplication.UnicodeUTF8))
        self.actionChange_Instrument.setText(QtGui.QApplication.translate("SANSReduction", "Change Instrument", None, QtGui.QApplication.UnicodeUTF8))
        self.actionChange_Instrument.setShortcut(QtGui.QApplication.translate("SANSReduction", "Ctrl+I", None, QtGui.QApplication.UnicodeUTF8))

