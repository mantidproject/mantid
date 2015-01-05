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

import mantid
import mantid.simpleapi as api
import mantid.kernel
from mantid.simpleapi import AnalysisDataService 

from mantid.kernel import ConfigService

import os

HUGE_FAST = 10000
HUGE_PARALLEL = 100000
MAXTIMEBINSIZE = 20000

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

class MyPopErrorMsg(QWidget):
    """ Pop up dialog window
    """
    def __init__(self):
        """ Init
        """
        import Ui_ErrorMessage as errui
        QWidget.__init__(self)


        self.ui = errui.Ui_Dialog()
        self.ui.setupUi(self)

        QtCore.QObject.connect(self.ui.pushButton_quit, QtCore.SIGNAL('clicked()'), self.quit)

    def setMessage(self, errmsg):
        """ Set message
        """
        self.ui.label_errmsg.setWordWrap(True)
        self.ui.label_errmsg.setText(errmsg)

        return


    def quit(self):
        """ Quit
        """
        self.close()

        return

    def XpaintEvent(self, e):
        """ ???
        """
        import Ui_ErrorMessage as errui

        self.ui = errui.Ui_Dialog()
        self.ui.setupUi(self)

        return

class MainWindow(QtGui.QMainWindow): 
    """ Class of Main Window (top)

    Move to ui.setupUI

    # Version 3.0 + Import for Ui_MainWindow.py
        from MplFigureCanvas import Qt4MplCanvas 

        # Replace 'self.graphicsView = QtGui.QtGraphicsView' with the following
        self.graphicsView = Qt4MplCanvas(self.centralwidget)
        self.mainplot = self.graphicsView.getPlot()

        
    # Version 2.0 + Import
        import matplotlib
        from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
        from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
        from matplotlib.figure import Figure

        self.figure = Figure((4.0, 3.0), dpi=100)
        self.mainplot = self.figure.add_subplot(111)
        self.graphicsView = FigureCanvas(self.figure)
        self.graphicsView.setParent(self.centralwidget)
        self.graphicsView.setGeometry(QtCore.QRect(20, 150, 741, 411))
        self.graphicsView.setObjectName(_fromUtf8("graphicsView"))

    # Version 1.0
        Replacement is not a valid approach as the UI is setup at the end of self.ui.setupUI
        self.dpi = 100
        self.fig = Figure((5.0, 4.0), dpi=self.dpi)
        self.figure = Figure((4.0, 3.0), dpi=100)
        self.mainplot = self.figure.add_subplot(111)
        self.ui.graphicsView = FigureCanvas(self.figure)
        self.ui.graphicsView.setParent(self.centralwidget)
        self.ui.graphicsView.setGeometry(QtCore.QRect(40, 230, 821, 411))
        self.ui.graphicsView.setObjectName(_fromUtf8("graphicsView"))

    """ 
    
    def __init__(self, parent=None):
        """ Intialization and set up 
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # Mantid configuration
        config = ConfigService.Instance()
        self._instrument = config["default.instrument"]

        # Central widget 
        self.centralwidget = QtGui.QWidget(self)

        # UI Window (from Qt Designer) 
        self.ui = Ui_MainWindow() 
        self.ui.setupUi(self)

        # Do initialize plotting
        vecx, vecy, xlim, ylim = self.computeMock()

        self.mainline = self.ui.mainplot.plot(vecx, vecy, 'r-')

        leftx = [xlim[0], xlim[0]]
        lefty = [ylim[0], ylim[1]]
        self.leftslideline = self.ui.mainplot.plot(leftx, lefty, 'b--')
        rightx = [xlim[1], xlim[1]]
        righty = [ylim[0], ylim[1]]
        self.rightslideline = self.ui.mainplot.plot(rightx, righty, 'g--')
        upperx = [xlim[0], xlim[1]]
        uppery = [ylim[1], ylim[1]]
        self.upperslideline = self.ui.mainplot.plot(upperx, uppery, 'b--')
        lowerx = [xlim[0], xlim[1]]
        lowery = [ylim[0], ylim[0]]
        self.lowerslideline = self.ui.mainplot.plot(lowerx, lowery, 'g--')

        self.ui.graphicsView.mpl_connect('button_press_event', self.on_mouseDownEvent)

        # Set up horizontal slide (integer) and string value
        self._leftSlideValue = 0
        self._rightSlideValue = 99

        self.ui.horizontalSlider.setRange(0, 100)
        self.ui.horizontalSlider.setValue(self._leftSlideValue)
        self.ui.horizontalSlider.setTracking(True)
        self.ui.horizontalSlider.setTickPosition(QSlider.NoTicks)
        self.connect(self.ui.horizontalSlider, SIGNAL('valueChanged(int)'), self.move_leftSlider)

        self.ui.horizontalSlider_2.setRange(0, 100)
        self.ui.horizontalSlider_2.setValue(self._rightSlideValue)
        self.ui.horizontalSlider_2.setTracking(True)
        self.ui.horizontalSlider_2.setTickPosition(QSlider.NoTicks)
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
        self.connect(self.ui.pushButton_browse, SIGNAL('clicked()'), self.browse_File)
        self.connect(self.ui.pushButton_load, SIGNAL('clicked()'), self.load_File)
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

        # Set up for filtering (advanced setup)
        self._tofcorrection = False
        self.ui.checkBox_filterByPulse.setChecked(False)
        self.ui.checkBox_from1.setChecked(False)
        self.ui.checkBox_groupWS.setChecked(True)

        self.connect(self.ui.comboBox_tofCorr, SIGNAL('currentIndexChanged(int)'), self.showHideEi) 
        self.connect(self.ui.pushButton_refreshCorrWSList, SIGNAL('clicked()'),  self._searchTableWorkspaces)

        self.ui.lineEdit_Ei.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_Ei))

        self.ui.label_Ei.hide()
        self.ui.lineEdit_Ei.hide()
        self.ui.label_Ei_2.hide()
        self.ui.comboBox_corrWS.hide()
        self.ui.pushButton_refreshCorrWSList.hide()

        # Error message
        # self.connect(self.ui.pushButton_clearerror, SIGNAL('clicked()'), self._clearErrorMsg)
        # self.ui.plainTextEdit_ErrorMsg.setReadOnly(True)
        # self.ui.label_error.hide()

        # Set up for workspaces
        self._dataWS = None
        self._sampleLogNames = []
        self._sampleLog = None

        # Side information
        self.ui.label_mean.hide()
        self.ui.label_meanvalue.hide()
        self.ui.label_avg.hide() 
        self.ui.label_timeAvgValue.hide() 
        self.ui.label_freq.hide()
        self.ui.label_freqValue.hide()
        self.ui.label_logname.hide()
        self.ui.label_lognamevalue.hide()
        self.ui.label_logsize.hide()
        self.ui.label_logsizevalue.hide()

        # Default 
        self._defaultdir = os.getcwd()

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
            xlim = self.ui.mainplot.get_xlim()
            newx = xlim[0] + newx*(xlim[1] - xlim[0])*0.01
            leftx = [newx, newx]
            lefty = self.ui.mainplot.get_ylim()
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
       
        xlim = self.ui.mainplot.get_xlim()
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
        lefty = self.ui.mainplot.get_ylim()
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

            xlim = self.ui.mainplot.get_xlim()
            newx = xlim[0] + newx*(xlim[1] - xlim[0])*0.01
            leftx = [newx, newx]
            lefty = self.ui.mainplot.get_ylim()
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
        
        xlim = self.ui.mainplot.get_xlim()
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
        righty = self.ui.mainplot.get_ylim()
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
        inewy = self.ui.verticalSlider_2.value()
        print "LowerSlider is set with value %d  vs. class variable %d" % (inewy, self._lowerSlideValue)

        # Return with no change
        if inewy == self._lowerSlideValue:
            # No change
            return

        if inewy >= self._upperSlideValue:
            # Out of upper range
            inewy = self._upperSlideValue - 1

        if inewy == 0 and self._lowerSlideValue < 0:
            setLineEdit = False
        else:
            setLineEdit = True

        # Move the lower vertical bar
        ylim = self.ui.mainplot.get_ylim()
        newy = ylim[0] + inewy*(ylim[1] - ylim[0])*0.01
        lowerx = self.ui.mainplot.get_xlim()
        lowery = [newy, newy]
        setp(self.lowerslideline, xdata=lowerx, ydata=lowery)

        self.ui.graphicsView.draw()

        # Set line edit input
        if setLineEdit is True: 
            # Change value to line edit (5)
            self.ui.lineEdit_5.setText(str(newy))
            # Reset the class variable
            self._lowerSlideValue = inewy

        return

    def set_minLogValue(self):
        """ Set the starting time and left slide bar
        """
        print "Minimum Log Value = %s" %(str(self.ui.lineEdit_5.text()))

        ylim = self.ui.mainplot.get_ylim()
      
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
        if iminlogval >= self._upperSlideValue:
            iminlogval = self._upperSlideValue - 1
        else:
            resetL = False

        if resetL is True:
            newminY = ylim[0] + iminlogval * (ylim[1]-ylim[0]) * 0.01

        # Move the vertical line
        lowerx =  self.ui.mainplot.get_xlim()
        lowery =  [newminY, newminY]        
        setp(self.lowerslideline, xdata=lowerx, ydata=lowery)

        self.ui.graphicsView.draw()

        # Move the slide bar (lower)
        self._lowerSlideValue = iminlogval
        print "LineEdit5 set slide to %d" % (self._lowerSlideValue)
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)

        # Reset line Edit if using default
        if resetL is True:
            self.ui.lineEdit_5.setText(str(newminY))

        return

    def move_upperSlider(self):
        """ Re-setup upper range line in figure. 
        Triggered by a change in Qt Widget.  NO EVENT is required. 
        """ 
        inewy = self.ui.verticalSlider.value()

        # Return w/o change
        if inewy == self._upperSlideValue:
            return

        # Set to boundary value
        if inewy <= self._lowerSlideValue:
            inewy = self._lowerSlideValue + 1

        # Reset line editor?
        if inewy == 100 and self._upperSlideValue > 100:
            setLineEdit = False
        else:
            setLineEdit = True

        # Move the upper value bar: upperx and uppery are real value (float but not (0,100)) of the figure
        ylim = self.ui.mainplot.get_ylim()
        newy = ylim[0] + inewy*(ylim[1] - ylim[0])*0.01
        upperx = self.ui.mainplot.get_xlim()
        uppery = [newy, newy]
        setp(self.upperslideline, xdata=upperx, ydata=uppery)

        self.ui.graphicsView.draw()

            # Change value
        if setLineEdit is True:
            self.ui.lineEdit_6.setText(str(newy))
            self._upperSlideValue = inewy

        return

    def set_maxLogValue(self):
        """ Set maximum log value from line-edit
        """
        inps = str(self.ui.lineEdit_6.text())
        print "Maximum Log Value = %s" %(inps)

        ylim = self.ui.mainplot.get_ylim()
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
        # if imaxlogval >= 100:
        #     imaxlogval = 100
        if imaxlogval < self._lowerSlideValue:
            imaxlogval = self._lowerSlideValue + 1
        else:
            resetL = False

        # Set newmaxY if necessary
        if resetL is True:
            newmaxY = ylim[0] + imaxlogval * (ylim[1] - ylim[0]) * 0.01

        # Move the vertical line
        upperx =  self.ui.mainplot.get_xlim()
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

    def browse_File(self):
        """ Open a file dialog to get file
        """
        filename = QtGui.QFileDialog.getOpenFileName(self, 'Input File Dialog', 
            self._defaultdir, "Data (*.nxs *.dat);;All files (*.*)")

        self.ui.lineEdit.setText(str(filename))

        # print "Selected file: ", filename

        return

    def load_File(self):
        """ Load the file by file name or run number
        """
        # Get file name from line editor
        filename = str(self.ui.lineEdit.text())

        # Find out it is relative path or absolute path
        if os.path.abspath(filename) == filename:
            isabspath = True
        else:
            isabspath = False

        dataws = self._loadFile(str(filename))
        if dataws is None:
            errmsg = "Unable to locate run %s in default directory %s." % (filename, self._defaultdir)
            print errmsg
            self._setErrorMsg(errmsg)
        else:
            self._importDataWorkspace(dataws)
            self._defaultdir = os.path.dirname(str(filename))

        # Reset GUI
        self._resetGUI(resetfilerun=False)

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

        # Reset GUI
        self._resetGUI(resetfilerun=True)

        return


    def plotLogValue(self):
        """ Plot log value
        """
        # Get log value
        logname = str(self.ui.comboBox_2.currentText())
        if len(logname) == 0:
            # return due to the empty one is chozen
            return

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
        self.ui.mainplot.set_xlim(xlim[0], xlim[1])
        self.ui.mainplot.set_ylim(ylim[0], ylim[1])

        setp(self.mainline, xdata=vecreltimes, ydata=vecvalue) 

        samunit = samplelog.units
        if len(samunit) == 0:
            ylabel = logname
        else:
            ylabel = "%s (%s)" % (logname, samunit)
        self.ui.mainplot.set_ylabel(ylabel, fontsize=13)

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

        # Load property's statistic and give suggestion on parallel and fast log
        timeavg = samplelog.timeAverageValue()
        numentries = samplelog.size()
        stat = samplelog.getStatistics()

        duration = stat.duration
        mean = stat.mean
        freq = float(numentries)/float(duration)

        self.ui.label_mean.show()
        self.ui.label_meanvalue.show()
        self.ui.label_avg.show() 
        self.ui.label_timeAvgValue.show() 
        self.ui.label_freq.show()
        self.ui.label_freqValue.show()
        self.ui.label_logname.show()
        self.ui.label_lognamevalue.show()
        self.ui.label_logsize.show()
        self.ui.label_logsizevalue.show()

        self.ui.label_meanvalue.setText("%.5e"%(mean))
        self.ui.label_timeAvgValue.setText("%.5e"%(timeavg))
        self.ui.label_freqValue.setText("%.5e"%(freq))
        self.ui.label_lognamevalue.setText(logname)
        self.ui.label_logsizevalue.setText(str(numentries))

        # Set suggested processing scheme
        if numentries > HUGE_FAST:
            self.ui.checkBox_fastLog.setCheckState(True)
            if numentries > HUGE_PARALLEL: 
                self.ui.checkBox_doParallel.setCheckState(True)
            else:
                self.ui.checkBox_doParallel.setCheckState(False)
        else:
            self.ui.checkBox_fastLog.setCheckState(False)
            self.ui.checkBox_doParallel.setCheckState(False)

        return


    def _importDataWorkspace(self, dataws):
        """ Import data workspace for filtering
        """
        if dataws is None: 
            return

        # Plot time counts
        errmsg = self._plotTimeCounts(dataws)
        if errmsg is not None:
            errmsg = "Workspace %s has invalid sample logs for splitting. Loading \
                    failure! \n%s\n" % (str(dataws), errmsg)
            self._setErrorMsg(errmsg)
            return False
        
        # Import log
        self._sampleLogNames = [""]

        run = dataws.getRun()
        plist = run.getProperties()
        for p in plist:
            pv = p.value
            if isinstance(pv, numpy.ndarray):
                times = p.times
                if len(times) > 1: 
                    self._sampleLogNames.append(p.name)
        # ENDFOR(p)

        # Set up sample log 
        self.ui.comboBox_2.clear()
        self.ui.comboBox_2.addItems(self._sampleLogNames)

        # Side information
        self.ui.label_mean.hide()
        self.ui.label_meanvalue.hide()
        self.ui.label_avg.hide() 
        self.ui.label_timeAvgValue.hide() 
        self.ui.label_freq.hide()
        self.ui.label_freqValue.hide()

        # Set dataws to class variable
        self._dataWS = dataws 

        return True

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
            self.ui.comboBox.clear()
            self.ui.comboBox.addItems(eventwsnames)

        return


    def _loadFile(self, filename):
        """ Load file or run
        File will be loaded to a workspace shown in MantidPlot
        """
        config = ConfigService

        # Check input file name and output workspace name
        if filename.isdigit() is True:
            # Construct a file name from run number
            runnumber = int(filename)
            if runnumber <= 0:
                print "Run number cannot be less or equal to zero.  User gives %s. " % (filename)
                return None
            else: 
                ishort = config.getInstrument(self._instrument).shortName()
                filename = "%s_%s" %(ishort, filename)
                wsname = filename + "_event"

        elif filename.count(".") > 0:
            # A proper file name
            wsname = os.path.splitext(os.path.split(filename)[1])[0]

        elif filename.count("_") == 1:
            # A short one as instrument_runnumber
            iname = filename.split("_")[0]
            str_runnumber = filename.split("_")[1]
            if str_runnumber.isdigit() is True and int(str_runnumber) > 0:
                # Acccepted format
                ishort = config.getInstrument(iname).shortName()
                wsname = "%s_%s_event" % (ishort, str_runnumber)
            else:
                # Non-supported
                print "File name / run number in such format %s is not supported. " % (filename)
                return None

        else:
            # Unsupported format
            print "File name / run number in such format %s is not supported. " % (filename)
            return None

        # Load
        try: 
            ws = api.Load(Filename=filename, OutputWorkspace=wsname)
        except:
            ws = None

        return ws

    
    def _plotTimeCounts(self, wksp):
        """ Plot time/counts 
        """
        import datetime
        # Rebin events by pulse time
        try:
            # Get run start and run stop
            if wksp.getRun().hasProperty("run_start"): 
                runstart = wksp.getRun().getProperty("run_start").value
            else:
                runstart = wksp.getRun().getProperty("proton_charge").times[0]
            runstop = wksp.getRun().getProperty("proton_charge").times[-1]

            runstart = str(runstart).split(".")[0].strip()
            runstop = str(runstop).split(".")[0].strip()
            
            t0 = datetime.datetime.strptime(runstart, "%Y-%m-%dT%H:%M:%S")
            tf = datetime.datetime.strptime(runstop, "%Y-%m-%dT%H:%M:%S")
           
            # Calcualte 
            dt = tf-t0
            timeduration = dt.days*3600*24 + dt.seconds

            timeres = float(timeduration)/MAXTIMEBINSIZE
            if timeres < 1.0:
                timeres = 1.0

            sumwsname = "_Summed_%s"%(str(wksp))
            if AnalysisDataService.doesExist(sumwsname) is False:
                sumws = api.RebinByPulseTimes(InputWorkspace=wksp, OutputWorkspace = sumwsname, 
                    Params="0, %f, %d"%(timeres, timeduration))
                sumws = api.SumSpectra(InputWorkspace=sumws, OutputWorkspace=str(sumws))
                sumws = api.ConvertToPointData(InputWorkspace=sumws, OutputWorkspace=str(sumws))
            else:
                sumws = AnalysisDataService.retrieve(sumwsname)
        except Exception as e:
            return str(e)

        vecx = sumws.readX(0)
        vecy = sumws.readY(0)

        xmin = min(vecx)
        xmax = max(vecx)
        ymin = min(vecy)
        ymax = max(vecy)

        # Reset graph  
        self.ui.mainplot.set_xlim(xmin, xmax)
        self.ui.mainplot.set_ylim(ymin, ymax)

        self.ui.mainplot.set_xlabel('Time (seconds)', fontsize=13)
        self.ui.mainplot.set_ylabel('Counts', fontsize=13)

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

        try: 
            self.splitWksp(splitws, infows)
        except Exception as mtderror:
            self._setErrorMsg("Splitting Failed!\n %s" % (str(mtderror)))

        return

    def splitWksp(self, splitws, infows):
        """ Run FilterEvents
        """
        dogroupws = self.ui.checkBox_groupWS.isChecked()
        filterbypulse = self.ui.checkBox_filterByPulse.isChecked()
        startfrom1 = self.ui.checkBox_from1.isChecked()
        splitsamplelog = self.ui.checkBox_splitLog.isChecked()

        corr2sample = str(self.ui.comboBox_tofCorr.currentText())
        how2skip = str(self.ui.comboBox_skipSpectrum.currentText())

        kwargs = {}
        if corr2sample == "Direct":
            ei = float(self.ui.lineEdit_Ei.text())
            kwargs["IncidentEnergy"] = ei
        elif corr2sample == "Customized":
            corrws = str(self.ui.comboBox_corrWS.currentText())
            kwargs["DetectorTOFCorrectionWorkspace"] = corrws

        # Output workspace name
        outbasewsname = str(self.ui.lineEdit_outwsname.text())
        if len(outbasewsname) == 0:
            outbasewsname = "tempsplitted"
            self.ui.lineEdit_outwsname.setText(outbasewsname)
         
        api.FilterEvents(
                InputWorkspace          = self._dataWS, 
                SplitterWorkspace       = splitws, 
                InformationWorkspace    = infows, 
                OutputWorkspaceBaseName = outbasewsname, 
                GroupWorkspaces         = dogroupws, 
                FilterByPulseTime       = filterbypulse, 
                CorrectionToSample      = corr2sample, 
                SpectrumWithoutDetector = how2skip, 
                SplitSampleLogs         = splitsamplelog, 
                OutputWorkspaceIndexedFrom1     = startfrom1, 
                OutputTOFCorrectionWorkspace    = 'TOFCorrTable', **kwargs)  

        return

    def showHideEi(self):
        """
        """
        corrtype = str(self.ui.comboBox_tofCorr.currentText())

        # Incident energy
        if corrtype == "Direct":
            self.ui.label_Ei.show()
            self.ui.lineEdit_Ei.show()
        else:
            self.ui.label_Ei.hide()
            self.ui.lineEdit_Ei.hide()

        # Workspace
        if corrtype == "Customized":
            self.ui.label_Ei_2.show()
            self.ui.comboBox_corrWS.show()
            self.ui.pushButton_refreshCorrWSList.show()

            # Search for table workspace
            self._searchTableWorkspaces()

        else:
            self.ui.label_Ei_2.hide()
            self.ui.comboBox_corrWS.hide()
            self.ui.pushButton_refreshCorrWSList.hide()

        return


    def _searchTableWorkspaces(self):
        """ Search table workspaces and add to 'comboBox_corrWS'
        """
        wsnames = AnalysisDataService.getObjectNames()

        tablewsnames = []
        for wsname in wsnames:
            wksp = AnalysisDataService.retrieve(wsname)
            if isinstance(wksp, mantid.api._api.ITableWorkspace): 
                tablewsnames.append(wsname)
        # ENDFOR

        self.ui.comboBox_corrWS.clear()
        if len(tablewsnames) > 0: 
            self.ui.comboBox_corrWS.addItems(tablewsnames)

        return

    def _clearErrorMsg(self):
        """ Clear error message
        """
        #self.ui.plainTextEdit_ErrorMsg.setPlainText("")
        #self.ui.label_error.hide()

        return

    def _setErrorMsg(self, errmsg):
        """ Clear error message
        """ 
        #self.ui.plainTextEdit_ErrorMsg.setPlainText(errmsg)
        #self.ui.label_error.show()

        #print "Testing Pop-up Error Message Window: %s" % (errmsg)
        self._errMsgWindow = MyPopErrorMsg()
        self._errMsgWindow.setMessage(errmsg)
        self._errMsgWindow.show()

        return


    def _resetGUI(self, resetfilerun=False, resetwslist=False):
        """ Reset GUI including all text edits and etc. 
        """
        if resetfilerun is True:
            self.ui.lineEdit.clear()

        # Plot related
        self.ui.lineEdit_3.clear()
        self.ui.lineEdit_4.clear()
        self.ui.horizontalSlider.setValue(0)
        self.ui.horizontalSlider_2.setValue(100)


        self.ui.lineEdit_outwsname.clear()
        self.ui.lineEdit_title.clear()

        # Filter by log value
        self.ui.lineEdit_5.clear()
        self.ui.lineEdit_6.clear()

        self.ui.verticalSlider_2.setValue(0)
        self.ui.verticalSlider.setValue(100)

        ylim = self.ui.mainplot.get_ylim()
        miny = ylim[0]
        maxy = ylim[1]
        xlim = self.ui.mainplot.get_xlim() 
        setp(self.lowerslideline, xdata=xlim, ydata=[miny, miny])
        setp(self.upperslideline, xdata=xlim, ydata=[maxy, maxy])
        self.ui.graphicsView.draw()

        self.ui.lineEdit_7.clear()
        self.ui.lineEdit_8.clear()
        self.ui.lineEdit_9.clear()

        # Filter by time
        self.ui.lineEdit_timeInterval.clear()

        # Advanced setup
        self.ui.comboBox_tofCorr.setCurrentIndex(0)
        self.ui.lineEdit_Ei.clear()

        self.ui.checkBox_fastLog.setCheckState(False)
        self.ui.checkBox_doParallel.setCheckState(False)

        self.ui.comboBox_skipSpectrum.setCurrentIndex(0)
        
        self.ui.checkBox_filterByPulse.setCheckState(False)
        self.ui.checkBox_from1.setCheckState(False)
        self.ui.checkBox_groupWS.setCheckState(True)
        self.ui.checkBox_splitLog.setCheckState(False)

        # Error message
        # self.ui.plainTextEdit_ErrorMsg.clear()

        return

