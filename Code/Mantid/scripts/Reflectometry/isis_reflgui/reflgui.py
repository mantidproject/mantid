# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'mainwindow.ui'
#
# Created: Mon Jan 17 14:49:16 2011
#      by: PyQt4 UI code generator 4.8.1
#
# WARNING! All changes made in this file will be lost!

# from ReflectometerCors import *
import csv
import math
import fileinput
import xml.etree.ElementTree as xml
from PyQt4 import QtCore, QtGui, uic
from PyQt4.QtGui import QFont
# from mantidsimple import *
from mantid.simpleapi import *  # New API

# import qti as qti
from quick import *
from combineMulti import *
from mantid.api import WorkspaceGroup
from settings import *
from latest_isis_runs import *

currentTable = ' '

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s
    
canMantidPlot = True
try:
    from mantidplot import *
except ImportError:
    canMantidPlot = False
    
class Ui_SaveWindow(object):
    def setupUi(self, SaveWindow):
        SaveWindow.setObjectName(_fromUtf8("SaveWindow"))
        SaveWindow.resize(500, 400)
        SaveWindow.setAcceptDrops(True)
        
        self.centralWidget = QtGui.QWidget(SaveWindow)
        self.centralWidget.setObjectName(_fromUtf8("centralWidget"))
        self.gridLayout_2 = QtGui.QGridLayout(self.centralWidget)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setSizeConstraint(QtGui.QLayout.SetNoConstraint)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))

# Path label and edit field
        self.PathLabel = QtGui.QLabel("Save path: ", self.centralWidget)
        self.gridLayout.addWidget(self.PathLabel, 0, 2, 1, 1)
        self.lineEdit = QtGui.QLineEdit(self.centralWidget)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(False)
        self.lineEdit.setFont(font)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.gridLayout.addWidget(self.lineEdit, 0, 3, 1, 3)
        # print QtGui.QMainWindow.findChild(QtGui.QMainWindow.QLabel,'RBEdit')

# Prefix label and edit field
        self.PrefixLabel = QtGui.QLabel("Prefix: ", self.centralWidget)
        self.gridLayout.addWidget(self.PrefixLabel, 0, 6, 1, 1)
        self.lineEdit2 = QtGui.QLineEdit(self.centralWidget)
        self.lineEdit2.setFont(font)
        self.lineEdit2.setObjectName(_fromUtf8("lineEdit2"))
        self.gridLayout.addWidget(self.lineEdit2, 0, 7, 1, 2)

        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.lineEdit.sizePolicy().hasHeightForWidth())
        self.lineEdit.setSizePolicy(sizePolicy)
        self.lineEdit2.setSizePolicy(sizePolicy)

        self.listWidget = QtGui.QListWidget(self.centralWidget)
        self.listWidget.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())

        self.gridLayout.addWidget(self.listWidget, 1, 2, 1, 8)
        spacerItem = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 1, 5, 1, 1)
        spacerItem1 = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem1, 4, 2, 1, 1)
        spacerItem2 = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem2, 1, 0, 1, 1)
        self.pushButton = QtGui.QPushButton(self.centralWidget)

        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton.sizePolicy().hasHeightForWidth())
        self.pushButton.setSizePolicy(sizePolicy)
        self.pushButton.setObjectName(_fromUtf8("pushButton"))
        self.gridLayout.addWidget(self.pushButton, 2, 2, 1, 1)
        spacerItem3 = QtGui.QSpacerItem(20, 28, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem3, 2, 3, 1, 1)
        self.pushButton_2 = QtGui.QPushButton(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton_2.sizePolicy().hasHeightForWidth())
        self.pushButton_2.setSizePolicy(sizePolicy)
        self.pushButton_2.setObjectName(_fromUtf8("pushButton_2"))
        self.gridLayout.addWidget(self.pushButton_2, 2, 4, 1, 1)

        self.gridLayout_2.addLayout(self.gridLayout, 0, 0, 1, 1)



        self.retranslateUi(SaveWindow)
        self.populateList()
        QtCore.QObject.connect(self.pushButton, QtCore.SIGNAL(_fromUtf8("clicked()")), self.buttonClickHandler1)
        QtCore.QObject.connect(self.pushButton_2, QtCore.SIGNAL(_fromUtf8("clicked()")), self.populateList)
     #   QtCore.QObject.connect(self.actionSave_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveDialog)
     #   QtCore.QObject.connect(self.actionLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.loadDialog)
        QtCore.QMetaObject.connectSlotsByName(SaveWindow)

    def retranslateUi(self, SaveWindow):
        SaveWindow.setWindowTitle(QtGui.QApplication.translate("SaveWindow", "SaveWindow", None, QtGui.QApplication.UnicodeUTF8))        
        self.pushButton.setText(QtGui.QApplication.translate("SaveWindow", "SAVE", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_2.setText(QtGui.QApplication.translate("SaveWindow", "Refresh", None, QtGui.QApplication.UnicodeUTF8))


    def populateList(self):
        self.listWidget.clear()
        names = mtd.getObjectNames()
        for ws in names:
            self.listWidget.addItem(ws)
        try:
            instrumentRuns =  LatestISISRuns(instrument=config['default.instrument'])
            runs = instrumentRuns.getJournalRuns()
            for run in runs:    
                    self.listWidget.addItem(run)
                  
        except Exception as ex:
            logger.notice("Could not list archive runs")
            logger.notice(str(ex))
        
        
        
        
#--------- If "Save" button pressed, selcted workspaces are saved -------------
    def buttonClickHandler1(self):
        names = mtd.getObjectNames()
        dataToSave = []
        prefix = str(self.lineEdit2.text())
        if (self.lineEdit.text()[len(self.lineEdit.text()) - 1] != '/'):
            path = self.lineEdit.text() + '/'
        else:
            path = self.lineEdit.text()
        for idx in self.listWidget.selectedItems():
            fname = str(path + prefix + idx.text() + '.dat')
            print "FILENAME: ", fname
            wksp = str(idx.text())
            SaveAscii(InputWorkspace=wksp, Filename=fname)
            
        

class Ui_MainWindow(object):
    
    __instrumentRuns = None
    
    def setupUi(self, MainWindow):
        MainWindow.setObjectName(_fromUtf8("ISIS Reflectometry"))
        MainWindow.resize(1300, 400)
        MainWindow.setAcceptDrops(True)

        self.centralWidget = QtGui.QWidget(MainWindow)
        self.centralWidget.setObjectName(_fromUtf8("centralWidget"))
        self.gridLayout_2 = QtGui.QGridLayout(self.centralWidget)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setSizeConstraint(QtGui.QLayout.SetNoConstraint)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.tableWidget = QtGui.QTableWidget(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.tableWidget.sizePolicy().hasHeightForWidth())
        self.tableWidget.setSizePolicy(sizePolicy)
        self.tableWidget.setAlternatingRowColors(True)
        self.tableWidget.setRowCount(100)
        self.tableWidget.setColumnCount(18)
        self.tableWidget.setObjectName(_fromUtf8("tableWidget"))
        self.tableWidget.setColumnCount(18)
        self.tableWidget.setRowCount(100)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(0, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(1, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(2, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(3, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(4, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(5, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(6, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(7, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(8, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(9, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(10, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(11, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(12, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(13, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(14, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(15, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(16, item)
        item = QtGui.QTableWidgetItem()
        self.tableWidget.setHorizontalHeaderItem(17, item)
        
        self.cycleWidget = QtGui.QComboBox(self.centralWidget)
        self.gridLayout.addWidget(self.cycleWidget, 0, 1)
        QtCore.QObject.connect(self.cycleWidget, QtCore.SIGNAL(_fromUtf8("activated(QString)")), self.on_cycle_changed)

# RB number label and edit field
        self.RBLabel = QtGui.QLabel("RB: ", self.centralWidget)
        self.gridLayout.addWidget(self.RBLabel, 0, 2, 1, 1)
        self.RBEdit = QtGui.QLineEdit(self.centralWidget)
        self.RBEdit.setObjectName(_fromUtf8("RBEdit"))
        self.RBEdit.setMaximumWidth(55)
        self.gridLayout.addWidget(self.RBEdit, 0, 3, 1, 1)
        # spacerItemRB = QtGui.QSpacerItem(38, 20, QtGui.QSizePolicy.Maximum, QtGui.QSizePolicy.Minimum)
        # self.gridLayout.addItem(spacerItemRB, 0, 1, 1, 1)

# tranmission runs label and edit field
        self.transRunLabel = QtGui.QLabel("Transmission run(s): ", self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.transRunLabel.sizePolicy().hasHeightForWidth())
        self.transRunLabel.setSizePolicy(sizePolicy)
        self.gridLayout.addWidget(self.transRunLabel, 0, 5, 1, 1)
        self.transRunEdit = QtGui.QLineEdit(self.centralWidget)
        self.transRunEdit.setMaximumWidth(100)
        self.gridLayout.addWidget(self.transRunEdit, 0, 6, 1, 1)
        spacerItem2 = QtGui.QSpacerItem(200, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem2, 0, 11, 1, 1)
        
# Autofill button
        self.fillButton = QtGui.QPushButton(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.fillButton.sizePolicy().hasHeightForWidth())
        self.fillButton.setSizePolicy(sizePolicy)
        self.fillButton.setObjectName(_fromUtf8("fillButton"))
        self.gridLayout.addWidget(self.fillButton, 0, 12, 1, 1)

# polarisation corrections label and checkbox
        self.tickLabel1 = QtGui.QLabel("Polarisation corrections ", self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.tickLabel1.sizePolicy().hasHeightForWidth())
        self.tickLabel1.setSizePolicy(sizePolicy)
        self.gridLayout.addWidget(self.tickLabel1, 0, 7, 1, 1)
        # self.tickBox1 = QtGui.QCheckBox(self.centralWidget)
        # self.gridLayout.addWidget(self.tickBox1,0,8,1,1)

# polarisation correction selector
        self.comboBox2 = QtGui.QComboBox(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.comboBox2.sizePolicy().hasHeightForWidth())
        self.comboBox2.setSizePolicy(sizePolicy)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(True)
        self.comboBox2.setFont(font)
        self.comboBox2.setObjectName(_fromUtf8("comboBox"))
        self.comboBox2.addItem(_fromUtf8("none"))
        self.comboBox2.addItem(_fromUtf8("1-PNR"))
        self.comboBox2.addItem(_fromUtf8("2-PA"))
        self.comboBox2.addItem(_fromUtf8("3-other"))
        self.gridLayout.addWidget(self.comboBox2, 0, 8, 1, 1)

# polarisation group box
        # self.groupBox = QtGui.QGroupBox("Polarisation corrections ")
        # self.groupBox.setCheckable(1)
        # self.radio1 = QtGui.QRadioButton("P&NR")
        # self.radio2 = QtGui.QRadioButton("PA")
        

        # self.radio1.setChecked(True)

        # self.hbox = QtGui.QHBoxLayout()
        # self.hbox.addWidget(self.radio1)
        # self.hbox.addWidget(self.radio2)
        # self.hbox.addStretch(1)
        # self.groupBox.setLayout(self.hbox)
        # self.gridLayout.addWidget(self.groupBox,0,7,1,2)



# (un)tick all label and checkbox
        self.tickLabel = QtGui.QLabel("(un)tick all ", self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.tickLabel.sizePolicy().hasHeightForWidth())
        self.tickLabel.setSizePolicy(sizePolicy)
        self.gridLayout.addWidget(self.tickLabel, 0, 10, 1, 1)
        self.tickBox = QtGui.QCheckBox(self.centralWidget)
        self.gridLayout.addWidget(self.tickBox, 0, 11, 1, 1)        

# listwidget
        self.listWidget = QtGui.QListWidget(self.centralWidget)
        self.listWidget.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())
        
        # self.gridLayout.addWidget(self.listWidget, 1, 0, 2, 4)

        self.splitter1 = QtGui.QSplitter(QtCore.Qt.Horizontal)
        self.splitter1.addWidget(self.listWidget)
        

        # spacerItem = QtGui.QSpacerItem(38, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        # self.gridLayout.addItem(spacerItem, 1, 2, 1, 1)
# transfer button
        self.transferButton = QtGui.QPushButton(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        # sizePolicy.setHeightForWidth(self.transferButton.sizePolicy().hasHeightForWidth())
        self.transferButton.setSizePolicy(sizePolicy)
        self.transferButton.setObjectName(_fromUtf8("transferButton"))
        self.transferButton.setMaximumWidth(20)
        # self.gridLayout.addWidget(self.transferButton, 1, 4, 1, 1)
        
        self.splitter1.addWidget(self.transferButton)
        self.gridLayout.addWidget(self.splitter1, 1, 0, 3, 4)

# tablewidget
        self.tableWidget.horizontalHeader().setCascadingSectionResizes(True)
        self.tableWidget.horizontalHeader().setDefaultSectionSize(60)
        self.tableWidget.verticalHeader().setDefaultSectionSize(20)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())
        self.gridLayout.addWidget(self.tableWidget, 1, 5, 1, 8)



        spacerItem = QtGui.QSpacerItem(38, 20, QtGui.QSizePolicy.Maximum, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 1, 10, 1, 1)
        spacerItem1 = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem1, 4, 5, 1, 1)
        spacerItem3 = QtGui.QSpacerItem(200, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem3, 0, 9, 1, 1)

        self.pushButton = QtGui.QPushButton(self.centralWidget)
        font = QtGui.QFont(self.centralWidget)
        font.setBold(True)
        self.pushButton.setFont(font)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton.sizePolicy().hasHeightForWidth())
        self.pushButton.setSizePolicy(sizePolicy)
        self.pushButton.setObjectName(_fromUtf8("pushButton"))
        self.gridLayout.addWidget(self.pushButton, 2, 5, 1, 7)

        spacerItem3 = QtGui.QSpacerItem(20, 28, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem3, 2, 6, 1, 1)
        self.pushButton_2 = QtGui.QPushButton(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton_2.sizePolicy().hasHeightForWidth())
        self.pushButton_2.setSizePolicy(sizePolicy)
        self.pushButton_2.setObjectName(_fromUtf8("pushButton_2"))
        self.gridLayout.addWidget(self.pushButton_2, 2, 12, 1, 1)
# instrument selector
        self.comboBox = QtGui.QComboBox(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.comboBox.sizePolicy().hasHeightForWidth())
        self.comboBox.setSizePolicy(sizePolicy)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(True)
        self.comboBox.setFont(font)
        self.comboBox.setObjectName(_fromUtf8("comboBox"))
        self.comboBox.addItem(_fromUtf8(""))
        self.comboBox.addItem(_fromUtf8(""))
        self.comboBox.addItem(_fromUtf8(""))
        self.comboBox.addItem(_fromUtf8(""))
        self.gridLayout.addWidget(self.comboBox, 0, 0, 1, 1)
        self.gridLayout_2.addLayout(self.gridLayout, 0, 0, 1, 1)
        MainWindow.setCentralWidget(self.centralWidget)
# set up Menu Bar
        self.menuBar = QtGui.QMenuBar(MainWindow)
        self.menuBar.setGeometry(QtCore.QRect(0, 0, 789, 18))
        self.menuBar.setDefaultUp(False)
        self.menuBar.setObjectName(_fromUtf8("menuBar"))
        self.menuFile = QtGui.QMenu(self.menuBar)
        self.menuFile.setObjectName(_fromUtf8("menuFile"))
        MainWindow.setMenuBar(self.menuBar)
        self.statusBar = QtGui.QStatusBar(MainWindow)
        self.statusBar.setObjectName(_fromUtf8("statusBar"))
        MainWindow.setStatusBar(self.statusBar)

        self.actionInter = QtGui.QAction(MainWindow)
        self.actionInter.setObjectName(_fromUtf8("actionInter"))
        self.actionSURF = QtGui.QAction(MainWindow)
        self.actionSURF.setObjectName(_fromUtf8("actionSURF"))
        self.actionCRISP = QtGui.QAction(MainWindow)
        self.actionCRISP.setObjectName(_fromUtf8("actionCRISP"))
        self.actionPolRef = QtGui.QAction(MainWindow)
        self.actionPolRef.setObjectName(_fromUtf8("actionPolRef"))


        self.actionSave_table = QtGui.QAction(MainWindow)
        self.actionSave_table.setObjectName(_fromUtf8("actionSave_table"))
        self.actionSave_table.setShortcut('Ctrl+S')
        self.actionLoad_table = QtGui.QAction(MainWindow)
        self.actionLoad_table.setObjectName(_fromUtf8("actionLoad_table"))
        self.actionLoad_table.setShortcut('Ctrl+O')
        self.actionReLoad_table = QtGui.QAction(MainWindow)
        self.actionReLoad_table.setObjectName(_fromUtf8("actionReLoad_table"))
        self.actionReLoad_table.setShortcut('Ctrl+R')
        self.actionSaveDialog = QtGui.QAction(MainWindow)
        self.actionSaveDialog.setObjectName(_fromUtf8("actionSaveDialog"))
        self.actionSaveDialog.setShortcut('Ctrl+Shift+S')
        self.menuFile.addAction(self.actionSave_table)
        self.menuFile.addAction(self.actionLoad_table)
        self.menuFile.addAction(self.actionReLoad_table)
        self.menuFile.addAction(self.actionSaveDialog)
        self.menuBar.addAction(self.menuFile.menuAction())


        self.retranslateUi(MainWindow)
        self.initTable()
        self.readJournal()

        QtCore.QObject.connect(self.pushButton, QtCore.SIGNAL(_fromUtf8("clicked()")), self.buttonClickHandler1)
        QtCore.QObject.connect(self.fillButton, QtCore.SIGNAL(_fromUtf8("clicked()")), self.fillButtonHandler)
        QtCore.QObject.connect(self.transferButton, QtCore.SIGNAL(_fromUtf8("clicked()")), self.transferButtonHandler)
        QtCore.QObject.connect(self.pushButton_2, QtCore.SIGNAL(_fromUtf8("clicked()")), self.initTable)
        QtCore.QObject.connect(self.tickBox, QtCore.SIGNAL(_fromUtf8("stateChanged(int)")), self.unTickAll)
        QtCore.QObject.connect(self.RBEdit, QtCore.SIGNAL(_fromUtf8("editingFinished()")), self.readJournal)
        QtCore.QObject.connect(self.comboBox, QtCore.SIGNAL(_fromUtf8("activated(QString)")), self.on_comboBox_Activated)
        QtCore.QObject.connect(self.actionSave_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveDialog)
        QtCore.QObject.connect(self.actionLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.loadDialog)
        QtCore.QObject.connect(self.actionReLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.ReloadDialog)
        QtCore.QObject.connect(self.actionSaveDialog, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveWksp)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)
        
        
    def populateList(self, selected_cycle=None):
        # Clear existing
        self.listWidget.clear()
        # Fill with ADS workspaces
        self.__populateListADSWorkspaces()
        try:
            selectedInstrument = config['default.instrument'].strip().upper()
            if not self.__instrumentRuns:
                self.__instrumentRuns =  LatestISISRuns(instrument=selectedInstrument)
            elif not self.__instrumentRuns.getInstrument() == selectedInstrument:
                    self.__instrumentRuns =  LatestISISRuns(selectedInstrument)
            self.__populateListCycle(selected_cycle)
        except Exception as ex:
            self.cycleWidget.setVisible(False)
            logger.notice("Could not list archive runs")
            logger.notice(str(ex))
        
    def __populateListCycle(self, selected_cycle=None):  
        runs = self.__instrumentRuns.getJournalRuns(cycle=selected_cycle)
        for run in runs:
            self.listWidget.addItem(run)
            
        # Get possible cycles for this instrument.            
        cycles = self.__instrumentRuns.getCycles() 
        # Setup the list of possible cycles. And choose the latest as the default
        if not selected_cycle: 
            cycle_count = 0
            self.cycleWidget.clear()
            for cycle in cycles:
                self.cycleWidget.addItem(cycle)
                if cycle == self.__instrumentRuns.getLatestCycle():
                    self.cycleWidget.setCurrentIndex(cycle_count)
                cycle_count+=1
        # Ensure that the cycle widget is shown.
        self.cycleWidget.setVisible(True)
     
    def __populateListADSWorkspaces(self):
        names = mtd.getObjectNames()
        for ws in names:
            self.listWidget.addItem(ws)
              
    
    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QtGui.QApplication.translate("ISIS Reflectometry", "ISIS Reflectometry", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(0).setText(QtGui.QApplication.translate("ISIS Reflectometry", "Run(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(1).setText(QtGui.QApplication.translate("ISIS Reflectometry", "Angle 1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(2).setText(QtGui.QApplication.translate("ISIS Reflectometry", "trans 1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(3).setText(QtGui.QApplication.translate("ISIS Reflectometry", "qmin_1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(4).setText(QtGui.QApplication.translate("ISIS Reflectometry", "qmax_1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(5).setText(QtGui.QApplication.translate("ISIS Reflectometry", "Run(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(6).setText(QtGui.QApplication.translate("ISIS Reflectometry", "Angle 2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(7).setText(QtGui.QApplication.translate("ISIS Reflectometry", "trans 2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(8).setText(QtGui.QApplication.translate("ISIS Reflectometry", "qmin_2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(9).setText(QtGui.QApplication.translate("ISIS Reflectometry", "qmax_2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(10).setText(QtGui.QApplication.translate("ISIS Reflectometry", "Run(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(11).setText(QtGui.QApplication.translate("ISIS Reflectometry", "Angle 3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(12).setText(QtGui.QApplication.translate("ISIS Reflectometry", "trans 3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(13).setText(QtGui.QApplication.translate("ISIS Reflectometry", "qmin_3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(14).setText(QtGui.QApplication.translate("ISIS Reflectometry", "qmax_3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(15).setText(QtGui.QApplication.translate("ISIS Reflectometry", "dq/q", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(16).setText(QtGui.QApplication.translate("ISIS Reflectometry", "scale", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(17).setText(QtGui.QApplication.translate("ISIS Reflectometry", "stitch?", None, QtGui.QApplication.UnicodeUTF8))
        
        self.pushButton.setText(QtGui.QApplication.translate("ISIS Reflectometry", "&Process", None, QtGui.QApplication.UnicodeUTF8))
        self.fillButton.setText(QtGui.QApplication.translate("ISIS Reflectometry", "&Autofill", None, QtGui.QApplication.UnicodeUTF8))
        self.transferButton.setText(QtGui.QApplication.translate("ISIS Reflectometry", "=>", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_2.setText(QtGui.QApplication.translate("ISIS Reflectometry", "&Clear all", None, QtGui.QApplication.UnicodeUTF8))
        self.menuFile.setTitle(QtGui.QApplication.translate("ISIS Reflectometry", "&File", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(1, QtGui.QApplication.translate("ISIS Reflectometry", "SURF", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(2, QtGui.QApplication.translate("ISIS Reflectometry", "CRISP", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(3, QtGui.QApplication.translate("ISIS Reflectometry", "PolRef", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(0, QtGui.QApplication.translate("ISIS Reflectometry", "Inter", None, QtGui.QApplication.UnicodeUTF8))
        self.actionInter.setText(QtGui.QApplication.translate("ISIS Reflectometry", "Inter", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSURF.setText(QtGui.QApplication.translate("ISIS Reflectometry", "SURF", None, QtGui.QApplication.UnicodeUTF8))
        self.actionCRISP.setText(QtGui.QApplication.translate("ISIS Reflectometry", "CRISP", None, QtGui.QApplication.UnicodeUTF8))
        self.actionPolRef.setText(QtGui.QApplication.translate("ISIS Reflectometry", "PolRef", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSave_table.setText(QtGui.QApplication.translate("ISIS Reflectometry", "Save table", None, QtGui.QApplication.UnicodeUTF8))
        self.actionLoad_table.setText(QtGui.QApplication.translate("ISIS Reflectometry", "Load table", None, QtGui.QApplication.UnicodeUTF8))
        self.actionReLoad_table.setText(QtGui.QApplication.translate("ISIS Reflectometry", "Re-Load table", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSaveDialog.setText(QtGui.QApplication.translate("ISIS Reflectometry", "Save Workspaces", None, QtGui.QApplication.UnicodeUTF8))


    def readJournal(self):
        self.populateList()
        
    def on_cycle_changed(self, cycle):
        self.populateList(selected_cycle=cycle)


    def initTable(self): 
        self.tableWidget.resizeColumnsToContents()
        # self.tableWidget.setSelectionMode(2)
        # self.tableWidget.selectRow(2)
        # self.tableWidget.selectRow(4)
        instrumentList = ['INTER', 'SURF', 'CRISP', 'POLREF']
        currentInstrument = config['default.instrument']
        if currentInstrument in instrumentList:
            self.comboBox.setCurrentIndex(instrumentList.index(config['default.instrument'].upper()))
        else:
            self.comboBox.setCurrentIndex(0)
            config['default.instrument'] = 'INTER'
        for column in range(self.tableWidget.columnCount()):
            for row in range(self.tableWidget.rowCount()):
                if (column == 17):
#                    wdg = QtGui.QWidget()
                    item = QtGui.QCheckBox()
                    item.setCheckState(False)
                    self.tableWidget.setCellWidget(row, column, item)
                else:
                    self.tableWidget.setRowHeight(row, 20)
                    item = QtGui.QTableWidgetItem()
                    item.setText('')
                    self.tableWidget.setItem(row, column, item)

    def fillButtonHandler(self):
        col = 0
        # make sure all selected cells are in the same row
        sum = 0
        howMany = len(self.tableWidget.selectedItems())
        for cell in self.tableWidget.selectedItems():
            sum = sum + self.tableWidget.row(cell)
        if (howMany):
            if (sum / howMany == self.tableWidget.row(self.tableWidget.selectedItems()[0])):
                for cell in self.tableWidget.selectedItems():
                    row = self.tableWidget.row(cell) + 1
                    txt = cell.text()
                    while (self.tableWidget.item(row, 0).text() != ''):
                        item = QtGui.QTableWidgetItem()
                        item.setText(txt)
                        self.tableWidget.setItem(row, self.tableWidget.column(cell), item)
                        row = row + 1

                # while (self.tableWidget.item(row,0).text() != ''):
            # row=row+1
        # for idx in self.listWidget.selectedItems():
            # runno=idx.text().split(':')[0]
            # item=QtGui.QTableWidgetItem()
            # item.setText(runno)
            # self.tableWidget.setItem(row,col,item)
            # item=QtGui.QTableWidgetItem()
            # item.setText(self.transRunEdit.text())
            # self.tableWidget.setItem(row,col+2,item)
            # col=col+5
        

    def transferButtonHandler(self):
        col = 0
        row = 0
        while (self.tableWidget.item(row, 0).text() != ''):
            row = row + 1
        for idx in self.listWidget.selectedItems():
            contents = str(idx.text()).strip()
            first_contents = contents.split(':')[0]
            runnumber = None
            if mtd.doesExist(first_contents):
                runnumber = groupGet(mtd[first_contents], "samp", "run_number")
            else:
                temp = Load(Filename=first_contents, OutputWorkspace="_tempforrunnumber")
                runnumber = groupGet("_tempforrunnumber", "samp", "run_number")
                DeleteWorkspace(temp)
            
            item = QtGui.QTableWidgetItem()
            item.setText(runnumber)
            self.tableWidget.setItem(row, col, item)
            item = QtGui.QTableWidgetItem()
            item.setText(self.transRunEdit.text())
            self.tableWidget.setItem(row, col + 2, item)
            col = col + 5
        
        
    def unTickAll(self):
        for row in range(self.tableWidget.rowCount()):
            if (self.tickBox.checkState()):
                self.tableWidget.cellWidget(row, 17).setCheckState(True)
            else:
                self.tableWidget.cellWidget(row, 17).setCheckState(False)
            # self.tableWidget.setCellWidget(row, 17, item)

#--------- If "Process" button pressed, convert raw files to IvsLam and IvsQ and combine if checkbox ticked -------------
    def buttonClickHandler1(self):
        rows = [] 
        for idx in self.tableWidget.selectionModel().selectedRows():
            rows.append(idx.row())
        
        noOfRows = range(self.tableWidget.rowCount())
        if len(rows):
            noOfRows = rows
        for row in noOfRows:  # range(self.tableWidget.rowCount()):
            runno = []
            wksp = []
            wkspBinned = []
            overlapLow = []
            overlapHigh = []
            g = ['g1', 'g2', 'g3']
            theta = [0, 0, 0]
            if (self.tableWidget.item(row, 0).text() != ''):
                for i in range(3):
                    r = str(self.tableWidget.item(row, i * 5).text())
                    if (r != ''):
                        runno.append(r)
                    ovLow = str(self.tableWidget.item(row, i * 5 + 3).text())
                    if (ovLow != ''):
                        overlapLow.append(float(ovLow))
                    ovHigh = str(self.tableWidget.item(row, i * 5 + 4).text())
                    if (ovHigh != ''):
                        overlapHigh.append(float(ovHigh))

                        
                print len(runno), "runs: ", runno
                # Determine resolution
                # if (runno[0] != ''):
                if (self.tableWidget.item(row, 15).text() == ''):
                    dqq = calcRes(runno[0])
                    item = QtGui.QTableWidgetItem()
                    item.setText(str(dqq))
                    self.tableWidget.setItem(row, 15, item)
                    print "Calculated resolution: ", dqq
                else:
                    dqq = float(self.tableWidget.item(row, 15).text())
                # Populate runlist
                for i in range(len(runno)):
                    [theta, qmin, qmax] = self.dorun(runno[i], row, i)
                    theta = round(theta, 3)
                    qmin = round(qmin, 3)
                    qmax = round(qmax, 3)
                    wksp.append(runno[i] + '_IvsQ')
                    if (self.tableWidget.item(row, i * 5 + 1).text() == ''):
                        item = QtGui.QTableWidgetItem()
                        item.setText(str(theta))
                        self.tableWidget.setItem(row, i * 5 + 1, item)

                    if (self.tableWidget.item(row, i * 5 + 3).text() == ''):
                        item = QtGui.QTableWidgetItem()
                        item.setText(str(qmin))
                        self.tableWidget.setItem(row, i * 5 + 3, item)
                        overlapLow.append(qmin)

                    if (self.tableWidget.item(row, i * 5 + 4).text() == ''):
                        item = QtGui.QTableWidgetItem()
                        if i == len(runno) - 1:
                        # allow full high q-range for last angle
                            qmax = 4 * math.pi / ((4 * math.pi / qmax * math.sin(theta * math.pi / 180)) - 0.5) * math.sin(theta * math.pi / 180)
                        item.setText(str(qmax))
                        self.tableWidget.setItem(row, i * 5 + 4, item)
                        overlapHigh.append(qmax)

                    if wksp[i].find(',') > 0 or wksp[i].find(':') > 0:
                        runlist = []
                        l1 = wksp[i].split(',')
                        for subs in l1:
                            l2 = subs.split(':')
                            for l3 in l2:
                                runlist.append(l3)
                        wksp[i] = runlist[0] + '_IvsQ'
                    ws_name_binned = wksp[i] + '_binned'
                    ws = getWorkspace(wksp[i])
                    w1 = getWorkspace(wksp[0])
                    w2 = getWorkspace(wksp[len(wksp) - 1])
                    if len(overlapLow):
                        Qmin = overlapLow[0]
                    else:
                        Qmin = w1.readX(0)[0]
                    if len(overlapHigh):
                        Qmax = overlapHigh[len(overlapHigh) - 1]
                    else:
                        Qmax = max(w2.readX(0))
                    
                    Rebin(InputWorkspace=wksp[i], Params=str(overlapLow[i]) + ',' + str(-dqq) + ',' + str(overlapHigh[i]), OutputWorkspace=ws_name_binned)
                    wkspBinned.append(ws_name_binned)
                    wsb = getWorkspace(ws_name_binned)
                    Imin = min(wsb.readY(0))
                    Imax = max(wsb.readY(0))
                    if canMantidPlot:
                        g[i] = plotSpectrum(ws_name_binned, 0, True)
                        titl = groupGet(ws_name_binned, 'samp', 'run_title')
                        if (i > 0):
                            mergePlots(g[0], g[i])
                        if (type(titl) == str):
                            g[0].activeLayer().setTitle(titl)
                        g[0].activeLayer().setAxisScale(Layer.Left, Imin * 0.1, Imax * 10, Layer.Log10)
                        g[0].activeLayer().setAxisScale(Layer.Bottom, Qmin * 0.9, Qmax * 1.1, Layer.Log10)
                        g[0].activeLayer().setAutoScale()
                if (self.tableWidget.cellWidget(row, 17).checkState() > 0):
                    if (len(runno) == 1):
                        print "Nothing to combine!"
                    elif (len(runno) == 2):
                        outputwksp = runno[0] + '_' + runno[1][3:5]
                    else:
                        outputwksp = runno[0] + '_' + runno[2][3:5]
                    print runno
                    w1 = getWorkspace(wksp[0])
                    w2 = getWorkspace(wksp[len(wksp) - 1])
                    begoverlap = w2.readX(0)[0]
                    # Qmin = w1.readX(0)[0]
                    # Qmax = max(w2.readX(0))
                    # get Qmax
                    if (self.tableWidget.item(row, i * 5 + 4).text() == ''):
                        overlapHigh = 0.3 * max(w1.readX(0))

                    print overlapLow, overlapHigh
                    wcomb = combineDataMulti(wkspBinned, outputwksp, overlapLow, overlapHigh, Qmin, Qmax, -dqq, 1)
                    if (self.tableWidget.item(row, 16).text() != ''):
                        Scale(InputWorkspace=outputwksp, OutputWorkspace=outputwksp, Factor=1 / float(self.tableWidget.item(row, 16).text()))
                    Qmin = getWorkspace(outputwksp).readX(0)[0]
                    Qmax = max(getWorkspace(outputwksp).readX(0))
                    if canMantidPlot:
                        gcomb = plotSpectrum(outputwksp, 0, True)
                        titl = groupGet(outputwksp, 'samp', 'run_title')
                        gcomb.activeLayer().setTitle(titl)
                        gcomb.activeLayer().setAxisScale(Layer.Left, 1e-8, 100.0, Layer.Log10)
                        gcomb.activeLayer().setAxisScale(Layer.Bottom, Qmin * 0.9, Qmax * 1.1, Layer.Log10)

    def dorun(self, runno, row, which):
        g = ['g1', 'g2', 'g3']
        transrun = str(self.tableWidget.item(row, which * 5 + 2).text())
        angle = str(self.tableWidget.item(row, which * 5 + 1).text())  
        names = mtd.getObjectNames()
        [wlam, wq, th] = quick(runno, trans=transrun, theta=angle)        
        if ':' in runno:
            runno = runno.split(':')[0]
        if ',' in runno:
            runno = runno.split(',')[0]

        ws_name = str(runno) + '_IvsQ'
        inst = groupGet(ws_name, 'inst')
        lmin = inst.getNumberParameter('LambdaMin')[0] + 1
        lmax = inst.getNumberParameter('LambdaMax')[0] - 2
        qmin = 4 * math.pi / lmax * math.sin(th * math.pi / 180)
        qmax = 4 * math.pi / lmin * math.sin(th * math.pi / 180)
        return th, qmin, qmax
        
    def on_comboBox_Activated(self, instrument):
        config['default.instrument'] = str(instrument)
        print "Instrument is now: ", str(instrument)
        self.readJournal()

    def saveDialog(self):
        filename = QtGui.QFileDialog.getSaveFileName() 
        writer = csv.writer(open(filename, "wb"))
        for row in range(self.tableWidget.rowCount()):
            rowtext = []
            for column in range(self.tableWidget.columnCount() - 1):
                    rowtext.append(self.tableWidget.item(row, column).text())
            if (len(rowtext) > 0):
                writer.writerow(rowtext)

    def loadDialog(self):
        global currentTable
        filename = QtGui.QFileDialog.getOpenFileName()
        currentTable = filename
        reader = csv.reader(open(filename, "rb"))
        row = 0
        for line in reader:
            if (row < 100):
                for column in range(self.tableWidget.columnCount() - 1):
                    item = QtGui.QTableWidgetItem()
                    item.setText(line[column])
                    self.tableWidget.setItem(row, column, item)
                row = row + 1

    def ReloadDialog(self):
        global currentTable
        filename = currentTable
        reader = csv.reader(open(filename, "rb"))
        row = 0
        for line in reader:
            if (row < 100):
                for column in range(self.tableWidget.columnCount() - 1):
                    item = QtGui.QTableWidgetItem()
                    item.setText(line[column])
                    self.tableWidget.setItem(row, column, item)
                row = row + 1

    def saveWksp(self):
        Dialog = QtGui.QDialog()
        u = Ui_SaveWindow()
        u.setupUi(Dialog)
        
        Dialog.exec_()

def calcRes(run):    
    runno = '_' + str(run) + 'temp'
    if type(run) == type(int()):
        Load(Filename=run, OutputWorkspace=runno)
    else:
        Load(Filename=run.replace("raw", "nxs", 1), OutputWorkspace=runno)
    # Get slits and detector angle theta from NeXuS
    theta = groupGet(runno, 'samp', 'THETA')
    inst = groupGet(runno, 'inst')
    s1z = inst.getComponentByName('slit1').getPos().getZ() * 1000.0  # distance in mm
    s2z = inst.getComponentByName('slit2').getPos().getZ() * 1000.0  # distance in mm
    s1vg = inst.getComponentByName('slit1')
    s1vg = s1vg.getNumberParameter('vertical gap')[0]
    s2vg = inst.getComponentByName('slit2')
    s2vg = s2vg.getNumberParameter('vertical gap')[0]

    if type(theta) != float:
        th = theta[len(theta) - 1]
    else:
        th = theta
    
    print "s1vg=", s1vg, "s2vg=", s2vg, "theta=", theta
    #1500.0 is the S1-S2 distance in mm for SURF!!!
    resolution = math.atan((s1vg + s2vg) / (2 * (s2z - s1z))) * 180 / math.pi / th
    print "dq/q=", resolution
    DeleteWorkspace(runno)
    return resolution

    
def groupGet(wksp, whattoget, field=''):
    '''
    returns information about instrument or sample details for a given workspace wksp,
    also if the workspace is a group (info from first group element)
    '''
    
    if (whattoget == 'inst'):
        if isinstance(mtd[wksp], WorkspaceGroup):
            return mtd[wksp + '_1'].getInstrument()
        else:
            return mtd[wksp].getInstrument()
            
    elif (whattoget == 'samp' and field != ''):
        if isinstance(mtd[wksp], WorkspaceGroup):
            try:
                log = mtd[wksp + '_1'].getRun().getLogData(field).value
                if (type(log) is int or type(log) is str):
                    res = log
                else:
                    res = log[len(log) - 1]
            except RuntimeError:
                res = 0
                print "Block " + field + " not found."            
        else:
            try:
                log = mtd[wksp].getRun().getLogData(field).value
                if (type(log) is int or type(log) is str):
                    res = log
                else:
                    res = log[len(log) - 1]
            except RuntimeError:        
                res = 0
                print "Block " + field + " not found."
        return res
    elif (whattoget == 'wksp'):
        if isinstance(mtd[wksp], WorkspaceGroup):
            return mtd[wksp + '_1'].getNumberHistograms()
        else:
            return mtd[wksp].getNumberHistograms()



        
        
        
def getWorkspace(wksp):

    if isinstance(mtd[wksp], WorkspaceGroup):
        wout = mtd[wksp + '_1']
    else:
        wout = mtd[wksp]
        
    return wout
    


    
if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    MainWindow = QtGui.QMainWindow()
    ui = Ui_MainWindow()
    ui.setupUi(MainWindow)
    MainWindow.show()
    sys.exit(app.exec_())
