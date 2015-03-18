#pylint: disable=invalid-name,no-name-in-module,too-many-instance-attributes
from PyQt4 import QtGui, QtCore
import sys
import mantid
import numpy
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot
try:
    from PyQt4.QtCore import QString
except ImportError:
    QString = type("")


class GonioTableModel(QtCore.QAbstractTableModel):
    """
    Dealing with the goniometer input
    """
    changed=QtCore.pyqtSignal(dict) #each value is a list
    def __init__(self, axes, parent = None):
        QtCore.QAbstractTableModel.__init__(self, parent)
        self.labels = axes['labels']
        self.dirstrings = axes['dirs']
        self.senses = axes['senses']
        self.minvalues = axes['minvals']
        self.maxvalues = axes['maxvals']
        self.steps = axes['steps']
        self.gonioColumns=['Name','Direction','Sense','Minimum','Maximum','Step']
        self.gonioRows=['Axis0','Axis1','Axis2']

    def rowCount(self, dummy_parent):
        return 3

    def columnCount(self, dummy_parent):
        return 6

    def flags(self, dummy_index):
        return QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable

    def headerData(self, section, Qt_Orientation, role=None):
        if role == QtCore.Qt.DisplayRole and Qt_Orientation == QtCore.Qt.Horizontal:
            return self.gonioColumns[section]
        if role == QtCore.Qt.DisplayRole and Qt_Orientation == QtCore.Qt.Vertical:
            return self.gonioRows[section]

    def data(self, index, role):
        row = index.row()
        column = index.column()
        if role == QtCore.Qt.EditRole or role == QtCore.Qt.DisplayRole:
            
            if column==0:
                value=QString(self.labels[row])
            elif column==1:
                value=QString(self.dirstrings[row])
            elif column==2:
                value=QString(str(self.senses[row]))
            elif column==3:
                value=QString(str(self.minvalues[row]))
            elif column==4:
                value=QString(str(self.maxvalues[row]))
            elif column==5:
                value=QString(str(self.steps[row]))
            return value
        elif role == QtCore.Qt.BackgroundRole:
            if column==0 and len(self.labels[row])>0 and self.labels.count(self.labels[row])==1:
                return QtGui.QBrush(QtCore.Qt.white)
            elif column==1 and self.validDir(self.dirstrings[row]):
                return QtGui.QBrush(QtCore.Qt.white)
            elif column==2 and (self.senses[row]==1 or self.senses[row]==-1):
                return QtGui.QBrush(QtCore.Qt.white)
            elif (column==3 or column==4) and self.minvalues[row]<=self.maxvalues[row]:
                return QtGui.QBrush(QtCore.Qt.white)
            elif column==5 and self.steps[row]>0:
                return QtGui.QBrush(QtCore.Qt.white)
            else:
                return QtGui.QBrush(QtCore.Qt.red)

    def setData(self, index, value, role = QtCore.Qt.EditRole):
        if role == QtCore.Qt.EditRole:
            row = index.row()
            column = index.column()
            if column<=1:
                try:
                    val=str(value.toString()) #QVariant
                except AttributeError:
                    val=str(value) #string
                if column==0:
                    self.labels[row]=val
                else:
                    self.dirstrings[row]=val
            elif column==2:
                try:
                    val=value.toInt()[0] #QVariant
                except AttributeError:
                    val=int(value) #string
                self.senses[row]=val
            else:
                try:
                    val=value.toFloat()[0] #QVariant
                except AttributeError:
                    val=float(value) #string
                if column==3:
                    self.minvalues[row]=val
                elif column==4:
                    self.maxvalues[row]=val
                else:
                    self.steps[row]=val
            self.dataChanged.emit(index, index)
            if self.validateGon():
                values={'labels':self.labels,'dirs':self.dirstrings,'senses':self.senses,
                        'minvals':self.minvalues,'maxvals':self.maxvalues,'steps':self.steps}
                self.changed.emit(values)
                return True
        return False
    
    def validDir(self,dirstring):
        d=numpy.fromstring(dirstring,dtype=float,sep=',')
        if len(d)==3:
            return numpy.alltrue(numpy.isfinite(d))
        return False

    def validateGon(self):
        for i in range(3):
            if len(self.labels[i])==0 or self.labels.count(self.labels[i])>1 or self.senses[i] not in [-1,1]:
                return False
            if not self.validDir(self.dirstrings[i]):
                return False
            if self.minvalues[i]>self.maxvalues[i] or self.steps[i]<=0:
                return False
        return True

class InstrumentSetupWidget(QtGui.QWidget):
    def __init__(self,parent=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(InstrumentSetupWidget,self).__init__()
        metrics=QtGui.QFontMetrics(self.font())
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
        self.editEi.setFixedWidth(metrics.width("8888.88"))
        self.editS2.setFixedWidth(metrics.width("888.88"))
        #goniometer settings
        self.labelGon=QtGui.QLabel('Goniometer')
        self.tableViewGon = QtGui.QTableView(self)
        self.tableViewGon.setMinimumWidth(metrics.width("Minimum ")*7)
        self.tableViewGon.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self.tableViewGon.verticalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self.goniometerNames=['psi','gl','gs']
        self.goniometerDirections=['0,1,0','0,0,1','1,0,0']
        self.goniometerRotationSense=[1,1,1]
        self.goniometerMin=[0.,0.,0.]
        self.goniometerMax=[0.,0.,0.]
        self.goniometerStep=[1.,1.,1.]
        values={'labels':self.goniometerNames,'dirs':self.goniometerDirections,'senses':self.goniometerRotationSense,
                'minvals':self.goniometerMin,'maxvals':self.goniometerMax,'steps':self.goniometerStep}
        self.goniomodel = GonioTableModel(values,self)
        self.tableViewGon.setModel(self.goniomodel)
        self.tableViewGon.update()
        #goniometer figure
        self.figure=Figure(figsize=(2,4))
        self.figure.patch.set_facecolor('white')
        self.canvas=FigureCanvas(self.figure)
        self.gonfig=None
        self.updateFigure()
        #layout
        self.gridI = QtGui.QGridLayout()
        self.gridI.addWidget(self.labelInst,0,0)
        self.gridI.addWidget(self.combo,0,1)
        self.gridI.addWidget(self.labelEi,0,2)
        self.gridI.addWidget(self.editEi,0,3)
        self.gridI.addWidget(self.labelS2,0,4)
        self.gridI.addWidget(self.editS2,0,5)
        self.setLayout(QtGui.QHBoxLayout())
        self.rightside=QtGui.QVBoxLayout()
        self.layout().addLayout(self.rightside)
        self.rightside.addLayout(self.gridI)
        self.rightside.addWidget(self.labelGon)
        self.rightside.addWidget(self.tableViewGon)
        self.layout().addWidget(self.canvas)
        #connections
        self.editS2.textEdited.connect(self.checkValidInputs)
        self.editEi.textEdited.connect(self.checkValidInputs)
        self.combo.activated[str].connect(self.instrumentSelected)
        #call instrumentSelected once. this will update everything else
        self.instrumentSelected(self.instrument)
        #connect goniometer change with figure
        self.goniomodel.changed.connect(self.updateGon)

    def updateGon(self,axes):
        self.goniometerNames=axes['labels']
        self.updateFigure()

    def updateFigure(self):
        #plot directions
        if self.gonfig is not None:
            self.gonfig.clear()
        self.gonfig = Axes3D(self.figure)
        self.gonfig.hold(True)
        self.gonfig.set_frame_on(False)
        self.gonfig.set_xlim3d(-0.6,0.6)
        self.gonfig.set_ylim3d(-0.6,0.6)
        self.gonfig.set_zlim3d(-1,5)
        self.gonfig.set_axis_off()
        self.gonfig.plot([0,1],[-3,-3],[0,0],zdir='y',color='black')
        self.gonfig.plot([0,0],[-3,-2],[0,0],zdir='y',color='black')
        self.gonfig.plot([0,0],[-3,-3],[0,1],zdir='y',color='black')
        self.gonfig.text(0,1,-2.5,'Z',zdir=None,color='black')
        self.gonfig.text(1,0,-2.5,'X',zdir=None,color='black')
        
        matplotlib.pyplot.gca().set_aspect('equal', adjustable='datalim')
        self.gonfig.view_init(10,45)
        
        colors=['b','g','r']
        for i in range(3):
            circle=numpy.array([mantid.kernel.Quat(0,0,0.5*numpy.sin(t),0.5*numpy.cos(t)) for t in numpy.arange(0,1.51*numpy.pi,0.1*numpy.pi)])
            if self.goniometerRotationSense[i]==1:
                circle=numpy.append(circle,mantid.kernel.Quat(0,0,-0.45,-0.05))
                circle=numpy.append(circle,mantid.kernel.Quat(0,0,-0.55,-0.05))
                circle=numpy.append(circle,mantid.kernel.Quat(0,0,-0.5,0))
            else:
                circle=numpy.insert(circle,0,mantid.kernel.Quat(0,0,0,0.5))
                circle=numpy.insert(circle,1,mantid.kernel.Quat(0,0,0.05,0.45))
                circle=numpy.insert(circle,2,mantid.kernel.Quat(0,0,0.05,0.55))

            t=numpy.fromstring(self.goniometerDirections[i],dtype=float,sep=',')
            vt=mantid.kernel.V3D(t[0],t[1],t[2])
            vt*=(1./vt.norm())
            direction=mantid.kernel.Quat(mantid.kernel.V3D(1,0,0),vt)
            directionS=mantid.kernel.Quat(direction[0],-direction[1],-direction[2],-direction[3])
            gonAxis=numpy.array([mantid.kernel.Quat(0,1,0,0),mantid.kernel.Quat(0,-1,0,0)])

            newcircle=direction*circle*directionS
            newgonAxis=direction*gonAxis*directionS
            parray=numpy.array([(p[1],p[2]+2*i,p[3]) for p in newcircle])
            self.gonfig.plot(parray[:,0],parray[:,1],parray[:,2],zdir='y',color=colors[i])
            parray=numpy.array([(p[1],p[2]+2*i,p[3]) for p in newgonAxis])
            self.gonfig.plot(parray[:,0],parray[:,1],parray[:,2],zdir='y',color=colors[i])
            self.gonfig.plot([t[0],-t[0]],[t[1]+2*i,-t[1]+2*i],[t[2],-t[2]],zdir='y',color=colors[i])
            self.gonfig.text(0,1,2*i,self.goniometerNames[i],zdir=None,color=colors[i])

        #plot sample
        self.gonfig.text(0,0,6.7,'Sample',zdir=None,color='black')
        u=numpy.linspace(0,2*numpy.pi,50)
        v=numpy.linspace(0,numpy.pi,50)
        x = 0.3 * numpy.outer(numpy.cos(u), numpy.sin(v))
        y = 0.3 * numpy.outer(numpy.sin(u), numpy.sin(v))
        z = 0.3 * numpy.outer(numpy.ones(numpy.size(u)),numpy. cos(v))
        self.gonfig.plot_surface(x,y,z+6,color='black',rstride=4, cstride=4)
        self.canvas.draw()

    def instrumentSelected(self,text):
        self.instrument=text
        if self.instrument=="HYSPEC":
            self.labelS2.show()
            self.editS2.show()
        else:
            self.labelS2.hide()
            self.editS2.hide()
        self.updateAll()

    def checkValidInputs(self, *dummy_args, **dummy_kwargs):
        sender = self.sender()
        state = sender.validator().validate(sender.text(), 0)[0]
        if state == QtGui.QValidator.Acceptable:
            color = '#ffffff'
            if sender==self.editS2:
                self.S2=float(sender.text())
            if sender==self.editEi:
                self.Ei=float(sender.text())
        else:
            color = '#ff0000'
        sender.setStyleSheet('QLineEdit { background-color: %s }' % color)
        if state == QtGui.QValidator.Acceptable:
            self.updateAll()

    def updateAll(self):
        pass
        #if mantid.mtd.doesExist("__InstrumentSetupWidgetWorkspace"):
        #    __w=mantid.mtd["__InstrumentSetupWidgetWorkspace"]
        #else:
        #    __w=mantid.simpleapi.CreateSingleValuedWorkspace(0., OutputWorkspace="__InstrumentSetupWidgetWorkspace")
        #mantid.simpleapi.AddSampleLog(__w,LogName="Ei",LogText=str(self.Ei), LogType="Number Series")
        #mantid.simpleapi.AddSampleLog(__w,LogName="s2",LogText=str(self.S2), LogType="Number Series")
        #mantid.simpleapi.LoadInstrument(__w,InstrumentName=str(self.instrument))

if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    mainForm=InstrumentSetupWidget()
    mainForm.show()
    sys.exit(app.exec_())
