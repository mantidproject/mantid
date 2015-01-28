from PyQt4 import QtCore, QtGui
import sys
import mantid

try:
    from PyQt4.QtCore import QString
except ImportError:
    QString = type("")
    
class InstrumentSetupWidget(QtGui.QWidget): 
    def __init__(self,ol=None,parent=None):
        super(InstrumentSetupWidget,self).__init__()
        #instrument selector
        self.instrumentList=['ARCS','CNCS','HYSPEC','SEQUOIA']
        self.combo = QtGui.QComboBox(self)
        for inst in self.instrumentList:
            self.combo.addItem(inst)
        defaultInstrument=mantid.config.getInstrument().name()
        if defaultInstrument in self.instrumentList:
            self.instrument=defaultInstrument
            self.combo.setCurrentIndex(self.instrumentList.index(defaultInstrument))
        else:
            self.instrument=self.InstrumentList[0]
            self.combo.setCurrentIndex(0)
        self.labelInst=QtGui.QLabel('Instrument')
        #S2 and Ei edits
        self.S2=0.0
        self.Ei=10.0
        self.validatorS2=QtGui.QDoubleValidator(-90.,90.,5,self)
        self.validatorEi=QtGui.QDoubleValidator(1.,10000.,5,self)
        self.labelS2=QtGui.QLabel('HYSPEC S2')
        self.labelEi=QtGui.QLabel('Incident Energy:')
        self.editS2=QtGui.QLineEdit() 
        self.editS2.setValidator(self.validatorS2)
        self.editEi=QtGui.QLineEdit() 
        self.editEi.setValidator(self.validatorEi)
        self.editS2.setText(QString(format(self.S2,'.2f')))   
        self.editEi.setText(QString(format(self.Ei,'.1f')))
        #goniometer settings
        self.labelGon=QtGui.QLabel('Goniometer')
        self.tableViewGon = QtGui.QTableView(self)
        self.tableViewGon.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self.tableViewGon.verticalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        #self.goniomodel = gonioTableModel(self.gonio,self)
        #self.tableViewGon.setModel(self.goniomodel)
        #self.tableViewGon.update()
        #layout
        self.gridI = QtGui.QGridLayout()
        self.gridI.addWidget(self.labelInst,0,0)
        self.gridI.addWidget(self.combo,0,1)
        self.gridI.addWidget(self.labelS2,1,0)
        self.gridI.addWidget(self.editS2,1,1)
        self.gridI.addWidget(self.labelEi,2,0)
        self.gridI.addWidget(self.editEi,2,1)
        self.setLayout(QtGui.QHBoxLayout())
        self.layout().addLayout(self.gridI)
        self.rightside=QtGui.QVBoxLayout()
        self.rightside.addWidget(self.labelGon)
        self.rightside.addWidget(self.tableViewGon)
        self.layout().addLayout(self.rightside)
        #connections
        self.editS2.textEdited.connect(self.checkValidInputs)
        self.editEi.textEdited.connect(self.checkValidInputs)
        self.combo.activated[str].connect(self.instrumentSelected)
        #call instrumentSelected once. this will update everything else
        self.instrumentSelected(self.instrument)
        
    def instrumentSelected(self,text):
        self.instrument=text
        if self.instrument=="HYSPEC":
            self.labelS2.show()
            self.editS2.show()
        else:
            self.labelS2.hide()
            self.editS2.hide()
        self.updateAll()
        
    def checkValidInputs(self, *args, **kwargs):
        sender = self.sender()
        validator = sender.validator()
        state = validator.validate(sender.text(), 0)[0]
   
        if state == QtGui.QValidator.Acceptable:
            color = '#ffffff' 
            if sender==self.editS2:
                self.S2=float(sender.text())
            if sender==self.editEi:
                self.Ei=float(sender.text())
        elif state == QtGui.QValidator.Intermediate:
            color = '#ffaaaa' 
        else:
            color = '#ffaaaa' 
        sender.setStyleSheet('QLineEdit { background-color: %s }' % color)
        if state == QtGui.QValidator.Acceptable:
            self.updateAll()
        
    def updateAll(self):
        print self.instrument,self.Ei,self.S2
        if mantid.mtd.doesExist("__InstrumentSetupWidgetWorkspace"):
            __w=mantid.mtd["__InstrumentSetupWidgetWorkspace"]
        else:
            __w=mantid.simpleapi.CreateSingleValuedWorkspace(0., OutputWorkspace="__InstrumentSetupWidgetWorkspace")
        mantid.simpleapi.AddSampleLog(__w,LogName="Ei",LogText=str(self.Ei), LogType="Number Series")
        mantid.simpleapi.AddSampleLog(__w,LogName="s2",LogText=str(self.S2), LogType="Number Series")
        mantid.simpleapi.LoadInstrument(__w,InstrumentName=str(self.instrument))
        #self.editS2.setText(QString(format(self.S2,'.2f')))   
        #self.editEi.setText(QString(format(self.Ei,'.1f')))
        
        
if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    mainForm=InstrumentSetupWidget()
    mainForm.show()
    sys.exit(app.exec_())
