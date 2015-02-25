#pylint: disable=invalid-name
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '/Users/jon/QtSDK/pychopui.ui'
#
# Created: Mon May 28 23:37:52 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName(_fromUtf8("MainWindow"))
        MainWindow.resize(729, 530)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.calc = QtGui.QPushButton(self.centralwidget)
        self.calc.setGeometry(QtCore.QRect(600, 70, 114, 32))
        self.calc.setObjectName(_fromUtf8("calc"))
        self.disp = QtGui.QListWidget(self.centralwidget)
        self.disp.setGeometry(QtCore.QRect(80, 20, 491, 451))
        self.disp.setObjectName(_fromUtf8("disp"))
        self.ei = QtGui.QLineEdit(self.centralwidget)
        self.ei.setGeometry(QtCore.QRect(600, 40, 113, 22))
        self.ei.setObjectName(_fromUtf8("ei"))
        self.label = QtGui.QLabel(self.centralwidget)
        self.label.setGeometry(QtCore.QRect(601, 20, 101, 16))
        self.label.setObjectName(_fromUtf8("label"))
        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(MainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 729, 22))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        self.menuInstrument = QtGui.QMenu(self.menubar)
        self.menuInstrument.setObjectName(_fromUtf8("menuInstrument"))
        self.menuChopper = QtGui.QMenu(self.menubar)
        self.menuChopper.setObjectName(_fromUtf8("menuChopper"))
        MainWindow.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(MainWindow)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        MainWindow.setStatusBar(self.statusbar)
        self.actionMari = QtGui.QAction(MainWindow)
        self.actionMari.setObjectName(_fromUtf8("actionMari"))
        self.actionMaps = QtGui.QAction(MainWindow)
        self.actionMaps.setObjectName(_fromUtf8("actionMaps"))
        self.actionMerlin = QtGui.QAction(MainWindow)
        self.actionMerlin.setObjectName(_fromUtf8("actionMerlin"))
        self.actionGadolinium = QtGui.QAction(MainWindow)
        self.actionGadolinium.setObjectName(_fromUtf8("actionGadolinium"))
        self.actionSloppy = QtGui.QAction(MainWindow)
        self.actionSloppy.setObjectName(_fromUtf8("actionSloppy"))
        self.actionA = QtGui.QAction(MainWindow)
        self.actionA.setObjectName(_fromUtf8("actionA"))
        self.actionRType = QtGui.QAction(MainWindow)
        self.actionRType.setObjectName(_fromUtf8("actionRType"))
        self.menuInstrument.addAction(self.actionMari)
        self.menuInstrument.addAction(self.actionMaps)
        self.menuInstrument.addAction(self.actionMerlin)
        self.menuChopper.addAction(self.actionGadolinium)
        self.menuChopper.addAction(self.actionSloppy)
        self.menuChopper.addAction(self.actionA)
        self.menuChopper.addAction(self.actionRType)
        self.menubar.addAction(self.menuInstrument.menuAction())
        self.menubar.addAction(self.menuChopper.menuAction())

        self.retranslateUi(MainWindow)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QtGui.QApplication.translate("MainWindow", "MainWindow", None, QtGui.QApplication.UnicodeUTF8))
        self.calc.setText(QtGui.QApplication.translate("MainWindow", "Calculate", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("MainWindow", "Incident Energy", None, QtGui.QApplication.UnicodeUTF8))
        self.menuInstrument.setTitle(QtGui.QApplication.translate("MainWindow", "Instrument", None, QtGui.QApplication.UnicodeUTF8))
        self.menuChopper.setTitle(QtGui.QApplication.translate("MainWindow", "Chopper", None, QtGui.QApplication.UnicodeUTF8))
        self.actionMari.setText(QtGui.QApplication.translate("MainWindow", "Mari", None, QtGui.QApplication.UnicodeUTF8))
        self.actionMaps.setText(QtGui.QApplication.translate("MainWindow", "Maps", None, QtGui.QApplication.UnicodeUTF8))
        self.actionMerlin.setText(QtGui.QApplication.translate("MainWindow", "Merlin", None, QtGui.QApplication.UnicodeUTF8))
        self.actionGadolinium.setText(QtGui.QApplication.translate("MainWindow", "Gadolinium", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSloppy.setText(QtGui.QApplication.translate("MainWindow", "Sloppy", None, QtGui.QApplication.UnicodeUTF8))
        self.actionA.setText(QtGui.QApplication.translate("MainWindow", "AType", None, QtGui.QApplication.UnicodeUTF8))
        self.actionRType.setText(QtGui.QApplication.translate("MainWindow", "RType", None, QtGui.QApplication.UnicodeUTF8))

