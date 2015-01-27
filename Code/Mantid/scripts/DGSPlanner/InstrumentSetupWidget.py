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
        #layout
        self.gridI = QtGui.QGridLayout()
        self.gridI.addWidget(self.labelS2,0,0)
        self.gridI.addWidget(self.editS2,0,1)
        self.gridI.addWidget(self.labelEi,1,0)
        self.gridI.addWidget(self.editEi,1,1)
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().addWidget(self.combo)
        self.layout().addLayout(self.gridI)
        #connections
        self._edita.textEdited.connect(self.check_state_latt)
        self._editb.textEdited.connect(self.check_state_latt)
        self.combo.activated[str].connect(self.instrumentSelected)
        #call instrumentSelected once
        self.instrumentSelected(self.instrument)
        
    def instrumentSelected(self,text):
        self.instrument=text
        if self.instrument=="HYSPEC":
            self.labelS2.show()
            self.editS2.show()
        else:
            self.labelS2.hide()
            self.editS2.hide()
            
    
if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    mainForm=InstrumentSetupWidget()
    mainForm.show()
    sys.exit(app.exec_())
