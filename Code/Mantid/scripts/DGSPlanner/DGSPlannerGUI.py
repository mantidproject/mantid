#pylint: disable=invalid-name
import InstrumentSetupWidget,ClassicUBInputWidget,MatrixUBInputWidget,DimensionSelectorWidget
from PyQt4 import QtCore, QtGui
import sys
import mantid
from ValidateOL import ValidateOL
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.pyplot
import numpy

class DGSPlannerGUI(QtGui.QWidget):
    def __init__(self,ol=None,parent=None):
        # pylint: disable=unused-argument
        super(DGSPlannerGUI,self).__init__(parent)
        #OrientedLattice
        if ValidateOL(ol):
            self.ol=ol
        else:
            self.ol=mantid.geometry.OrientedLattice()
        self.masterDict=dict() #holds info about instrument and ranges
        self.instrumentWidget=InstrumentSetupWidget.InstrumentSetupWidget(self)
        self.setLayout(QtGui.QHBoxLayout())
        controlLayout=QtGui.QVBoxLayout()
        controlLayout.addWidget(self.instrumentWidget)
        self.ublayout=QtGui.QHBoxLayout()
        self.classic=ClassicUBInputWidget.ClassicUBInputWidget(self.ol)
        self.ublayout.addWidget(self.classic)
        self.matrix=MatrixUBInputWidget.MatrixUBInputWidget(self.ol)
        self.ublayout.addStretch(1)
        self.ublayout.addWidget(self.matrix)
        controlLayout.addLayout(self.ublayout)
        self.dimensionWidget=DimensionSelectorWidget.DimensionSelectorWidget(self)
        controlLayout.addWidget(self.dimensionWidget)
        plotControlLayout=QtGui.QGridLayout()
        self.plotButton=QtGui.QPushButton("Plot",self)
        self.oplotButton=QtGui.QPushButton("Overplot",self)
        self.helpButton=QtGui.QPushButton("?",self)
        self.colorLabel=QtGui.QLabel('Color by angle',self)
        self.colorButton=QtGui.QCheckBox(self)
        plotControlLayout.addWidget(self.plotButton,0,0)
        plotControlLayout.addWidget(self.oplotButton,0,1)
        plotControlLayout.addWidget(self.colorLabel,0,2,QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.colorButton,0,3)
        plotControlLayout.addWidget(self.helpButton,0,4)
        controlLayout.addLayout(plotControlLayout)
        self.layout().addLayout(controlLayout)
        
        #figure
        self.figure=Figure()
        self.figure.patch.set_facecolor('white')
        self.canvas=FigureCanvas(self.figure)
        self.trajfig=self.figure.add_subplot(111)
        self.layout().addWidget(self.canvas)
        #self.updateFigure()
        
        #connections        
        self.matrix.UBmodel.changed.connect(self.updateUB)
        self.matrix.UBmodel.changed.connect(self.classic.updateOL)
        self.classic.changed.connect(self.matrix.UBmodel.updateOL)
        self.classic.changed.connect(self.updateUB)
        self.instrumentWidget.changed.connect(self.updateParams)
        self.dimensionWidget.changed.connect(self.updateParams)
        self.plotButton.clicked.connect(self.updateFigure)
        self.oplotButton.clicked.connect(self.updateFigure)
        self.helpButton.clicked.connect(self.help)
        #force an update of values
        self.instrumentWidget.updateAll()
        self.dimensionWidget.updateChanges()

    @QtCore.pyqtSlot(mantid.geometry.OrientedLattice)
    def updateUB(self,ol):
        print ol.getUB()
        
    @QtCore.pyqtSlot(dict)
    def updateParams(self,d):
        #print d
        self.masterDict.update(d)
    
    def help(self):
        #TODO: put the correct url. Check for assistant and try to use that first
        QtGui.QDesktopServices.openUrl(QtCore.QUrl("http://docs.mantidproject.org/nightly/concepts/Lattice.html"))
        
    def updateFigure(self):
        #print self.sender(),type(self.masterDict['dimStep'][0]),
        if self.sender() is self.plotButton:
            self.trajfig.clear()
        self.trajfig=self.figure.add_subplot(111)
        self.trajfig.hold(True)
        t=numpy.arange(0,3,0.01)
        s=numpy.sin(2*numpy.pi*t+self.masterDict['dimStep'][0])
        if self.colorButton.isChecked():
            s+=0.1
        self.trajfig.plot(t,s)    
        self.canvas.draw()   
            
if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    orl=mantid.geometry.OrientedLattice(2,3,4,90,90,90)
    mainForm=DGSPlannerGUI()
    mainForm.show()
    sys.exit(app.exec_())
