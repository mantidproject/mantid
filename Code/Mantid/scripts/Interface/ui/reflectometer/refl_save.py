from PyQt4 import QtCore, QtGui
import os
from mantid.simpleapi import *
from mantid.api import WorkspaceGroup
import xml.etree.ElementTree as xml
from isis_reflectometry.quick import *
from isis_reflectometry.procedures import *
from isis_reflectometry.combineMulti import *
from isis_reflectometry.saveModule import *
from isis_reflectometry.settings import MissingSettings, Settings

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_SaveWindow(object):
    def __init__(self):

        self.__has_mount_point = True

        self.__instrument = config['default.instrument'].strip().upper()

        try:
            usersettings = Settings() # This will throw a missing config exception if no config file is available.
            self.__mountpoint = usersettings.get_named_setting("DataMountPoint")
        except KeyError:
            print "DataMountPoint is missing from the config.xml file."
            self.__has_mount_point = False

    def setupUi(self, SaveWindow):
        self.SavePath=""
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
        self.filterLabel = QtGui.QLabel("Filter: ",self.centralWidget)
        self.gridLayout.addWidget(self.filterLabel,1,2,1,1)
        self.filterEdit = QtGui.QLineEdit(self.centralWidget)
        self.filterEdit.setFont(font)
        self.filterEdit.setObjectName(_fromUtf8("filterEdit"))
        self.gridLayout.addWidget(self.filterEdit, 1, 3, 1, 1)

        self.regExCheckBox = QtGui.QCheckBox("RegEx", self.centralWidget)
        self.gridLayout.addWidget(self.regExCheckBox, 1, 4, 1, 1)



        self.LogsLabel = QtGui.QLabel("List of logged parameters: ",self.centralWidget)
        self.gridLayout.addWidget(self.LogsLabel,1,6,1,3)


        self.ListLabel = QtGui.QLabel("List of workspaces: ",self.centralWidget)

# List of workspaces
        self.listWidget = QtGui.QListWidget(self.centralWidget)
        self.listWidget.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.listWidget.sizePolicy().hasHeightForWidth())

        self.workspacesLayout = QtGui.QBoxLayout(QtGui.QBoxLayout.TopToBottom)
        self.workspacesLayout.addWidget(self.ListLabel)
        self.workspacesLayout.addWidget(self.listWidget)
        self.gridLayout.addLayout(self.workspacesLayout,2,2,1,3)

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
        self.gridLayout.addWidget(self.spectraLabel,4,2,1,1)
        self.spectraEdit = QtGui.QLineEdit(self.centralWidget)
        self.spectraEdit.setObjectName(_fromUtf8("spectraEdit"))
        self.gridLayout.addWidget(self.spectraEdit, 4, 3, 1, 1)

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
        QtCore.QObject.connect(self.filterEdit, QtCore.SIGNAL(_fromUtf8("textChanged(QString)")), self.filterWksp)
        QtCore.QObject.connect(self.listWidget, QtCore.SIGNAL(_fromUtf8("itemActivated(QListWidgetItem*)")), self.workspaceSelected)
     #   QtCore.QObject.connect(self.actionSave_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.saveDialog)
     #   QtCore.QObject.connect(self.actionLoad_table, QtCore.SIGNAL(_fromUtf8("triggered()")), self.loadDialog)
        QtCore.QMetaObject.connectSlotsByName(SaveWindow)

    def retranslateUi(self, SaveWindow):
        SaveWindow.setWindowTitle(QtGui.QApplication.translate("SaveWindow", "SaveWindow", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton.setText(QtGui.QApplication.translate("SaveWindow", "SAVE", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_2.setText(QtGui.QApplication.translate("SaveWindow", "Refresh", None, QtGui.QApplication.UnicodeUTF8))

    def filterWksp(self):
        self.listWidget.clear()
        names = mtd.getObjectNames()
        if self.regExCheckBox.isChecked():
            regex=re.compile(self.filterEdit.text())
            filtered = list()
            for w in names:
                match = regex.search(w)
                if match:
                    filtered.append( match.string )
            newList = filtered
        else:
            newList=filter(lambda k: self.filterEdit.text() in k, names)

        self.listWidget.insertItems(0, newList)

    def setPath():
        self.SavePath=self.lineEdit.text()

    def workspaceSelected(self):
        self.listWidget2.clear()
        #self.listWidget.setCurrentRow(0)
        print str(self.listWidget.currentItem().text())
        logs = mtd[str(self.listWidget.currentItem().text())].getRun().getLogData()
        for i in range(0,len(logs)):
            self.listWidget2.addItem(logs[i].name)

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
            if (self.SavePath!=''):
                self.lineEdit.setText(self.SavePath)
            else:
                if self.__has_mount_point:
                    try:
                        base_path = os.path.join(self.__mountpoint, 'NDX'+  self.__instrument, 'Instrument','logs','journal')
                        print "Loading journal from", base_path
                        main_journal_path = os.path.join(base_path, 'journal_main.xml')
                        tree1=xml.parse(main_journal_path)
                        root1=tree1.getroot()
                        currentJournal=root1[len(root1)-1].attrib.get('name')
                        cycle_journal_path = os.path.join(base_path, currentJournal)
                        tree=xml.parse(cycle_journal_path)
                        root=tree.getroot()
                        i=0
                        try:
                            while root[i][4].text!=str(RB_Number):
                                i+=1
                            if root[i][1].text.find(',')>0:
                                user=root[i][1].text[0:root[i][1].text.find(',')]
                            else:
                                user=root[i][1].text[0:root[i][1].text.find(' ')]
                            SavePath = os.path.join('U:', user)
                            self.lineEdit.setText(SavePath)
                        except LookupError:
                            print "Couldn't find user name in archives!"
                    except:
                        print "Journal does not exist or is unreachable, please check your network connection."

#--------- If "Save" button pressed, selcted workspaces are saved -------------
    def buttonClickHandler1(self):
        names = mtd.getObjectNames()
        dataToSave=[]
        prefix = str(self.lineEdit2.text())
        if not (self.lineEdit.text() and os.path.exists(self.lineEdit.text())):
            logger.notice("Directory specified doesn't exist or was invalid for your operating system")
            QtGui.QMessageBox.critical(self.lineEdit, 'Could not save',"Directory specified doesn't exist or was invalid for your operating system")
            return
        for idx in self.listWidget.selectedItems():
            runlist=parseRunList(str(self.spectraEdit.text()))
            fname=os.path.join(self.lineEdit.text(),prefix + idx.text())
            if (self.comboBox.currentIndex() == 0):
                print "Custom Ascii format"
                if (self.radio1.isChecked()):
                    sep=','
                elif (self.radio2.isChecked()):
                    sep=' '
                elif (self.radio3.isChecked()):
                    sep='\t'
                else:
                    sep=' '
                saveCustom(idx,fname,sep,self.listWidget2.selectedItems(),self.titleCheckBox.isChecked(),self.xErrorCheckBox.isChecked())
            elif (self.comboBox.currentIndex() == 1):
                print "Not yet implemented!"
            elif (self.comboBox.currentIndex() == 2):
                print "ANSTO format"
                saveANSTO(idx,fname)
            elif (self.comboBox.currentIndex() == 3):
                print "ILL MFT format"
                saveMFT(idx,fname,self.listWidget2.selectedItems())
        # for idx in self.listWidget.selectedItems():
            # fname=str(path+prefix+idx.text()+'.dat')
            # print "FILENAME: ", fname
            # wksp=str(idx.text())
            # SaveAscii(InputWorkspace=wksp,Filename=fname)

        self.SavePath=self.lineEdit.text()

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
