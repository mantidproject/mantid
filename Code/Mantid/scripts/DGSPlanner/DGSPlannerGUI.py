#pylint: disable=invalid-name
import InstrumentSetupWidget,ClassicUBInputWidget,MatrixUBInputWidget,DimensionSelectorWidget
from PyQt4 import QtCore, QtGui
import sys
import mantid
from ValidateOL import ValidateOL
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from  mpl_toolkits.axisartist.grid_helper_curvelinear import GridHelperCurveLinear
from mpl_toolkits.axisartist import Subplot
import matplotlib.pyplot
import numpy
import copy
import os

def float2Input(x):
    if numpy.isfinite(x):
        return x
    else:
        return None

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
        self.updatedInstrument=False
        self.updatedOL=False
        self.wg=None #workspace group
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
        self.colorButton.toggle()
        self.aspectLabel=QtGui.QLabel('Aspect ratio 1:1',self)
        self.aspectButton=QtGui.QCheckBox(self)
        self.saveButton=QtGui.QPushButton("Save Figure",self)
        plotControlLayout.addWidget(self.plotButton,0,0)
        plotControlLayout.addWidget(self.oplotButton,0,1)
        plotControlLayout.addWidget(self.colorLabel,0,2,QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.colorButton,0,3)
        plotControlLayout.addWidget(self.aspectLabel,0,4,QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.aspectButton,0,5)
        plotControlLayout.addWidget(self.helpButton,0,6)
        plotControlLayout.addWidget(self.saveButton,0,7)
        controlLayout.addLayout(plotControlLayout)
        self.layout().addLayout(controlLayout)
        
        #figure
        self.figure=Figure()
        self.figure.patch.set_facecolor('white')
        self.canvas=FigureCanvas(self.figure)
        self.grid_helper = GridHelperCurveLinear((self.tr, self.inv_tr))
        self.trajfig = Subplot(self.figure, 1, 1, 1, grid_helper=self.grid_helper)
        self.trajfig.hold(True)
        self.figure.add_subplot(self.trajfig)
        self.layout().addWidget(self.canvas)
        self.needToClear=False
        self.saveDir=''
        
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
        self.saveButton.clicked.connect(self.save)
        #force an update of values
        self.instrumentWidget.updateAll()
        self.dimensionWidget.updateChanges()

    @QtCore.pyqtSlot(mantid.geometry.OrientedLattice)
    def updateUB(self,ol):
        self.ol=ol
        self.updatedOL=True
        self.trajfig.clear()

    @QtCore.pyqtSlot(dict)
    def updateParams(self,d):
        if self.sender() is self.instrumentWidget:
            self.updatedInstrument=True
        if d.has_key('dimBasis') and self.masterDict.has_key('dimBasis') and d['dimBasis']!=self.masterDict['dimBasis']:
            self.needToClear=True
        if d.has_key('dimIndex') and self.masterDict.has_key('dimIndex')and d['dimIndex']!=self.masterDict['dimIndex']:
            self.needToClear=True
        self.masterDict.update(copy.deepcopy(d))
    
    def help(self):
        #TODO: put the correct url. Check for assistant and try to use that first
        QtGui.QDesktopServices.openUrl(QtCore.QUrl("http://docs.mantidproject.org/nightly/concepts/Lattice.html"))
        
    def updateFigure(self):
        if self.updatedInstrument:
            #get goniometer settings first
            gonioAxis0values=numpy.arange(self.masterDict['gonioMinvals'][0],self.masterDict['gonioMaxvals'][0]
                                          +0.1*self.masterDict['gonioSteps'][0],self.masterDict['gonioSteps'][0])
            gonioAxis1values=numpy.arange(self.masterDict['gonioMinvals'][1],self.masterDict['gonioMaxvals'][1]
                                          +0.1*self.masterDict['gonioSteps'][1],self.masterDict['gonioSteps'][1])
            gonioAxis2values=numpy.arange(self.masterDict['gonioMinvals'][2],self.masterDict['gonioMaxvals'][2]
                                          +0.1*self.masterDict['gonioSteps'][2],self.masterDict['gonioSteps'][2])
            if len(gonioAxis0values)*len(gonioAxis1values)*len(gonioAxis2values)>10:
                reply = QtGui.QMessageBox.warning(self, 'Goniometer',"More than 10 goniometer settings. This might be long.\n"
                                                  "Are you sure you want to proceed?", QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                                  QtGui.QMessageBox.No)
                if reply==QtGui.QMessageBox.No:
                    return
            if self.wg!=None:
                mantid.simpleapi.DeleteWorkspace(self.wg)
            mantid.simpleapi.LoadEmptyInstrument(mantid.api.ExperimentInfo.getInstrumentFilename(self.masterDict['instrument']),
                                                 OutputWorkspace="__temp_instrument")
            if self.masterDict['instrument']=='HYSPEC':
                mantid.simpleapi.AddSampleLog(Workspace="__temp_instrument",LogName='msd',LogText='1798.5',LogType='Number Series')
                mantid.simpleapi.AddSampleLog(Workspace="__temp_instrument",LogName='s2',
                                              LogText=self.masterDict['S2'],LogType='Number Series')
                mantid.simpleapi.LoadInstrument(Workspace="__temp_instrument",Instrument="HYSPEC")
            #masking
            if self.masterDict['makeFast']:
                sp=range(mantid.mtd["__temp_instrument"].getNumberHistograms())
                tomask=sp[::4]+sp[1::4]+sp[2::4]
                mantid.simpleapi.MaskDetectors("__temp_instrument",SpectraList=tomask)
            i=0
            groupingStrings=[]
            for g0 in gonioAxis0values:
                for g1 in gonioAxis1values:
                    for g2 in gonioAxis2values:
                        name="__temp_instrument"+str(i)
                        i+=1
                        groupingStrings.append(name)
                        mantid.simpleapi.CloneWorkspace("__temp_instrument",OutputWorkspace=name)
                        mantid.simpleapi.SetGoniometer(Workspace=name,
                                                       Axis0=str(g0)+","+self.masterDict['gonioDirs'][0]+","+str(self.masterDict['gonioSenses'][0]),
                                                       Axis1=str(g1)+","+self.masterDict['gonioDirs'][1]+","+str(self.masterDict['gonioSenses'][1]),
                                                       Axis2=str(g2)+","+self.masterDict['gonioDirs'][2]+","+str(self.masterDict['gonioSenses'][2]))
            mantid.simpleapi.DeleteWorkspace("__temp_instrument")
            self.wg=mantid.simpleapi.GroupWorkspaces(groupingStrings,OutputWorkspace="__temp_instrument")
            self.updatedInstrument=False
        #set the UB
        if self.updatedOL or not self.wg[0].sample().hasOrientedLattice():
            mantid.simpleapi.SetUB(self.wg,UB=self.ol.getUB())
            self.updatedOL=False
        #calculate coverage
        dimensions=['Q1','Q2','Q3','DeltaE']
        __mdws=mantid.simpleapi.CalculateCoverageDGS(self.wg,
                                                    Q1Basis=self.masterDict['dimBasis'][0],
                                                    Q2Basis=self.masterDict['dimBasis'][1],
                                                    Q3Basis=self.masterDict['dimBasis'][2],
                                                    IncidentEnergy=self.masterDict['Ei'],
                                                    Dimension1=dimensions[self.masterDict['dimIndex'][0]],
                                                    Dimension1Min=float2Input(self.masterDict['dimMin'][0]), 
                                                    Dimension1Max=float2Input(self.masterDict['dimMax'][0]), 
                                                    Dimension1Step=float2Input(self.masterDict['dimStep'][0]),
                                                    Dimension2=dimensions[self.masterDict['dimIndex'][1]],
                                                    Dimension2Min=float2Input(self.masterDict['dimMin'][1]), 
                                                    Dimension2Max=float2Input(self.masterDict['dimMax'][1]),
                                                    Dimension2Step=float2Input(self.masterDict['dimStep'][1]),
                                                    Dimension3=dimensions[self.masterDict['dimIndex'][2]],
                                                    Dimension3Min=float2Input(self.masterDict['dimMin'][2]), 
                                                    Dimension3Max=float2Input(self.masterDict['dimMax'][2]),
                                                    Dimension4=dimensions[self.masterDict['dimIndex'][3]],
                                                    Dimension4Min=float2Input(self.masterDict['dimMin'][3]), 
                                                    Dimension4Max=float2Input(self.masterDict['dimMax'][3]))
        intensity=__mdws[0].getSignalArray()*1. #to make it writeable
        if self.colorButton.isChecked():
            for i in range(__mdws.getNumberOfEntries())[1:]:
                tempintensity=  __mdws[i].getSignalArray()
                intensity[numpy.where( tempintensity>0)]=i+1.
        else:
            for i in range(__mdws.getNumberOfEntries())[1:]:
                tempintensity=  __mdws[i].getSignalArray()
                intensity[numpy.where( tempintensity>0)]=1.      
        Z = numpy.transpose(intensity)
        x = numpy.linspace(__mdws[0].getDimension(0).getMinimum(), __mdws[0].getDimension(0).getMaximum(),intensity.shape[1] )
        y = numpy.linspace(__mdws[0].getDimension(1).getMinimum(), __mdws[0].getDimension(1).getMaximum(),intensity.shape[0] )
        X,Y = numpy.meshgrid(x,y,indexing='ij')
        xx, yy = self.tr(X, Y)
        Z=numpy.ma.masked_array(Z,Z==0)
        Z = Z[:-1, :-1]
        #plotting
        if self.sender() is self.plotButton or self.needToClear:
            self.trajfig.clear()
            self.needToClear=False
        self.trajfig.pcolorfast(xx,yy,Z)

        if self.aspectButton.isChecked():
            self.trajfig.set_aspect(1.)
        else:
            self.trajfig.set_aspect('auto')
        self.trajfig.set_xlabel(self.masterDict['dimNames'][0])
        self.trajfig.set_ylabel(self.masterDict['dimNames'][1])
        self.trajfig.grid(True)   
        self.canvas.draw()
        mantid.simpleapi.DeleteWorkspace(__mdws)

    def save(self):
        fileName = str(QtGui.QFileDialog.getSaveFileName(self, 'Save Plot', self.saveDir,'*.png'))
        self.figure.savefig(fileName)
        self.saveDir=os.path.dirname(fileName)

    def tr(self,x, y):
        x, y = numpy.asarray(x), numpy.asarray(y)
        #one of the axes is energy
        if self.masterDict['dimIndex'][0]==3 or self.masterDict['dimIndex'][1]==3:
            return x,y
        else:
            h1,k1,l1=(float(temp) for temp in self.masterDict['dimBasis'][self.masterDict['dimIndex'][0]].split(','))
            h2,k2,l2=(float(temp) for temp in self.masterDict['dimBasis'][self.masterDict['dimIndex'][1]].split(','))
            angle=numpy.radians(self.ol.recAngle(h1,k1,l1,h2,k2,l2))
            return 1.*x+numpy.cos(angle)*y,  numpy.sin(angle)*y
    def inv_tr(self,x,y):
        x, y = numpy.asarray(x), numpy.asarray(y)
        #one of the axes is energy
        if self.masterDict['dimIndex'][0]==3 or self.masterDict['dimIndex'][1]==3:
            return x,y
        else:
            h1,k1,l1=(float(temp) for temp in self.masterDict['dimBasis'][self.masterDict['dimIndex'][0]].split(','))
            h2,k2,l2=(float(temp) for temp in self.masterDict['dimBasis'][self.masterDict['dimIndex'][1]].split(','))
            angle=numpy.radians(self.ol.recAngle(h1,k1,l1,h2,k2,l2))
            return 1.*x-y/numpy.tan(angle),  y/numpy.sin(angle)
      
if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    orl=mantid.geometry.OrientedLattice(2,3,4,90,90,90)
    mainForm=DGSPlannerGUI()
    mainForm.show()
    sys.exit(app.exec_())
