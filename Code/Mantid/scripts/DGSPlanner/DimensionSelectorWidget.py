#pylint: disable=invalid-name,no-name-in-module,too-many-instance-attributes
from PyQt4 import QtGui, QtCore
import sys
import numpy
try:
    from PyQt4.QtCore import QString
except ImportError:
    QString = type("")

class EmptyOrDoubleValidator(QtGui.QValidator):
    def __init__(self, parent):
        super(EmptyOrDoubleValidator,self).__init__()
    def validate(self,string, pos):
        if len(string)==0:
            return (QtGui.QValidator.Acceptable,pos)
        else:
            try:
                dummy=float(string)
                return (QtGui.QValidator.Acceptable,pos)
            except ValueError:
                return (QtGui.QValidator.Invalid,pos)

def FloatToQString(value):
    if numpy.isfinite(value):
        return QString(format(value,'.3f'))
    else:
        return QString("")

class DimensionSelectorWidget(QtGui.QWidget):
    changed=QtCore.pyqtSignal(dict)
    def __init__(self,parent=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(DimensionSelectorWidget,self).__init__()
        #validators
        self.doubleValidator=EmptyOrDoubleValidator(self)
        self.positiveDoubleValidator=QtGui.QDoubleValidator(self)
        self.positiveDoubleValidator.setBottom(1e-10)
        self.V3DValidator=V3DValidator(self)
        #labels
        self._labelBasis=QtGui.QLabel('    Projection Basis')
        self._labelBasis1=QtGui.QLabel('    Projection u')
        self._labelBasis2=QtGui.QLabel('    Projection v')
        self._labelBasis3=QtGui.QLabel('    Projection w')
        self._labelMin=QtGui.QLabel('Min')
        self._labelMax=QtGui.QLabel('Max')
        self._labelStep=QtGui.QLabel('Step')
        #lineedits
        self._editBasis1=QtGui.QLineEdit()
        self._editBasis2=QtGui.QLineEdit()
        self._editBasis3=QtGui.QLineEdit()
        self._editMin1=QtGui.QLineEdit()
        self._editMin1.setValidator(self.doubleValidator)
        self._editMin2=QtGui.QLineEdit()
        self._editMin2.setValidator(self.doubleValidator)
        self._editMin3=QtGui.QLineEdit()
        self._editMin3.setValidator(self.doubleValidator)
        self._editMin4=QtGui.QLineEdit()
        self._editMin4.setValidator(self.doubleValidator)
        self._editMax1=QtGui.QLineEdit()
        self._editMax1.setValidator(self.doubleValidator)
        self._editMax2=QtGui.QLineEdit()
        self._editMax2.setValidator(self.doubleValidator)
        self._editMax3=QtGui.QLineEdit()
        self._editMax3.setValidator(self.doubleValidator)
        self._editMax4=QtGui.QLineEdit()
        self._editMax4.setValidator(self.doubleValidator)
        self._editStep1=QtGui.QLineEdit()
        self._editStep1.setValidator(self.doubleValidator)
        self._editStep2=QtGui.QLineEdit()
        self._editStep2.setValidator(self.doubleValidator)
        self._comboDim1=QtGui.QComboBox(self)
        self._comboDim2=QtGui.QComboBox(self)
        self._comboDim3=QtGui.QComboBox(self)
        self._comboDim4=QtGui.QComboBox(self)
        #basis
        self.basis=['1,0,0','0,1,0','0,0,1']
        #default values
        self.dimNames=['[H,0,0]','[0,K,0]','[0,0,L]','DeltaE']
        self.dimMin=[-numpy.inf,-numpy.inf,-numpy.inf,-numpy.inf]
        self.dimMax=[numpy.inf,numpy.inf,numpy.inf,numpy.inf]
        self.dimStep=[0.05,0.05,0.05,1]
        #layout
        grid = QtGui.QGridLayout()
        self.setLayout(grid)
        grid.addWidget(self._labelMin,0,1)
        grid.addWidget(self._labelMax,0,2)
        grid.addWidget(self._labelStep,0,3)
        grid.addWidget(self._labelBasis,0,4,1,2)
        grid.addWidget(self._comboDim1,1,0)
        grid.addWidget(self._editMin1,1,1)
        grid.addWidget(self._editMax1,1,2)
        grid.addWidget(self._editStep1,1,3)
        grid.addWidget(self._labelBasis1,1,4)
        grid.addWidget(self._editBasis1,1,5)
        grid.addWidget(self._comboDim2,2,0)
        grid.addWidget(self._editMin2,2,1)
        grid.addWidget(self._editMax2,2,2)
        grid.addWidget(self._editStep2,2,3)
        grid.addWidget(self._labelBasis2,2,4)
        grid.addWidget(self._editBasis2,2,5)
        grid.addWidget(self._comboDim3,3,0)
        grid.addWidget(self._editMin3,3,1)
        grid.addWidget(self._editMax3,3,2)
        grid.addWidget(self._labelBasis3,3,4)
        grid.addWidget(self._editBasis3,3,5)
        grid.addWidget(self._comboDim4,4,0)
        grid.addWidget(self._editMin4,4,1)
        grid.addWidget(self._editMax4,4,2)
        self.updateGui()
        #connections
        self._editBasis1.textEdited.connect(self.basisChanged)
        self._editBasis2.textEdited.connect(self.basisChanged)
        self._editBasis3.textEdited.connect(self.basisChanged)
    
    def basisChanged(self):
        sender = self.sender()
        validator = sender.validator()
        state = validator.validate(sender.text(), 0)[0]
        if state == QtGui.QValidator.Acceptable:
            color = '#ffffff'
        else:
            color = '#ff0000'
        sender.setStyleSheet('QLineEdit { background-color: %s }' % color)
        #if state == QtGui.QValidator.Acceptable:
        #    self.validateAll()
        
    def updateGui(self):
        self._editBasis1.setText(QString(self.basis[0]))
        self._editBasis2.setText(QString(self.basis[1]))
        self._editBasis3.setText(QString(self.basis[2]))
        self._editMin1.setText(FloatToQString(self.dimMin[0]))
        self._editMin2.setText(FloatToQString(self.dimMin[1]))
        self._editMin3.setText(FloatToQString(self.dimMin[2]))
        self._editMin4.setText(FloatToQString(self.dimMin[3]))
        self._editMax1.setText(FloatToQString(self.dimMax[0]))
        self._editMax2.setText(FloatToQString(self.dimMax[1]))
        self._editMax3.setText(FloatToQString(self.dimMax[2]))
        self._editMax4.setText(FloatToQString(self.dimMax[3]))
        self._editStep1.setText(QString(format(self.dimStep[0],'.3f')))
        self._editStep2.setText(QString(format(self.dimStep[1],'.3f')))
        
        for name in self.dimNames:
            self._comboDim1.addItem(name)
            self._comboDim2.addItem(name)
            self._comboDim3.addItem(name)
            self._comboDim4.addItem(name)
        self._comboDim1.setCurrentIndex(0)
        self._comboDim2.setCurrentIndex(1)
        self._comboDim3.setCurrentIndex(2)
        self._comboDim4.setCurrentIndex(3)

if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    mainForm=DimensionSelectorWidget()
    mainForm.show()
    sys.exit(app.exec_())
