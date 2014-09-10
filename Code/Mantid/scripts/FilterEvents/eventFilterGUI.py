import math
import numpy

from Ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.pyplot import gcf, setp

import mantid.simpleapi as api

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

class MainWindow(QtGui.QMainWindow): 
    """ Class of Main Window (top)
    """ 
    
    def __init__(self, parent=None):
        """ Intialization and set up 
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # Central widget 
        self.centralwidget = QtGui.QWidget(self)

        # UI Window (from Qt Designer) 
        self.ui = Ui_MainWindow() 
        self.ui.setupUi(self)


        """ Move to ui.setupUI
        Replacement is not a valid approach as the UI is setup at the end of self.ui.setupUI
        self.dpi = 100
        self.fig = Figure((5.0, 4.0), dpi=self.dpi)
        self.figure = Figure((4.0, 3.0), dpi=100)
        self.theplot = self.figure.add_subplot(111)
        self.ui.graphicsView = FigureCanvas(self.figure)
        self.ui.graphicsView.setParent(self.centralwidget)
        self.ui.graphicsView.setGeometry(QtCore.QRect(40, 230, 821, 411))
        self.ui.graphicsView.setObjectName(_fromUtf8("graphicsView"))

        
        # Version 2.0 + Import
        self.figure = Figure((4.0, 3.0), dpi=100)
        self.theplot = self.figure.add_subplot(111)
        self.graphicsView = FigureCanvas(self.figure)
        self.graphicsView.setParent(self.centralwidget)
        self.graphicsView.setGeometry(QtCore.QRect(40, 230, 721, 411))
        self.graphicsView.setObjectName(_fromUtf8("graphicsView"))

        """

        # Do initialize plotting
        vecx, vecy = self.computeMock()
        leftx = [11, 11]
        lefty = [-10, 0]
        rightx = [19, 19]
        righty = [-10, 0]
        upperx = [0, 20]
        uppery = [0, 0]
        lowerx = [0, 20]
        lowery = [-10, -10]

        self.mainline = self.ui.theplot.plot(vecx, vecy, 'r-')
        self.leftslideline = self.ui.theplot.plot(leftx, lefty, 'b--')
        self.rightslideline = self.ui.theplot.plot(rightx, righty, 'g--')
        self.upperslideline = self.ui.theplot.plot(upperx, uppery, 'b--')
        self.lowerslideline = self.ui.theplot.plot(lowerx, lowery, 'g--')
        #setp(self.ui.mainline, xdata=vecx, ydata=vecy)

        self.ui.graphicsView.mpl_connect('button_press_event', self.on_mouseDownEvent)

        # Set up horizontal slide 
        self._leftSlideValue = 0
        self._rightSlideValue = 99

        self.ui.horizontalSlider.setRange(0, 100)
        self.ui.horizontalSlider.setValue(self._leftSlideValue)
        self.ui.horizontalSlider.setTracking(True)
        self.ui.horizontalSlider.setTickPosition(QSlider.TicksBothSides)
        self.connect(self.ui.horizontalSlider, SIGNAL('valueChanged(int)'), self.move_leftSlider)


        self.ui.horizontalSlider_2.setRange(0, 100)
        self.ui.horizontalSlider_2.setValue(self._rightSlideValue)
        self.ui.horizontalSlider_2.setTracking(True)
        self.ui.horizontalSlider_2.setTickPosition(QSlider.TicksBothSides)
        self.connect(self.ui.horizontalSlider_2, SIGNAL('valueChanged(int)'), self.move_rightSlider)

        # File loader
        self.connect(self.ui.pushButton_2, SIGNAL('clicked()'), self.browse_openFile)

        # Set up time
        self.ui.lineEdit_3.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_3))
        self.ui.lineEdit_4.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_4))

        # Filter by time
        self.connect(self.ui.pushButton_5, SIGNAL('clicked()'), self.filterByTime)

        # Filter by log value
        self.connect(self.ui.pushButton_4, SIGNAL('clicked()'), self.plotLogValue)

        # Set up vertical slide
        self._upperSlideValue = 99
        self._lowerSlideValue = 0

        self.ui.verticalSlider.setRange(0, 100)
        self.ui.verticalSlider.setValue(self._upperSlideValue)
        self.ui.verticalSlider.setTracking(True)
        self.connect(self.ui.verticalSlider, SIGNAL('valueChanged(int)'), self.move_upperSlider)

        self.ui.verticalSlider_2.setRange(0, 100)
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)
        self.ui.verticalSlider_2.setTracking(True)
        self.connect(self.ui.verticalSlider_2, SIGNAL('valueChanged(int)'), self.move_lowerSlider)


        # Set up for workspaces
        self._dataWS = None
        self._sampleLogNames = []
        self._sampleLog = None



        # self.ui.InputVal.setValidator(QtGui.QDoubleValidator(self.ui.InputVal))
        
        # QtCore.QObject.connect(self.ui.convert, QtCore.SIGNAL("clicked()"), self.convert )
        # QtCore.QObject.connect(self.ui.inputUnits, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setInstrumentInputs )
        # QtCore.QObject.connect(self.ui.outputUnits, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setInstrumentInputs )
        # self.setInstrumentInputs() 
        ##defaults
        self.flightpath=-1.0
        self.Theta=-1.0
        self.stage1output=0.0
        self.stage2output=0.0

        return


    def on_mouseDownEvent(self, event):
        """ Respond to pick up a value with mouse down event
        """
        x = event.xdata
        y = event.ydata

        if x is not None and y is not None: 
            msg = "You've clicked on a bar with coords:\n %f, %f" % (x, y)
            QMessageBox.information(self, "Click!", msg)

        return


    def computeMock(self):
        """ Compute vecx and vecy as mocking
        """
        import random, math

        x0 = 10.
        xf = 20.
        dx = 0.05

        vecx = []
        vecy = []
        
        x = x0
        while x < xf:
            y = math.sin(x/10) * (random.random()-1)*10.
            vecx.append(x)
            vecy.append(y)
            x += dx

        return (vecx, vecy)


    def move_leftSlider(self):
        """ Re-setup left range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newx = self.ui.horizontalSlider.value()
        if newx <= self._rightSlideValue:
            # Allowed value: move the value bar
            self._leftSlideValue = newx

            # Move the vertical line
            xlim = self.ui.theplot.get_xlim()
            newx = xlim[0] + newx*(xlim[1] - xlim[0])*0.01
            leftx = [newx, newx]
            lefty = self.ui.theplot.get_ylim()
            setp(self.leftslideline, xdata=leftx, ydata=lefty)

            self.ui.graphicsView.draw()

            # Change value
            self.ui.lineEdit_3.setText(str(newx))

        else:
            # Reset the value to original value
            self.ui.horizontalSlider.setValue(self._leftSlideValue)

        return

    def move_rightSlider(self):
        """ Re-setup left range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newx = self.ui.horizontalSlider_2.value()
        if newx >= self._leftSlideValue:
            # Allowed value: move the value bar
            self._rightSlideValue = newx

            xlim = self.ui.theplot.get_xlim()
            newx = xlim[0] + newx*(xlim[1] - xlim[0])*0.01
            leftx = [newx, newx]
            lefty = self.ui.theplot.get_ylim()
            setp(self.rightslideline, xdata=leftx, ydata=lefty)

            self.ui.graphicsView.draw()

            # Change value
            self.ui.lineEdit_4.setText(str(newx))

        else:
            # Reset the value
            self.ui.horizontalSlider_2.setValue(self._rightSlideValue)

        return


    def move_lowerSlider(self):
        """ Re-setup upper range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newy = self.ui.verticalSlider_2.value()
        if newy <= self._upperSlideValue:
            # Allowed value: move the value bar
            self._lowerSlideValue = newy

            # Move the vertical line
            ylim = self.ui.theplot.get_ylim()
            newy = ylim[0] + newy*(ylim[1] - ylim[0])*0.01
            lowerx = self.ui.theplot.get_xlim() 
            lowery = [newy, newy]
            setp(self.lowerslideline, xdata=lowerx, ydata=lowery)

            self.ui.graphicsView.draw()

            # Change value
            self.ui.lineEdit_5.setText(str(newy))

        else:
            # Reset the value to original value
            self.ui.verticalSlider.setValue(self._lowerSlideValue)

        return


    def move_upperSlider(self):
        """ Re-setup upper range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newy = self.ui.verticalSlider.value()
        if newy >= self._lowerSlideValue:
            # Allowed value: move the value bar
            self._upperSlideValue = newy

            # Move the vertical line
            ylim = self.ui.theplot.get_ylim()
            newy = ylim[0] + newy*(ylim[1] - ylim[0])*0.01
            upperx = self.ui.theplot.get_xlim() 
            uppery = [newy, newy]
            setp(self.upperslideline, xdata=upperx, ydata=uppery)

            self.ui.graphicsView.draw()

            # Change value
            self.ui.lineEdit_6.setText(str(newy))

        else:
            # Reset the value to original value
            self.ui.verticalSlider.setValue(self._upperSlideValue)

        return


    def browse_openFile(self):
        """ Open a file dialog to get file
        """
        defaultdir = "/home/wzz/Projects/MantidTests/Vulcan/Reduction/CompleteTest/Data/"

        filename = QtGui.QFileDialog.getOpenFileName(self, 'Input File Dialog', 
            defaultdir, "Data (*.nxs *.dat);;All files (*.*)")

        self.ui.lineEdit.setText(str(filename))

        print "Selected file: ", filename

        dataws = self._loadFile(str(filename))
        self._importDataWorkspace(dataws)

        return


    def plotLogValue(self):
        """ Plot log value
        """
        # Get log value
        logname = str(self.ui.comboBox_2.currentText())

        samplelog = self._dataWS.getRun().getProperty(logname)
        vectimes = samplelog.times
        vecvalue = samplelog.value

        # check
        if len(vectimes) == 0: 
            print "Empty log!"

        # Convert absolute time to relative time in seconds
        t0 = self._dataWS.getRun().getProperty("proton_charge").times[0]
        t0ns = t0.totalNanoseconds()

        # append 1 more log if original log only has 1 value
        tf = self._dataWS.getRun().getProperty("proton_charge").times[-1]
        vectimes.append(tf)
        vecvalue = numpy.append(vecvalue, vecvalue[-1])

        vecreltimes = []
        for t in vectimes:
            rt = float(t.totalNanoseconds() - t0ns) * 1.0E-9
            vecreltimes.append(rt)


        self.ui.theplot.set_xlim(min(vecreltimes), max(vecreltimes))
        self.ui.theplot.set_ylim(min(vecvalue), max(vecvalue))
        setp(self.mainline, xdata=vecreltimes, ydata=vecvalue) 
        # setp(self.leftslideline, ydata=newslidery) 
        # setp(self.rightslideline, ydata=newslidery) 

        self.ui.graphicsView.draw()

        return


    def _importDataWorkspace(self, dataws):
        """ Import data workspace for filtering
        """
        if dataws is None: 
            return

        self._dataWS = dataws 

        # Plot time counts
        self._plotTimeCounts(self._dataWS)
        
        # Import log
        run = self._dataWS.getRun()
        plist = run.getProperties()
        for p in plist:
            pv = p.value
            if isinstance(pv, numpy.ndarray):
                self._sampleLogNames.append(p.name)
        # ENDFOR(p)

        # Set up sample log 
        self.ui.comboBox_2.addItems(self._sampleLogNames)

        return


    def _loadFile(self, filename):
        """ Load file
        File will be loaded to a workspace shown in MantidPlot
        """
        import os
       
        wsname = os.path.splitext(os.path.split(filename)[1])[0]

        try: 
            ws = api.Load(Filename=filename, OutputWorkspace=wsname)
        except:
            ws = None

        return ws

    
    def _plotTimeCounts(self, wksp):
        """ Plot time/counts 
        """
        timeres = 1.
        sumws = api.SumSpectra(InputWorkspace=wksp, OutputWorkspace="Summed")
        sumws = api.RebinByPulseTimes(InputWorkspace=sumws, Params="%f"%(timeres), OutputWorkspace=str(sumws))
        sumws = api.ConvertToPointData(InputWorkspace=sumws, OutputWorkspace=str(sumws))


        vecx = sumws.readX(0)
        vecy = sumws.readY(0)

        newslidery = [min(vecy), max(vecy)]

        setp(self.mainline, xdata=vecx, ydata=vecy) 
        setp(self.leftslideline, ydata=newslidery) 
        setp(self.rightslideline, ydata=newslidery) 
        self.ui.theplot.set_xlim(min(vecx), max(vecx))
        self.ui.theplot.set_ylim(min(vecy), max(vecy))

        self.ui.graphicsView.draw()

        return


    def filterByTime(self):
        """ Filter by time
        """
        # Generate event filters
        kwargs = {}
        if self.ui.lineEdit_3.text() != "":
            rel_starttime = float(self.ui.lineEdit_3.text())
            kwargs["StartTime"] = str(rel_starttime)
        if self.ui.lineEdit_4.text() != "":
            rel_stoptime = float(self.ui.lineEdit_4.text())
            kwargs["StopTime"] = str(rel_stoptime)
        if self.ui.lineEdit_timeInterval.text() != "":
            interval = float(self.ui.lineEdit_timeInterval.text())
            kwargs["TimeInterval"] = interval

        splitwsname = str(self._dataWS) + "_splitters"
        splitinfowsname = str(self._dataWS) + "_info"

        title = str(self.ui.lineEdit_timeInterval_2.text())

        """ Debug 
        for k in kwargs.keys():
            print k, kwargs[k], type(kwargs[k])
        print "Input workspace = ", str(self._dataWS)
         END DB """

        splitws, infows = api.GenerateEventsFilter(
                InputWorkspace      = self._dataWS,
                UnitOfTime          = "Seconds",
                TitleOfSplitters    = title,
                OutputWorkspace     = splitwsname,
                InformationWorkspace = splitinfowsname, **kwargs)

        self.splitWksp(splitws, infows)

        return


    def splitWksp(self, splitws, infows):
        """ Run FilterEvents
        """
        print "Splitting workpsace by splitters %s with information %s" % (str(splitws), str(infows))

        return
