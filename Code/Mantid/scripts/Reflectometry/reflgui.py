# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'mainwindow.ui'
#
# Created: Mon Jan 17 14:49:16 2011
#      by: PyQt4 UI code generator 4.8.1
#
# WARNING! All changes made in this file will be lost!

#from ReflectometerCors import *
import csv
import math
import fileinput
from PyQt4 import QtCore, QtGui, uic
from mantidsimple import *
#from mantid.simpleapi import *  # New API
from mantidplot import *
#import qti as qti
from quick import *
from combineMulti import *

currentTable=' '

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s


class Ui_SaveWindow(object):
    def setupUi(self, SaveWindow):
        SaveWindow.setObjectName(_fromUtf8("SaveWindow"))
        SaveWindow.resize(400, 400)
        SaveWindow.setAcceptDrops(True)

        self.centralWidget = QtGui.QWidget(SaveWindow)
        self.centralWidget.setObjectName(_fromUtf8("centralWidget"))
        self.gridLayout_2 = QtGui.QGridLayout(self.centralWidget)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setSizeConstraint(QtGui.QLayout.SetNoConstraint)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))

        self.lineEdit = QtGui.QLineEdit(self.centralWidget)

        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.lineEdit.sizePolicy().hasHeightForWidth())
        self.lineEdit.setSizePolicy(sizePolicy)

        self.listWidget = QtGui.QListWidget(self.centralWidget)
        self.listWidget.setSelectionMode(QtGui.QAbstractItemView.MultiSelection)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())

        self.gridLayout.addWidget(self.listWidget, 1, 2, 1, 5)
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
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(False)
        self.lineEdit.setFont(font)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.gridLayout.addWidget(self.lineEdit, 0, 2, 1, 3)
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
        names = mtd.getWorkspaceNames()
        for ws in names:
            self.listWidget.addItem(ws)
        
#--------- If "Process" button pressed, convert raw files to IvsLam and IvsQ and combine if checkbox ticked -------------
    def buttonClickHandler1(self):
        names = mtd.getWorkspaceNames()
        dataToSave=[]
        if (self.lineEdit.text()[len(self.lineEdit.text())-1] != '\\'):
            path = self.lineEdit.text()+'\\'
        else:
            path = self.lineEdit.text()
        for idx in self.listWidget.selectedItems():
            SaveAscii(idx.text(),path+idx.text()+'.dat')
            
        

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName(_fromUtf8("MainWindow"))
        MainWindow.resize(1000, 400)
        MainWindow.setAcceptDrops(True)

        self.centralWidget = QtGui.QWidget(MainWindow)
        self.centralWidget.setObjectName(_fromUtf8("centralWidget"))
        self.gridLayout_2 = QtGui.QGridLayout(self.centralWidget)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setSizeConstraint(QtGui.QLayout.SetNoConstraint)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.tableWidget = QtGui.QTableWidget(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
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

        self.tableWidget.horizontalHeader().setCascadingSectionResizes(True)
        self.tableWidget.horizontalHeader().setDefaultSectionSize(60)
        self.tableWidget.verticalHeader().setDefaultSectionSize(20)
        self.gridLayout.addWidget(self.tableWidget, 1, 2, 1, 3)
        spacerItem = QtGui.QSpacerItem(38, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
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
        self.gridLayout.addWidget(self.comboBox, 0, 2, 1, 1)
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

        self.actionSURF = QtGui.QAction(MainWindow)
        self.actionSURF.setObjectName(_fromUtf8("actionSURF"))
        self.actionCRISP = QtGui.QAction(MainWindow)
        self.actionCRISP.setObjectName(_fromUtf8("actionCRISP"))
        self.actionPolRef = QtGui.QAction(MainWindow)
        self.actionPolRef.setObjectName(_fromUtf8("actionPolRef"))
        self.actionInter = QtGui.QAction(MainWindow)
        self.actionInter.setObjectName(_fromUtf8("actionInter"))

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
        QtCore.QObject.connect(self.pushButton, QtCore.SIGNAL(_fromUtf8("clicked()")), self.buttonClickHandler1)
        QtCore.QObject.connect(self.pushButton_2, QtCore.SIGNAL(_fromUtf8("clicked()")), self.initTable)
        QtCore.QObject.connect(self.comboBox, QtCore.SIGNAL(_fromUtf8("activated(QString)")), self.on_comboBox_Activated)
        QtCore.QObject.connect(self.actionSave_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveDialog)
        QtCore.QObject.connect(self.actionLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.loadDialog)
        QtCore.QObject.connect(self.actionReLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.ReloadDialog)
        QtCore.QObject.connect(self.actionSaveDialog, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveWksp)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QtGui.QApplication.translate("MainWindow", "MainWindow", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(0).setText(QtGui.QApplication.translate("MainWindow", "Run(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(1).setText(QtGui.QApplication.translate("MainWindow", "Angle 1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(2).setText(QtGui.QApplication.translate("MainWindow", "trans 1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(3).setText(QtGui.QApplication.translate("MainWindow", "qmin_1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(4).setText(QtGui.QApplication.translate("MainWindow", "qmax_1", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(5).setText(QtGui.QApplication.translate("MainWindow", "Run(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(6).setText(QtGui.QApplication.translate("MainWindow", "Angle 2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(7).setText(QtGui.QApplication.translate("MainWindow", "trans 2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(8).setText(QtGui.QApplication.translate("MainWindow", "qmin_2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(9).setText(QtGui.QApplication.translate("MainWindow", "qmax_2", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(10).setText(QtGui.QApplication.translate("MainWindow", "Run(s)", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(11).setText(QtGui.QApplication.translate("MainWindow", "Angle 3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(12).setText(QtGui.QApplication.translate("MainWindow", "trans 3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(13).setText(QtGui.QApplication.translate("MainWindow", "qmin_3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(14).setText(QtGui.QApplication.translate("MainWindow", "qmax_3", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(15).setText(QtGui.QApplication.translate("MainWindow", "dq/q", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(16).setText(QtGui.QApplication.translate("MainWindow", "scale", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.horizontalHeaderItem(17).setText(QtGui.QApplication.translate("MainWindow", "stitch?", None, QtGui.QApplication.UnicodeUTF8))
        
        self.pushButton.setText(QtGui.QApplication.translate("MainWindow", "Process", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_2.setText(QtGui.QApplication.translate("MainWindow", "Clear all", None, QtGui.QApplication.UnicodeUTF8))
        self.menuFile.setTitle(QtGui.QApplication.translate("MainWindow", "File", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(0, QtGui.QApplication.translate("MainWindow", "SURF", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(1, QtGui.QApplication.translate("MainWindow", "CRISP", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(2, QtGui.QApplication.translate("MainWindow", "PolRef", None, QtGui.QApplication.UnicodeUTF8))
        self.comboBox.setItemText(3, QtGui.QApplication.translate("MainWindow", "Inter", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSURF.setText(QtGui.QApplication.translate("MainWindow", "SURF", None, QtGui.QApplication.UnicodeUTF8))
        self.actionCRISP.setText(QtGui.QApplication.translate("MainWindow", "CRISP", None, QtGui.QApplication.UnicodeUTF8))
        self.actionPolRef.setText(QtGui.QApplication.translate("MainWindow", "PolRef", None, QtGui.QApplication.UnicodeUTF8))
        self.actionInter.setText(QtGui.QApplication.translate("MainWindow", "Inter", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSave_table.setText(QtGui.QApplication.translate("MainWindow", "Save table", None, QtGui.QApplication.UnicodeUTF8))
        self.actionLoad_table.setText(QtGui.QApplication.translate("MainWindow", "Load table", None, QtGui.QApplication.UnicodeUTF8))
        self.actionReLoad_table.setText(QtGui.QApplication.translate("MainWindow", "Re-Load table", None, QtGui.QApplication.UnicodeUTF8))
        self.actionSaveDialog.setText(QtGui.QApplication.translate("MainWindow", "Save Workspaces", None, QtGui.QApplication.UnicodeUTF8))


    def initTable(self): 
        self.tableWidget.resizeColumnsToContents()
        #self.tableWidget.setSelectionMode(2)
        #self.tableWidget.selectRow(2)
        #self.tableWidget.selectRow(4)
        for column in range(self.tableWidget.columnCount()):
            for row in range(self.tableWidget.rowCount()):
                if (column == 17):
#                    wdg = QtGui.QWidget()
                    item = QtGui.QCheckBox()
                    item.setCheckState(False)
                    self.tableWidget.setCellWidget(row, column, item)
                else:
                    self.tableWidget.setRowHeight(row,20)
                    item=QtGui.QTableWidgetItem()
                    item.setText('')
                    self.tableWidget.setItem(row,column,item)
		
#--------- If "Process" button pressed, convert raw files to IvsLam and IvsQ and combine if checkbox ticked -------------
    def buttonClickHandler1(self):
		rows=[] 
		for idx in self.tableWidget.selectionModel().selectedRows():
			rows.append(idx.row())
		
		noOfRows = range(self.tableWidget.rowCount())
		if len(rows):
			noOfRows = rows
		for row in noOfRows:#range(self.tableWidget.rowCount()):
			runno=[]
			wksp=[]
			wkspBinned=[]
			overlapLow=[]
			overlapHigh=[]
			g = ['g1','g2','g3']
			theta = [0,0,0]
			if (self.tableWidget.item(row,0).text() != ''):
				for i in range(3):
					r = str(self.tableWidget.item(row,i*5).text())
					if (r != ''):
						runno.append(r)
					ovLow = str(self.tableWidget.item(row,i*5+3).text())
					if (ovLow != ''):
						overlapLow.append(float(ovLow))
					ovHigh = str(self.tableWidget.item(row,i*5+4).text())
					if (ovHigh != ''):
						overlapHigh.append(float(ovHigh))

						
				print len(runno),"runs: ",runno
				# Determine resolution
				#if (runno[0] != ''):
				if (self.tableWidget.item(row,15).text() == ''):
					dqq = calcRes(runno[0])
					print "Calculated resolution: ",dqq
				else:
					dqq=float(self.tableWidget.item(row,15).text())
				# Populate runlist
				for i in range(len(runno)):
					theta = round(self.dorun(runno[i],row,i),3)
					wksp.append(runno[i]+'_IvsQ')
					if (self.tableWidget.item(row,i*5+1).text() == ''):
						item=QtGui.QTableWidgetItem()
						item.setText(str(theta))
						self.tableWidget.setItem(row,i*5+1,item)
					if wksp[i].find(',')>0 or wksp[i].find(':')>0:
						runlist = []
						l1 = wksp[i].split(',')
						for subs in l1:
							l2 = subs.split(':')
							for l3 in l2:
								runlist.append(l3)
						wksp[i] = runlist[0]+'_IvsQ'
					ws_name_binned = wksp[i] +'_binned'
					ws=getWorkspace(wksp[i])
					w1=getWorkspace(wksp[0])
					w2=getWorkspace(wksp[len(wksp)-1])
					if len(overlapLow):
						Qmin = overlapLow[0]
					else:
						Qmin = w1.readX(0)[0]
					if len(overlapHigh):
						Qmax = overlapHigh[len(overlapHigh)-1]
					else:
						Qmax = max(w2.readX(0))
					
					Rebin(wksp[i],ws_name_binned,str(overlapLow[i])+','+str(-dqq)+','+str(overlapHigh[i]))
					wkspBinned.append(ws_name_binned)
					wsb=getWorkspace(ws_name_binned)
					Imin = min(wsb.readY(0))
					Imax = max(wsb.readY(0))
					g[i] = plotSpectrum(ws_name_binned,0, True)
					titl=groupGet(ws_name_binned,'samp','run_title')
					if (i>0):
						mergePlots(g[0],g[i])
					g[0].activeLayer().setTitle(titl)
					g[0].activeLayer().setAxisScale(Layer.Left,Imin*0.1,Imax*10,Layer.Log10)
					g[0].activeLayer().setAxisScale(Layer.Bottom,Qmin*0.9,Qmax*1.1,Layer.Log10)
					g[0].activeLayer().setAutoScale()
				if (self.tableWidget.cellWidget(row,17).checkState() > 0):
					if (len(runno)==1):
						print "Nothing to combine!"
					elif (len(runno)==2):
						outputwksp = runno[0]+'_'+runno[1][3:5]
					else:
						outputwksp = runno[0]+'_'+runno[2][3:5]
					print runno
					w1=getWorkspace(wksp[0])
					w2=getWorkspace(wksp[len(wksp)-1])
					begoverlap = w2.readX(0)[0]
					#Qmin = w1.readX(0)[0]
					#Qmax = max(w2.readX(0))
					# get Qmax
					if (self.tableWidget.item(row,i*5+4).text() == ''):
						overlapHigh = 0.3*max(w1.readX(0))

					print overlapLow, overlapHigh
					wcomb = combineDataMulti(wkspBinned,outputwksp,overlapLow,overlapHigh,Qmin,Qmax,-dqq,1)
					if (self.tableWidget.item(row,16).text() != ''):
						Scale(outputwksp,outputwksp,1/float(self.tableWidget.item(row,16).text()))
					Qmin = getWorkspace(outputwksp).readX(0)[0]
					Qmax = max(getWorkspace(outputwksp).readX(0))
					gcomb = plotSpectrum(outputwksp,0, True)
					titl=groupGet(outputwksp,'samp','run_title')
					gcomb.activeLayer().setTitle(titl)
					gcomb.activeLayer().setAxisScale(Layer.Left,1e-8,100.0,Layer.Log10)
					gcomb.activeLayer().setAxisScale(Layer.Bottom,Qmin*0.9,Qmax*1.1,Layer.Log10)

    def dorun(self, runno, row, which):
        g = ['g1','g2','g3']
        transrun = str(self.tableWidget.item(row,which*5+2).text())
        angle = str(self.tableWidget.item(row,which*5+1).text())  
        names = mtd.getWorkspaceNames()
        if ',' in transrun:
            slam = transrun.split(',')[0]
            llam = transrun.split(',')[1]
            print "Transmission runs: ", transrun
            [I0MonitorIndex, MultiDetectorStart, nHist] = toLam(slam,'_'+slam)
            CropWorkspace(InputWorkspace="_D_"+slam,OutputWorkspace="_D_"+slam,StartWorkspaceIndex=0,EndWorkspaceIndex=0)
            [I0MonitorIndex, MultiDetectorStart, nHist] = toLam(llam,'_'+llam)
            CropWorkspace(InputWorkspace="_D_"+llam,OutputWorkspace="_D_"+llam,StartWorkspaceIndex=0,EndWorkspaceIndex=0)
            
            RebinToWorkspace(WorkspaceToRebin="_M_"+llam,WorkspaceToMatch="_DP_"+llam,OutputWorkspace="_M_P_"+llam)
            CropWorkspace(InputWorkspace="_M_P_"+llam,OutputWorkspace="_I0P_"+llam,StartWorkspaceIndex=I0MonitorIndex)
            Divide(LHSWorkspace="_DP_"+llam,RHSWorkspace="_I0P_"+llam,OutputWorkspace="_D_"+llam)
            
            RebinToWorkspace(WorkspaceToRebin="_M_"+slam,WorkspaceToMatch="_DP_"+slam,OutputWorkspace="_M_P_"+slam)
            CropWorkspace(InputWorkspace="_M_P_"+slam,OutputWorkspace="_I0P_"+slam,StartWorkspaceIndex=I0MonitorIndex)
            Divide(LHSWorkspace="_DP_"+slam,RHSWorkspace="_I0P_"+slam,OutputWorkspace="_D_"+slam)

            [transr, sf] = combine2("_D_"+slam,"_D_"+llam,"_transcomb",10.0,12.0,1.5,17.0,0.02,scalehigh=0)
            [wlam, wq, th] = quick(runno,angle,trans='_transcomb')
        else:
            [wlam, wq, th] = quick(runno,trans=transrun,theta=angle)        
        ws_name = str(runno) +'_IvsQ'        
        return th
        
    def on_comboBox_Activated(self, instrument):
        mtd.settings['default.instrument'] = str(instrument)
        print "Instrument is now: ", str(instrument)

    def saveDialog(self):
        filename = QtGui.QFileDialog.getSaveFileName() 
        writer = csv.writer(open(filename, "wb"))
        for row in range(self.tableWidget.rowCount()):
            rowtext = []
            for column in range(self.tableWidget.columnCount()-1):
                    rowtext.append(self.tableWidget.item(row,column).text())
            if (len(rowtext) > 0):
                writer.writerow(rowtext)

    def loadDialog(self):
        global currentTable
        filename = QtGui.QFileDialog.getOpenFileName()
        currentTable=filename
        reader = csv.reader(open(filename, "rb"))
        row = 0
        for line in reader:
            if (row<100):
                for column in range(self.tableWidget.columnCount()-1):
                    item=QtGui.QTableWidgetItem()
                    item.setText(line[column])
                    self.tableWidget.setItem(row,column,item)
                row = row + 1

    def ReloadDialog(self):
        global currentTable
        filename=currentTable
        reader = csv.reader(open(filename, "rb"))
        row = 0
        for line in reader:
            if (row<100):
                for column in range(self.tableWidget.columnCount()-1):
                    item=QtGui.QTableWidgetItem()
                    item.setText(line[column])
                    self.tableWidget.setItem(row,column,item)
                row = row + 1

    def saveWksp(self):
        Dialog = QtGui.QDialog()
        u = Ui_SaveWindow()
        u.setupUi(Dialog)
        
        Dialog.exec_()

def calcRes(run):    
    runno = '_' + str(run)+'temp'
    if type(run)==type(int()):
        LoadNexus(Filename=run,OutputWorkspace=runno)
    else:
        LoadNexus(Filename=run.replace("raw","nxs",1),OutputWorkspace=runno)
    # Get slits and detector angle theta from NeXuS
	s1a = groupGet(runno,'samp','S1')
	s4a = groupGet(runno,'samp','S4')
	theta = groupGet(runno,'samp','THETA')
	if type(s1a)!=float:
		s1 = s1a[len(s1a)-1]
	else:
		s1 = s1a
	if type(s4a)!=float:
		s4 = s4a[len(s4a)-1]
	else:
		s4 = s4a
	if type(theta)!=float:
		th = theta[len(theta)-1]
	else:
		th = theta
    
    print "s1=",s1,"s4=",s4,"theta=",theta
	#4268.0 is the S1-S4 distance in mm for SURF!!!
    resolution=math.atan((s1+s4)/(2*4268.0))*180/math.pi/th
    print "dq/q=",resolution
    mantid.deleteWorkspace(runno)
    return resolution


def groupGet(wksp,whattoget,field=''):
	'''
	returns information about instrument or sample details for a given workspace wksp,
	also if the workspace is a group (info from first group element)
	'''
	if (whattoget == 'inst'):
		if mtd[wksp].isGroup():
			return mtd[wksp+'_1'].getInstrument()
		else:
			return mtd[wksp].getInstrument()
			
	elif (whattoget == 'samp' and field != ''):
		if mtd[wksp].isGroup():
			try:
				res = mtd[wksp + '_1'].getSampleDetails().getLogData(field).value				
			except RuntimeError:
				res = 0
				print "Block "+field+" not found."			
		else:
			try:
				res = mtd[wksp].getSampleDetails().getLogData(field).value
			except RuntimeError:		
				res = 0
				print "Block "+field+" not found."
		return res

        
        
        
def getWorkspace(wksp):
    if mtd[wksp].isGroup():
        wout = mantid.getMatrixWorkspace(wksp+'_1')
    else:
        wout = mantid.getMatrixWorkspace(wksp)
        
    return wout
    


    
if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    MainWindow = QtGui.QMainWindow()
    ui = Ui_MainWindow()
    ui.setupUi(MainWindow)
    MainWindow.show()
    sys.exit(app.exec_())

