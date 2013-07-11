# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'd:\Data\Mantid_GIT\Code\Mantid\scripts\PyChop_LET\PyChop_LET_UI.ui'
#
# Created: Tue Jul 09 14:28:50 2013
#      by: PyQt4 UI code generator 4.8.3
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
        MainWindow.resize(453, 590)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.calcButton = QtGui.QPushButton(self.centralwidget)
        self.calcButton.setGeometry(QtCore.QRect(20, 70, 171, 23))
        self.calcButton.setObjectName(_fromUtf8("calcButton"))
        self.incidentEnergyValue = QtGui.QLineEdit(self.centralwidget)
        self.incidentEnergyValue.setGeometry(QtCore.QRect(40, 40, 113, 20))
        self.incidentEnergyValue.setText(_fromUtf8(""))
        self.incidentEnergyValue.setPlaceholderText(_fromUtf8(""))
        self.incidentEnergyValue.setObjectName(_fromUtf8("incidentEnergyValue"))
        self.label = QtGui.QLabel(self.centralwidget)
        self.label.setGeometry(QtCore.QRect(30, 10, 141, 20))
        self.label.setObjectName(_fromUtf8("label"))
        self.list = QtGui.QListWidget(self.centralwidget)
        self.list.setGeometry(QtCore.QRect(20, 100, 401, 421))
        self.list.setObjectName(_fromUtf8("list"))
        self.Plot = QtGui.QPushButton(self.centralwidget)
        self.Plot.setGeometry(QtCore.QRect(240, 10, 75, 23))
        self.Plot.setObjectName(_fromUtf8("Plot"))
        self.OverPlot = QtGui.QPushButton(self.centralwidget)
        self.OverPlot.setGeometry(QtCore.QRect(240, 40, 75, 23))
        self.OverPlot.setObjectName(_fromUtf8("OverPlot"))
        self.ClearFlux = QtGui.QPushButton(self.centralwidget)
        self.ClearFlux.setGeometry(QtCore.QRect(240, 70, 75, 23))
        self.ClearFlux.setToolTip(_fromUtf8(""))
        self.ClearFlux.setObjectName(_fromUtf8("ClearFlux"))
        MainWindow.setCentralWidget(self.centralwidget)
        self.Instrument = QtGui.QMenuBar(MainWindow)
        self.Instrument.setGeometry(QtCore.QRect(0, 0, 453, 18))
        self.Instrument.setWhatsThis(_fromUtf8(""))
        self.Instrument.setObjectName(_fromUtf8("Instrument"))
        self.InstrumentSelectionMenu = QtGui.QMenu(self.Instrument)
        self.InstrumentSelectionMenu.setObjectName(_fromUtf8("InstrumentSelectionMenu"))
        MainWindow.setMenuBar(self.Instrument)
        self.statusbar = QtGui.QStatusBar(MainWindow)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        MainWindow.setStatusBar(self.statusbar)
        self.actionLETFlux = QtGui.QAction(MainWindow)
        self.actionLETFlux.setObjectName(_fromUtf8("actionLETFlux"))
        self.actionLET = QtGui.QAction(MainWindow)
        self.actionLET.setCheckable(True)
        self.actionLET.setChecked(True)
        self.actionLET.setEnabled(True)
        self.actionLET.setObjectName(_fromUtf8("actionLET"))
        self.actionMARI = QtGui.QAction(MainWindow)
        self.actionMARI.setCheckable(False)
        self.actionMARI.setEnabled(False)
        self.actionMARI.setObjectName(_fromUtf8("actionMARI"))
        self.actionMAPS = QtGui.QAction(MainWindow)
        self.actionMAPS.setEnabled(False)
        self.actionMAPS.setObjectName(_fromUtf8("actionMAPS"))
        self.actionMERLIN = QtGui.QAction(MainWindow)
        self.actionMERLIN.setCheckable(True)
        self.actionMERLIN.setEnabled(False)
        self.actionMERLIN.setObjectName(_fromUtf8("actionMERLIN"))
        self.InstrumentSelectionMenu.addAction(self.actionLET)
        self.InstrumentSelectionMenu.addAction(self.actionMARI)
        self.InstrumentSelectionMenu.addAction(self.actionMAPS)
        self.InstrumentSelectionMenu.addAction(self.actionMERLIN)
        self.Instrument.addAction(self.InstrumentSelectionMenu.menuAction())

        self.retranslateUi(MainWindow)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QtGui.QApplication.translate("MainWindow", "Resolution Calculator", None, QtGui.QApplication.UnicodeUTF8))
        self.calcButton.setText(QtGui.QApplication.translate("MainWindow", "Calculate Flux and Resolution", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("MainWindow", "Incident energy (meV)", None, QtGui.QApplication.UnicodeUTF8))
        self.Plot.setText(QtGui.QApplication.translate("MainWindow", "Plot ", None, QtGui.QApplication.UnicodeUTF8))
        self.OverPlot.setText(QtGui.QApplication.translate("MainWindow", "OverPlot ", None, QtGui.QApplication.UnicodeUTF8))
        self.ClearFlux.setText(QtGui.QApplication.translate("MainWindow", "Clear Flux ", None, QtGui.QApplication.UnicodeUTF8))
        self.InstrumentSelectionMenu.setTitle(QtGui.QApplication.translate("MainWindow", "Instrument", None, QtGui.QApplication.UnicodeUTF8))
        self.actionLETFlux.setText(QtGui.QApplication.translate("MainWindow", "LETFlux", None, QtGui.QApplication.UnicodeUTF8))
        self.actionLET.setText(QtGui.QApplication.translate("MainWindow", "LET", None, QtGui.QApplication.UnicodeUTF8))
        self.actionMARI.setText(QtGui.QApplication.translate("MainWindow", "MARI", None, QtGui.QApplication.UnicodeUTF8))
        self.actionMAPS.setText(QtGui.QApplication.translate("MainWindow", "MAPS", None, QtGui.QApplication.UnicodeUTF8))
        self.actionMERLIN.setText(QtGui.QApplication.translate("MainWindow", "MERLIN", None, QtGui.QApplication.UnicodeUTF8))

