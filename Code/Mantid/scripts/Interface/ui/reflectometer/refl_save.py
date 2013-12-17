from PyQt4 import QtCore, QtGui
from mantid.simpleapi import *

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_SaveWindow(object):

    def setupUi(self, SaveWindow):
        SaveWindow.setObjectName(_fromUtf8("SaveWindow"))
        SaveWindow.resize(700, 450)
        SaveWindow.setAcceptDrops(True)

        self.centralWidget = QtGui.QWidget(SaveWindow)
        self.centralWidget.setObjectName(_fromUtf8("centralWidget"))
        self.gridLayout_2 = QtGui.QGridLayout(self.centralWidget)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setSizeConstraint(QtGui.QLayout.SetNoConstraint)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))

# Path label and edit field
        self.PathLabel = QtGui.QLabel("Save path: ",self.centralWidget)
        self.gridLayout.addWidget(self.PathLabel,0,2,1,1)
        self.lineEdit = QtGui.QLineEdit(self.centralWidget)
        font = QtGui.QFont()
        font.setWeight(75)
        font.setBold(False)
        self.lineEdit.setFont(font)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.gridLayout.addWidget(self.lineEdit, 0, 3, 1, 3)
        #print QtGui.QMainWindow.findChild(QtGui.QMainWindow.QLabel,'RBEdit')

# Prefix label and edit field
        self.PrefixLabel = QtGui.QLabel("Prefix: ",self.centralWidget)
        self.gridLayout.addWidget(self.PrefixLabel,0,6,1,1)
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
        
        self.ListLabel = QtGui.QLabel("List of workspaces: ",self.centralWidget)
        self.gridLayout.addWidget(self.ListLabel,1,2,1,3)

        self.LogsLabel = QtGui.QLabel("List of logged parameters: ",self.centralWidget)
        self.gridLayout.addWidget(self.LogsLabel,1,6,1,3)
        
# List of workspaces
        self.listWidget = QtGui.QListWidget(self.centralWidget)
        self.listWidget.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())
        self.gridLayout.addWidget(self.listWidget, 2, 2, 1, 3)

# List of Logged Parameters
        self.listWidget2 = QtGui.QListWidget(self.centralWidget)
        self.listWidget2.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())
        self.gridLayout.addWidget(self.listWidget2, 2, 6, 1, 3)

        spacerItem = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 2, 5, 1, 1)
        spacerItem1 = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem1, 4, 2, 1, 1)
        spacerItem2 = QtGui.QSpacerItem(20, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem2, 2, 0, 1, 1)
        self.pushButton = QtGui.QPushButton(self.centralWidget)

# Save Title
        self.titleCheckBox = QtGui.QCheckBox("Title", self.centralWidget)
        #self.gridLayout.addWidget(self.titleCheckBox, 3, 6, 1, 1)



# Tab check box
        #self.tabCheckBox = QtGui.QCheckBox("tab", self.centralWidget)
        #self.gridLayout.addWidget(self.titleCheckBox, 3, 6, 1, 1)

# Comma check box
        #self.commaCheckBox = QtGui.QCheckBox("comma", self.centralWidget)
        #self.gridLayout.addWidget(self.commaCheckBox, 3, 6, 1, 1)


# Space check box
        #self.spaceCheckBox = QtGui.QCheckBox("space", self.centralWidget)
        #self.gridLayout.addWidget(self.commaCheckBox, 3, 6, 1, 1)

# Save XError
        self.xErrorCheckBox = QtGui.QCheckBox("Q resolution", self.centralWidget)
        #self.gridLayout.addWidget(self.xErrorCheckBox, 3, 7, 1, 1)

# separator
        #self.separatorLabel = QtGui.QLabel("Separator: ", self.centralWidget)
        #self.gridLayout.addWidget(self.separatorLabel,4,6,1,1)
        #self.separatorEdit = QtGui.QLineEdit(self.centralWidget)
        #self.separatorEdit.setObjectName(_fromUtf8("separatorEdit"))
        #self.gridLayout.addWidget(self.separatorEdit, 4, 7, 1, 1)

        self.groupBox = QtGui.QGroupBox("Custom format options")

        self.vbox = QtGui.QVBoxLayout()
        self.hbox = QtGui.QHBoxLayout()
        self.vbox.addWidget(self.titleCheckBox)
        self.vbox.addWidget(self.xErrorCheckBox)

        self.groupBox2 = QtGui.QGroupBox("Separator")
        #self.buttonGroup=QtGui.QButtonGroup("Separator:", self.groupBox)
        self.radio1=QtGui.QRadioButton("Comma", self.centralWidget)
        self.radio2=QtGui.QRadioButton("Space", self.centralWidget)
        self.radio3=QtGui.QRadioButton("Tab", self.centralWidget)
        

        self.radio1.setChecked(1)
        self.hbox.addWidget(self.radio1)
        self.hbox.addWidget(self.radio2)
        self.hbox.addWidget(self.radio3)
        self.groupBox2.setLayout(self.hbox)
        # self.hbox.addWidget(self.separatorLabel)
        # self.hbox.addWidget(self.commaCheckBox)
        # self.hbox.addWidget(self.spaceCheckBox)
        # self.hbox.addWidget(self.tabCheckBox)

        self.vbox.addWidget(self.groupBox2)
        self.vbox.addStretch(1)
        #self.groupBox.setCheckable(1)        
        self.groupBox.setLayout(self.vbox)
        self.gridLayout.addWidget(self.groupBox, 3, 6, 3, 3)


# spectralist
        self.spectraLabel = QtGui.QLabel("Spectra list: ", self.centralWidget)
        self.gridLayout.addWidget(self.spectraLabel,3,2,1,1)
        self.spectraEdit = QtGui.QLineEdit(self.centralWidget)
        self.spectraEdit.setObjectName(_fromUtf8("spectraEdit"))
        self.gridLayout.addWidget(self.spectraEdit, 3, 3, 1, 1)



# file format selector
        self.fileFormatLabel = QtGui.QLabel("File format: ", self.centralWidget)
        self.gridLayout.addWidget(self.fileFormatLabel,5,2,1,1)
        self.comboBox = QtGui.QComboBox(self.centralWidget)
        self.comboBox.setToolTip("Please select the file format")
        self.comboBox.setStatusTip("Please select the file format")
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
        self.comboBox.addItem(_fromUtf8("Custom format (*.dat)"))
        self.comboBox.addItem(_fromUtf8("3 column (*.dat)"))
        self.comboBox.addItem(_fromUtf8("ANSTO, MotoFit, 4 column (*.txt)"))
        self.comboBox.addItem(_fromUtf8("ILL Cosmos (*.mft)"))
        self.gridLayout.addWidget(self.comboBox, 5, 3, 1, 1)

        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton.sizePolicy().hasHeightForWidth())
        self.pushButton.setSizePolicy(sizePolicy)
        self.pushButton.setObjectName(_fromUtf8("pushButton"))
        self.gridLayout.addWidget(self.pushButton, 8, 2, 1, 1)
        spacerItem3 = QtGui.QSpacerItem(20, 28, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        self.gridLayout.addItem(spacerItem3, 8, 3, 1, 1)
        self.pushButton_2 = QtGui.QPushButton(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.pushButton_2.sizePolicy().hasHeightForWidth())
        self.pushButton_2.setSizePolicy(sizePolicy)
        self.pushButton_2.setObjectName(_fromUtf8("pushButton_2"))
        self.gridLayout.addWidget(self.pushButton_2, 8, 4, 1, 1)

        self.gridLayout_2.addLayout(self.gridLayout, 0, 0, 1, 1)



        self.retranslateUi(SaveWindow)
        self.populateList()
        #self.workspaceSelected()
        QtCore.QObject.connect(self.pushButton, QtCore.SIGNAL(_fromUtf8("clicked()")), self.buttonClickHandler1)
        QtCore.QObject.connect(self.pushButton_2, QtCore.SIGNAL(_fromUtf8("clicked()")), self.populateList)
        QtCore.QObject.connect(self.lineEdit, QtCore.SIGNAL(_fromUtf8("textChanged()")), self.setPath)
        QtCore.QObject.connect(self.comboBox, QtCore.SIGNAL(_fromUtf8("activated(QString)")), self.on_comboBox_Activated)
        QtCore.QObject.connect(self.listWidget, QtCore.SIGNAL(_fromUtf8("itemActivated(QListWidgetItem*)")), self.workspaceSelected)
     #   QtCore.QObject.connect(self.actionSave_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveDialog)
     #   QtCore.QObject.connect(self.actionLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.loadDialog)
        QtCore.QMetaObject.connectSlotsByName(SaveWindow)

    def retranslateUi(self, SaveWindow):
        SaveWindow.setWindowTitle(QtGui.QApplication.translate("SaveWindow", "SaveWindow", None, QtGui.QApplication.UnicodeUTF8))        
        self.pushButton.setText(QtGui.QApplication.translate("SaveWindow", "SAVE", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_2.setText(QtGui.QApplication.translate("SaveWindow", "Refresh", None, QtGui.QApplication.UnicodeUTF8))

    def setPath():
        Ui_MainWindow.SavePath=self.lineEdit.text()

    def workspaceSelected(self):
        self.listWidget2.clear()
        #self.listWidget.setCurrentRow(0)
        print str(self.listWidget.currentItem().text())
        logs = mantid[str(self.listWidget.currentItem().text())].getSampleDetails().getLogData()
        for i in range(0,len(logs)):
            self.listWidget2.addItem(logs[i].name)
    
    def on_comboBox_Activated(self):
        print ""

    def populateList(self):
        self.listWidget.clear()
        names = mtd.getObjectNames()
        if len(names):
            RB_Number=groupGet(names[0],'samp','rb_proposal')
            for ws in names:
                self.listWidget.addItem(ws)
                
            self.listWidget.setCurrentItem(self.listWidget.item(0))
            # try to get correct user directory
            currentInstrument=config['default.instrument']
            if (Ui_MainWindow.SavePath!=''):
                self.lineEdit.setText(Ui_MainWindow.SavePath)
            else:
                tree1=xml.parse(r'\\isis\inst$\NDX'+currentInstrument+'\Instrument\logs\journal\journal_main.xml')
                root1=tree1.getroot()
                currentJournal=root1[len(root1)-1].attrib.get('name')
                tree=xml.parse(r'\\isis\inst$\NDX'+currentInstrument+'\Instrument\logs\journal\\'+currentJournal)
                root=tree.getroot()
                
                i=0
                try:
                    while root[i][4].text!=str(RB_Number):
                        i+=1
                    if root[i][1].text.find(',')>0:
                        user=root[i][1].text[0:root[i][1].text.find(',')]
                    else:
                        user=root[i][1].text[0:root[i][1].text.find(' ')]
                    SavePath='U:/'+user+'/'
                    self.lineEdit.setText(SavePath)
                except LookupError:
                    print "Not found!"
                    
#--------- If "Save" button pressed, selcted workspaces are saved -------------
    def buttonClickHandler1(self):
        names = mtd.getObjectNames()
        dataToSave=[]
        prefix = str(self.lineEdit2.text())
        if (self.lineEdit.text()[len(self.lineEdit.text())-1] != '/'):
            path = self.lineEdit.text()+'/'
        else:
            path = self.lineEdit.text()

        for idx in self.listWidget.selectedItems():
            runlist=parseRunList(str(self.spectraEdit.text()))
            print runlist
            fname=str(path+prefix+idx.text())
            if (self.comboBox.currentIndex() == 0):
                fname+='.dat'
                print "FILENAME: ", fname
                a1=mantid.getMatrixWorkspace(str(idx.text()))
                titl='#'+a1.getTitle()+'\n'
                x1=a1.readX(0)
                X1=n.zeros((len(x1)-1))
                for i in range(0,len(x1)-1):
                    X1[i]=(x1[i]+x1[i+1])/2.0
                y1=a1.readY(0)
                e1=a1.readE(0)
                if (self.radio1.isChecked()):
                    sep=','
                elif (self.radio2.isChecked()):
                    sep=' '
                elif (self.radio3.isChecked()):
                    sep='\t'
                    
                f=open(fname,'w')
                if self.titleCheckBox.isChecked():
                    f.write(titl)
                samp = a1.getSampleDetails()
                for log in self.listWidget2.selectedItems():
                    
                    prop = samp.getLogData(str(log.text()))
                    headerLine='#'+log.text() + ': ' + str(prop.value) + '\n'
                    print headerLine
                    f.write(headerLine)
                qres=(X1[1]-X1[0])/X1[1]
                print "Constant dq/q from file: ",qres
                for i in range(len(X1)):
                    if self.xErrorCheckBox.isChecked():
                        dq=X1[i]*qres
                        s="%e" % X1[i] +sep+"%e" % y1[i] +sep + "%e" % e1[i] + sep + "%e" % dq +"\n"
                    else:
                        s="%e" % X1[i] +sep+"%e" % y1[i] +sep + "%e" % e1[i]+ "\n"
                    f.write(s)
                f.close()
            elif (self.comboBox.currentIndex() == 1):
                print "Not yet implemented!"
            elif (self.comboBox.currentIndex() == 2):
                print "ANSTO format"
                self.saveANSTO(idx,fname)
            elif (self.comboBox.currentIndex() == 3):
                print "ILL MFT format"
                self.saveMFT(idx,fname)


        # for idx in self.listWidget.selectedItems():
            # fname=str(path+prefix+idx.text()+'.dat')
            # print "FILENAME: ", fname
            # wksp=str(idx.text())
            # SaveAscii(InputWorkspace=wksp,Filename=fname)
            
        Ui_MainWindow.SavePath=self.lineEdit.text()

    def saveANSTO(self,idx,fname):
        fname+='.txt'
        print "FILENAME: ", fname
        a1=mantid.getMatrixWorkspace(str(idx.text()))
        titl='#'+a1.getTitle()+'\n'
        x1=a1.readX(0)
        X1=n.zeros((len(x1)-1))
        for i in range(0,len(x1)-1):
            X1[i]=(x1[i]+x1[i+1])/2.0
        y1=a1.readY(0)
        e1=a1.readE(0)
        sep='\t'
        f=open(fname,'w')
        qres=(X1[1]-X1[0])/X1[1]
        print "Constant dq/q from file: ",qres
        for i in range(len(X1)):
            dq=X1[i]*qres
            s="%e" % X1[i] +sep+"%e" % y1[i] +sep + "%e" % e1[i] + sep + "%e" % dq +"\n"
            f.write(s)
        f.close()

    def saveMFT(self,idx,fname):
        fname+='.mft'
        print "FILENAME: ", fname
        a1=mantid.getMatrixWorkspace(str(idx.text()))
        titl=a1.getTitle()+'\n'
        x1=a1.readX(0)
        X1=n.zeros((len(x1)-1))
        for i in range(0,len(x1)-1):
            X1[i]=(x1[i]+x1[i+1])/2.0
        y1=a1.readY(0)
        e1=a1.readE(0)
        sep='\t'
        f=open(fname,'w')
        f.write('MFT\n')
        f.write('Instrument: '+a1.getInstrument().getName()+'\n')
        f.write('User-local contact: \n')
        f.write('Title: \n')
        samp = a1.getSampleDetails()
        s = 'Subtitle: '+samp.getLogData('run_title').value+'\n'
        f.write(s)
        s = 'Start date + time: '+samp.getLogData('run_start').value+'\n'
        f.write(s)
        s = 'End date + time: '+samp.getLogData('run_end').value+'\n'
        f.write(s)
        
        for log in self.listWidget2.selectedItems():
            
            prop = samp.getLogData(str(log.text()))
            headerLine=log.text() + ': ' + str(prop.value) + '\n'
            print headerLine
            f.write(headerLine)
        f.write('Number of file format: 2\n')
        s = 'Number of data points:\t' + str(len(X1))+'\n'
        f.write(s)
        f.write('\n')
        f.write('\tq\trefl\trefl_err\tq_res\n')
        qres=(X1[1]-X1[0])/X1[1]
        print "Constant dq/q from file: ",qres
        for i in range(len(X1)):
            dq=X1[i]*qres
            s="\t%e" % X1[i] +sep+"%e" % y1[i] +sep + "%e" % e1[i] + sep + "%e" % dq +"\n"
            f.write(s)
        f.close()
