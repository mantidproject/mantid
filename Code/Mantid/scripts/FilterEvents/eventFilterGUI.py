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
import mantid.kernel
from mantid.simpleapi import AnalysisDataService 

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

class MainWindow(QtGui.QMainWindow): 
    """ Class of Main Window (top)

      Move to ui.setupUI
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

        # Do initialize plotting
        vecx, vecy, xlim, ylim = self.computeMock()

        self.mainline = self.ui.theplot.plot(vecx, vecy, 'r-')

        leftx = [xlim[0], xlim[0]]
        lefty = [ylim[0], ylim[1]]
        self.leftslideline = self.ui.theplot.plot(leftx, lefty, 'b--')
        rightx = [xlim[1], xlim[1]]
        righty = [ylim[0], ylim[1]]
        self.rightslideline = self.ui.theplot.plot(rightx, righty, 'g--')
        upperx = [xlim[0], xlim[1]]
        uppery = [ylim[1], ylim[1]]
        self.upperslideline = self.ui.theplot.plot(upperx, uppery, 'b--')
        lowerx = [xlim[0], xlim[1]]
        lowery = [ylim[0], ylim[0]]
        self.lowerslideline = self.ui.theplot.plot(lowerx, lowery, 'g--')

        self.ui.graphicsView.mpl_connect('button_press_event', self.on_mouseDownEvent)

        # Set up horizontal slide (integer) and string value
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
        
        # self.connect(self.ui.lineEdit_3, QtCore.SIGNAL("textChanged(QString)"), 
        #         self.set_startTime)    
        self.ui.lineEdit_3.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_3))
        self.connect(self.ui.pushButton_setT0, QtCore.SIGNAL("clicked()"), self.set_startTime)    
        # self.connect(self.ui.lineEdit_4, QtCore.SIGNAL("textChanged(QString)"), 
        #         self.set_stopTime)
        self.ui.lineEdit_4.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_4))
        self.connect(self.ui.pushButton_setTf, QtCore.SIGNAL("clicked()"), self.set_stopTime)


        # File loader
        self.scanEventWorkspaces()
        self.connect(self.ui.pushButton_refreshWS, SIGNAL('clicked()'), self.scanEventWorkspaces)
        self.connect(self.ui.pushButton_2, SIGNAL('clicked()'), self.browse_openFile)
        self.connect(self.ui.pushButton_3, SIGNAL('clicked()'), self.use_existWS)

        # Set up time
        self.ui.lineEdit_3.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_3))
        self.ui.lineEdit_4.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_4))

        # Filter by time
        self.connect(self.ui.pushButton_filterTime, SIGNAL('clicked()'), self.filterByTime)

        # Filter by log value
        self.ui.lineEdit_5.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_5))
        self.ui.lineEdit_6.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_6))
        self.ui.lineEdit_7.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_7))
        self.ui.lineEdit_8.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_8))
        self.ui.lineEdit_9.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_9))
        
        self.connect(self.ui.lineEdit_5, QtCore.SIGNAL("textChanged(QString)"), 
                self.set_minLogValue)    
        self.connect(self.ui.lineEdit_6, QtCore.SIGNAL("textChanged(QString)"), 
                self.set_maxLogValue)    

        dirchangeops = ["Both", "Increase", "Decrease"]
        self.ui.comboBox_4.addItems(dirchangeops)

        logboundops = ["Centre", "Left"]
        self.ui.comboBox_5.addItems(logboundops)

        self.connect(self.ui.pushButton_4, SIGNAL('clicked()'), self.plotLogValue)

        self.connect(self.ui.pushButton_filterLog, SIGNAL('clicked()'), self.filterByLogValue)

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

        x0 = 0.
        xf = 1.
        dx = 0.1

        vecx = []
        vecy = []
        
        x = x0
        while x < xf:
            y = 0.0
            vecx.append(x)
            vecy.append(y)
            x += dx

        xlim = [x0, xf]
        ylim = [-1., 1]

        return (vecx, vecy, xlim, ylim)


    def move_leftSlider(self):
        """ Re-setup left range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newx = self.ui.horizontalSlider.value()
        if newx <= self._rightSlideValue and newx != self._leftSlideValue:
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


    def set_startTime(self):
        """ Set the starting time and left slide bar
        """
        inps = str(self.ui.lineEdit_3.text())
        print "Starting time = %s" % (inps)
       
        xlim = self.ui.theplot.get_xlim()
        if inps == "":
            # Empty. Use default
            newtime0 = xlim[0]
        else: 
            newtime0 = float(inps)

        # Convert to integer slide value
        ileftvalue = int( (newtime0-xlim[0])/(xlim[1] - xlim[0])*100 )
        print "iLeftSlide = %d" % (ileftvalue)

        # Skip if same as origina
        if ileftvalue == self._leftSlideValue:
            return

        # Set the value if out of range
        resetT = True
        if ileftvalue < 0:
            # Minimum value as 0
            ileftvalue = 0
        elif ileftvalue > self._rightSlideValue:
            # Maximum value as right slide value
            ileftvalue = self._rightSlideValue
        else:
            resetT = False

        if resetT is True:
            newtime0 = xlim[0] + ileftvalue*(xlim[1]-xlim[0])*0.01
        print "Corrected iLeftSlide = %d (vs. right = %d)" % (ileftvalue, self._rightSlideValue)

        # Move the slide bar (left)
        self._leftSlideValue = ileftvalue

        # Move the vertical line
        leftx = [newtime0, newtime0]
        lefty = self.ui.theplot.get_ylim()
        setp(self.leftslideline, xdata=leftx, ydata=lefty)

        self.ui.graphicsView.draw()

        # Set the value to left slider 
        self.ui.horizontalSlider.setValue(self._leftSlideValue)
        # Reset the value of line edit 
        if resetT is True: 
            self.ui.lineEdit_3.setText(str(newtime0))

        return

    def move_rightSlider(self):
        """ Re-setup left range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newx = self.ui.horizontalSlider_2.value()
        if newx >= self._leftSlideValue and newx != self._rightSlideValue:
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


    def set_stopTime(self):
        """ Set the starting time and left slide bar
        """
        inps = str(self.ui.lineEdit_4.text())
        print "Stopping time = %s" % (inps)
        
        xlim = self.ui.theplot.get_xlim()
        if inps == "":
            # Empty. Use default
            newtimef = xlim[1]
        else:
            # Parse 
            newtimef = float(inps)

        # Convert to integer slide value
        irightvalue = int( (newtimef-xlim[0])/(xlim[1] - xlim[0])*100 )
        print "iRightSlide = %d" % (irightvalue)

        # Return if no change
        if irightvalue == self._rightSlideValue:
            return

        # Correct value
        resetT = True
        if irightvalue >= 100:
            irightvalue == 100
        elif irightvalue < self._leftSlideValue:
            irightvalue = self._leftSlideValue
        else:
            resetT = False

        if resetT is True:
            newtimef = xlim[0] + irightvalue*(xlim[1]-xlim[0])*0.01

        # Move the slide bar (right)
        self._rightSlideValue = irightvalue

        # Move the vertical line
        rightx = [newtimef, newtimef]
        righty = self.ui.theplot.get_ylim()
        setp(self.rightslideline, xdata=rightx, ydata=righty)

        self.ui.graphicsView.draw()

        # Set the value to left slider 
        self.ui.horizontalSlider_2.setValue(self._rightSlideValue)

        # Reset to line edit
        if resetT:
            self.ui.lineEdit_4.setText(str(newtimef))

        return

    def move_lowerSlider(self):
        """ Re-setup upper range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newy = self.ui.verticalSlider_2.value()
        if newy <= self._upperSlideValue and newy != self._lowerSlideValue:
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
            self.ui.verticalSlider_2.setValue(self._lowerSlideValue)

        return

    def set_minLogValue(self):
        """ Set the starting time and left slide bar
        """
        print "Minimum Log Value = %s" %(str(self.ui.lineEdit_5.text()))

        ylim = self.ui.theplot.get_ylim()
      
        if str(self.ui.lineEdit_5.text()) == "":
            # Empty. Default to minY
            newminY = ylim[0]
        else: 
            # Non empty.  Parse
            newminY = float(self.ui.lineEdit_5.text())

        # Convert to integer slide value
        iminlogval = int( (newminY-ylim[0])/(ylim[1] - ylim[0])*100 )
        print "ilowerSlide = %d" % (iminlogval)

        # Return if no change
        if iminlogval == self._lowerSlideValue:
            return

        # Set value if out of range
        resetL = True
        if iminlogval < 0:
            iminlogval = 0
        elif iminlogval >= self._upperSlideValue:
            iminlogval = self._upperSlideValue - 1
        else:
            resetL = False

        if resetL is True:
            newminY = ylim[0] + iminlogval * (ylim[1]-ylim[0]) * 0.01

        # Move the vertical line
        lowerx =  self.ui.theplot.get_xlim()
        lowery =  [newminY, newminY]        
        setp(self.lowerslideline, xdata=lowerx, ydata=lowery)

        self.ui.graphicsView.draw()

        # Move the slide bar (lower)
        self._lowerSlideValue = iminlogval
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)

        # Reset line Edit if using default
        if resetL is True:
            self.ui.lineEdit_5.setText(str(newminY))

        return

    def move_upperSlider(self):
        """ Re-setup upper range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        newy = self.ui.verticalSlider.value()
        if newy >= self._lowerSlideValue and newy != self._upperSlideValue:
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

    def set_maxLogValue(self):
        """ Set maximum log value from line-edit
        """
        inps = str(self.ui.lineEdit_6.text())
        print "Maximum Log Value = %s" %(inps)

        ylim = self.ui.theplot.get_ylim()
        if inps == "":
            # Empty. Default to minY
            newmaxY = ylim[1]
        else:
            # Parse
            newmaxY = float(inps)

        # Convert to integer slide value
        imaxlogval = int( (newmaxY-ylim[0])/(ylim[1] - ylim[0])*100 )
        print "iUpperSlide = %d" % (imaxlogval)

        # Return if no change
        if imaxlogval == self._upperSlideValue:
            return

        # Set to default if out of range
        resetL = True
        if imaxlogval >= 100:
            imaxlogval = 100
        elif imaxlogval < self._lowerSlideValue:
            imaxlogval = self._lowerSlideValue + 1
        else:
            resetL = False

        # Set newmaxY if necessary
        if resetL is True:
            newmaxY = ylim[0] + imaxlogval * (ylim[1] - ylim[0]) * 0.01

        # Move the vertical line
        upperx =  self.ui.theplot.get_xlim()
        uppery =  [newmaxY, newmaxY]        
        setp(self.upperslideline, xdata=upperx, ydata=uppery)

        self.ui.graphicsView.draw()

        # Set the value to upper slider
        self._upperSlideValue = imaxlogval
        self.ui.verticalSlider.setValue(self._upperSlideValue)

        # Set the value to editor if necessary
        if resetL is True:
            self.ui.lineEdit_6.setText(str(newmaxY))

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


    def use_existWS(self):
        """ Set up workspace to an existing one
        """
        wsname = str(self.ui.comboBox.currentText())

        try: 
            dataws = AnalysisDataService.retrieve(wsname)
            self._importDataWorkspace(dataws)
        except KeyError: 
            pass

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

        # Set to plot
        xlim = [min(vecreltimes), max(vecreltimes)]
        ylim = [min(vecvalue), max(vecvalue)]
        self.ui.theplot.set_xlim(xlim[0], xlim[1])
        self.ui.theplot.set_ylim(ylim[0], ylim[1])

        setp(self.mainline, xdata=vecreltimes, ydata=vecvalue) 

        # assume that all logs are on almost same X-range.  Only Y need to be reset
        setp(self.leftslideline, ydata=ylim)
        setp(self.rightslideline, ydata=ylim)

        # reset the log value limit as previous one does not make any sense
        setp(self.lowerslideline, xdata=xlim, ydata=[ylim[0], ylim[0]])
        self._lowerSlideValue = 0
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)
        self.ui.lineEdit_5.setText("")

        setp(self.upperslideline, xdata=xlim, ydata=[ylim[1], ylim[1]]) 
        self._upperSlideValue = 100
        self.ui.verticalSlider.setValue(self._upperSlideValue)
        self.ui.lineEdit_6.setText("")

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
                times = p.times
                if len(times) > 1: 
                    self._sampleLogNames.append(p.name)
        # ENDFOR(p)

        # Set up sample log 
        self.ui.comboBox_2.addItems(self._sampleLogNames)

        return

    def scanEventWorkspaces(self):
        """ 
        """
        wsnames = AnalysisDataService.getObjectNames()

        eventwsnames = []
        for wsname in wsnames:
            wksp = AnalysisDataService.retrieve(wsname)
            if wksp.__class__.__name__.count("Event") == 1:
                eventwsnames.append(wsname)
        # ENDFOR

        if len(eventwsnames) > 0: 
            self.ui.comboBox.addItems(eventwsnames)

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
        # Rebin events by pulse time
        timeres = 1.
        sumws = api.SumSpectra(InputWorkspace=wksp, OutputWorkspace="Summed")
        sumws = api.RebinByPulseTimes(InputWorkspace=sumws, Params="%f"%(timeres), OutputWorkspace=str(sumws))
        sumws = api.ConvertToPointData(InputWorkspace=sumws, OutputWorkspace=str(sumws))

        vecx = sumws.readX(0)
        vecy = sumws.readY(0)

        xmin = min(vecx)
        xmax = max(vecx)
        ymin = min(vecy)
        ymax = max(vecy)

        # Reset graph
        self.ui.theplot.set_xlim(xmin, xmax)
        self.ui.theplot.set_ylim(ymin, ymax)

        # Set up main line
        setp(self.mainline, xdata=vecx, ydata=vecy) 

        # Reset slide
        newslidery = [min(vecy), max(vecy)]

        newleftx = xmin + (xmax-xmin)*self._leftSlideValue*0.01
        setp(self.leftslideline, xdata=[newleftx, newleftx], ydata=newslidery) 

        newrightx = xmin + (xmax-xmin)*self._rightSlideValue*0.01
        setp(self.rightslideline, xdata=[newrightx, newrightx], ydata=newslidery) 

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

        title = str(self.ui.lineEdit_title.text())

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

    def filterByLogValue(self):
        """ Filter by log value
        """
        # Generate event filter
        kwargs = {}
        samplelog = str(self.ui.comboBox_2.currentText())
        if len(samplelog) == 0:
            print "No sample log is selected!"
            return


        if self.ui.lineEdit_3.text() != "":
            rel_starttime = float(self.ui.lineEdit_3.text())
            kwargs["StartTime"] = str(rel_starttime)
        if self.ui.lineEdit_4.text() != "":
            rel_stoptime = float(self.ui.lineEdit_4.text())
            kwargs["StopTime"] = str(rel_stoptime)
        if self.ui.lineEdit_5.text() != "":
            minlogvalue = float(self.ui.lineEdit_5.text())
            kwargs["MinimumLogValue"] = minlogvalue
        if self.ui.lineEdit_6.text() != "":
            maxlogvalue = float(self.ui.lineEdit_6.text())
            kwargs["MaximumLogValue"] = maxlogvalue
        if self.ui.lineEdit_7.text() != "":
            logvalueintv = float(self.ui.lineEdit_7.text())
            kwargs["LogValueInterval"] = logvalueintv
        logvalchangedir = str(self.ui.comboBox_4.currentText())
        kwargs["FilterLogValueByChangingDirection"] = logvalchangedir
        if self.ui.lineEdit_9.text() != "":
            logvalueintv = float(self.ui.lineEdit_9.text())
            kwargs["TimeTolerance"] = logvalueintv
        logboundtype = str(self.ui.comboBox_5.currentText())
        kwargs["LogBoundary"] = logboundtype
        if self.ui.lineEdit_8.text() != "":
            logvaluetol = float(self.ui.lineEdit_8.text())
            kwargs["LogValueTolerance"] = logvaluetol
    

        splitwsname = str(self._dataWS) + "_splitters"
        splitinfowsname = str(self._dataWS) + "_info"

        title = str(self.ui.lineEdit_title.text())

        splitws, infows = api.GenerateEventsFilter(
                InputWorkspace      = self._dataWS,
                UnitOfTime          = "Seconds",
                TitleOfSplitters    = title,
                OutputWorkspace     = splitwsname,
                LogName             = samplelog,
                InformationWorkspace = splitinfowsname, **kwargs)


    def splitWksp(self, splitws, infows):
        """ Run FilterEvents
        """
        print "Splitting workpsace by splitters %s with information %s" % (str(splitws), str(infows))

        return
