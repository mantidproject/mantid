# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-lines, too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)
import numpy

from qtpy.QtWidgets import (QFileDialog, QMainWindow, QMessageBox, QSlider, QVBoxLayout, QWidget)  # noqa
from qtpy.QtGui import (QDoubleValidator, QDesktopServices)  # noqa
from qtpy.QtCore import QUrl


import mantid
import mantid.simpleapi as api
import mantid.kernel
from mantid.kernel import Logger
from mantid.simpleapi import AnalysisDataService

from mantid.kernel import ConfigService
from MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from matplotlib.pyplot import (Figure, setp)
import os

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("Filter_Events").information('Using legacy ui importer')
    from mantidplot import load_ui

HUGE_FAST = 10000
HUGE_PARALLEL = 100000
MAXTIMEBINSIZE = 3000


class MainWindow(QMainWindow):
    """ Class of Main Window (top)
    """

    _errMsgWindow = None

    def __init__(self, parent=None):
        """ Initialization and set up
        """
        # Base class
        QMainWindow.__init__(self, parent)

        # Mantid configuration
        config = ConfigService.Instance()
        self._instrument = config["default.instrument"]

        # Central widget
        self.centralwidget = QWidget(self)

        # UI Window (from Qt Designer)
        self.ui = load_ui(__file__, 'MainWindow.ui', baseinstance=self)
        mpl_layout = QVBoxLayout()
        self.ui.graphicsView.setLayout(mpl_layout)
        self.fig = Figure()
        self.canvas = FigureCanvas(self.fig)
        self.ui.mainplot = self.fig.add_subplot(111, projection='mantid')
        mpl_layout.addWidget(self.canvas)

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

        self.canvas.mpl_connect('button_press_event', self.on_mouseDownEvent)

        # Set up horizontal slide (integer) and string value
        self._leftSlideValue = 0
        self._rightSlideValue = 99

        self.ui.horizontalSlider.setRange(0, 100)
        self.ui.horizontalSlider.setValue(self._leftSlideValue)
        self.ui.horizontalSlider.setTracking(True)
        self.ui.horizontalSlider.setTickPosition(QSlider.NoTicks)
        self.ui.horizontalSlider.valueChanged.connect(self.move_leftSlider)

        self.ui.horizontalSlider_2.setRange(0, 100)
        self.ui.horizontalSlider_2.setValue(self._rightSlideValue)
        self.ui.horizontalSlider_2.setTracking(True)
        self.ui.horizontalSlider_2.setTickPosition(QSlider.NoTicks)
        self.ui.horizontalSlider_2.valueChanged.connect(self.move_rightSlider)

        # self.connect(self.ui.lineEdit_3, QtCore.SIGNAL("textChanged(QString)"),
        #         self.set_startTime)
        self.ui.lineEdit_3.setValidator(QDoubleValidator(self.ui.lineEdit_3))
        self.ui.pushButton_setT0.clicked.connect(self.set_startTime)
        # self.connect(self.ui.lineEdit_4, QtCore.SIGNAL("textChanged(QString)"),
        #         self.set_stopTime)
        self.ui.lineEdit_4.setValidator(QDoubleValidator(self.ui.lineEdit_4))
        self.ui.pushButton_setTf.clicked.connect(self.set_stopTime)

        # File loader
        self.scanEventWorkspaces()
        self.ui.pushButton_refreshWS.clicked.connect(self.scanEventWorkspaces)
        self.ui.pushButton_browse.clicked.connect(self.browse_File)
        self.ui.pushButton_load.clicked.connect(self.load_File)
        self.ui.pushButton_3.clicked.connect(self.use_existWS)

        # Set up time
        self.ui.lineEdit_3.setValidator(QDoubleValidator(self.ui.lineEdit_3))
        self.ui.lineEdit_4.setValidator(QDoubleValidator(self.ui.lineEdit_4))

        # Filter by time
        self.ui.pushButton_filterTime.clicked.connect(self.filterByTime)

        # Filter by log value
        self.ui.lineEdit_5.setValidator(QDoubleValidator(self.ui.lineEdit_5))
        self.ui.lineEdit_6.setValidator(QDoubleValidator(self.ui.lineEdit_6))
        self.ui.lineEdit_7.setValidator(QDoubleValidator(self.ui.lineEdit_7))
        self.ui.lineEdit_8.setValidator(QDoubleValidator(self.ui.lineEdit_8))
        self.ui.lineEdit_9.setValidator(QDoubleValidator(self.ui.lineEdit_9))

        self.ui.lineEdit_5.textChanged.connect(self.set_minLogValue)
        self.ui.lineEdit_6.textChanged.connect(self.set_maxLogValue)

        dirchangeops = ["Both", "Increase", "Decrease"]
        self.ui.comboBox_4.addItems(dirchangeops)

        logboundops = ["Centre", "Left"]
        self.ui.comboBox_5.addItems(logboundops)

        self.ui.pushButton_4.clicked.connect(self.plotLogValue)

        self.ui.pushButton_filterLog.clicked.connect(self.filterByLogValue)

        # Set up help button
        self.ui.helpBtn.clicked.connect(self.helpClicked)

        # Set up vertical slide
        self._upperSlideValue = 99
        self._lowerSlideValue = 0

        self.ui.verticalSlider.setRange(0, 100)
        self.ui.verticalSlider.setValue(self._upperSlideValue)
        self.ui.verticalSlider.setTracking(True)
        self.ui.verticalSlider.valueChanged.connect(self.move_upperSlider)

        self.ui.verticalSlider_2.setRange(0, 100)
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)
        self.ui.verticalSlider_2.setTracking(True)
        self.ui.verticalSlider_2.valueChanged.connect(self.move_lowerSlider)

        # Set up for filtering (advanced setup)
        self._tofcorrection = False
        self.ui.checkBox_fastLog.setChecked(False)
        self.ui.checkBox_filterByPulse.setChecked(False)
        self.ui.checkBox_from1.setChecked(False)
        self.ui.checkBox_groupWS.setChecked(True)

        self.ui.comboBox_tofCorr.currentIndexChanged.connect(self.showHideEi)
        self.ui.pushButton_refreshCorrWSList.clicked.connect(self._searchTableWorkspaces)

        self.ui.lineEdit_Ei.setValidator(QDoubleValidator(self.ui.lineEdit_Ei))

        self.ui.label_Ei.hide()
        self.ui.lineEdit_Ei.hide()
        self.ui.label_Ei_2.hide()
        self.ui.comboBox_corrWS.hide()
        self.ui.pushButton_refreshCorrWSList.hide()

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

        # register startup
        mantid.UsageService.registerFeatureUsage("Interface", "EventFilter", False)

    def on_mouseDownEvent(self, event):
        """ Respond to pick up a value with mouse down event
        """
        x = event.xdata
        y = event.ydata

        if x is not None and y is not None:
            msg = "You've clicked on a bar with coords:\n %f, %f" % (x, y)
            QMessageBox.information(self, "Click!", msg)

    def computeMock(self):
        """ Compute vecx and vecy as mocking
        """
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
            self.canvas.draw()

            # Change value
            self.ui.lineEdit_3.setText(str(newx))

        else:
            # Reset the value to original value
            self.ui.horizontalSlider.setValue(self._leftSlideValue)

    def set_startTime(self):
        """ Set the starting time and left slide bar
        """
        inps = str(self.ui.lineEdit_3.text())
        info_msg = "Starting time = %s" % (inps)
        Logger("Filter_Events").information(info_msg)

        xlim = self.ui.mainplot.get_xlim()
        if inps == "":
            # Empty. Use default
            newtime0 = xlim[0]
        else:
            newtime0 = float(inps)

        # Convert to integer slide value
        ileftvalue = int((newtime0-xlim[0])/(xlim[1] - xlim[0])*100)
        debug_msg = "iLeftSlide = %s" % str(ileftvalue)
        Logger("Filter_Events").debug(debug_msg)

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
        info_msg = 'Corrected iLeftSlide = {} (vs. right = {})'.format(ileftvalue,
                                                                       self._rightSlideValue)
        Logger("Filter_Events").information(info_msg)

        # Move the slide bar (left)
        self._leftSlideValue = ileftvalue

        # Move the vertical line
        leftx = [newtime0, newtime0]
        lefty = self.ui.mainplot.get_ylim()
        setp(self.leftslideline, xdata=leftx, ydata=lefty)
        self.canvas.draw()

        # Set the value to left slider
        self.ui.horizontalSlider.setValue(self._leftSlideValue)
        # Reset the value of line edit
        if resetT is True:
            self.ui.lineEdit_3.setText(str(newtime0))

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
            self.canvas.draw()

            # Change value
            self.ui.lineEdit_4.setText(str(newx))

        else:
            # Reset the value
            self.ui.horizontalSlider_2.setValue(self._rightSlideValue)

    def set_stopTime(self):
        """ Set the starting time and left slide bar
        """
        inps = str(self.ui.lineEdit_4.text())
        Logger("Filter_Events").information('Stopping time = {}'.format(inps))

        xlim = self.ui.mainplot.get_xlim()
        if inps == "":
            # Empty. Use default
            newtimef = xlim[1]
        else:
            # Parse
            newtimef = float(inps)

        # Convert to integer slide value
        irightvalue = int((newtimef-xlim[0])/(xlim[1] - xlim[0])*100)
        Logger("Filter_Events").information('iRightSlide = {}'.format(irightvalue))

        # Return if no change
        if irightvalue == self._rightSlideValue:
            return

        # Correct value
        resetT = True
        if irightvalue > 100:
            irightvalue = 100
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
        self.canvas.draw()

        # Set the value to left slider
        self.ui.horizontalSlider_2.setValue(self._rightSlideValue)

        # Reset to line edit
        if resetT:
            self.ui.lineEdit_4.setText(str(newtimef))

    def move_lowerSlider(self):
        """ Re-setup upper range line in figure.
        Triggered by a change in Qt Widget.  NO EVENT is required.
        """
        inewy = self.ui.verticalSlider_2.value()
        debug_msg = 'LowerSlFider is set with value {} vs. class variable {}'.format(inewy,
                                                                                     self._lowerSlideValue)
        Logger("Filter_Events").debug(debug_msg)

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
        self.canvas.draw()

        # Set line edit input
        if setLineEdit is True:
            # Change value to line edit (5)
            self.ui.lineEdit_5.setText(str(newy))
            # Reset the class variable
            self._lowerSlideValue = inewy

    def set_minLogValue(self):
        """ Set the starting time and left slide bar
        """
        debug_msg = 'Minimum Log Value = {}'.format(self.ui.lineEdit_5.text())
        Logger("Filter_Events").debug(debug_msg)

        ylim = self.ui.mainplot.get_ylim()

        if str(self.ui.lineEdit_5.text()) == "":
            # Empty. Default to minY
            newminY = ylim[0]
        else:
            # Non empty.  Parse
            newminY = float(self.ui.lineEdit_5.text())

        # Convert to integer slide value
        iminlogval = int((newminY-ylim[0])/(ylim[1] - ylim[0])*100)
        Logger("Filter_Events").debug('ilowerSlide = {}'.format(iminlogval))

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
        lowerx = self.ui.mainplot.get_xlim()
        lowery = [newminY, newminY]
        setp(self.lowerslideline, xdata=lowerx, ydata=lowery)
        self.canvas.draw()

        # Move the slide bar (lower)
        self._lowerSlideValue = iminlogval
        debug_msg = 'LineEdit5 set slide to {}'.format(self._lowerSlideValue)
        Logger("Filter_Events").debug(debug_msg)
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)

        # Reset line Edit if using default
        if resetL is True:
            self.ui.lineEdit_5.setText(str(newminY))

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

        # Move the upper value bar: upperx and uppery are
        # real value (float but not (0,100)) of the figure
        ylim = self.ui.mainplot.get_ylim()
        newy = ylim[0] + inewy*(ylim[1] - ylim[0])*0.01
        upperx = self.ui.mainplot.get_xlim()
        uppery = [newy, newy]
        setp(self.upperslideline, xdata=upperx, ydata=uppery)
        self.canvas.draw()

        # Change value
        if setLineEdit is True:
            self.ui.lineEdit_6.setText(str(newy))
            self._upperSlideValue = inewy

    def set_maxLogValue(self):
        """ Set maximum log value from line-edit
        """
        inps = str(self.ui.lineEdit_6.text())
        debug_msg = 'Maximum Log Value = {}'.format(inps)
        Logger("Filter_Events").debug(debug_msg)

        ylim = self.ui.mainplot.get_ylim()
        if inps == "":
            # Empty. Default to minY
            newmaxY = ylim[1]
        else:
            # Parse
            newmaxY = float(inps)

        # Convert to integer slide value
        imaxlogval = int((newmaxY-ylim[0])/(ylim[1] - ylim[0])*100)
        debug_msg = 'iUpperSlide = {}'.format(imaxlogval)
        Logger("Filter_Events").debug(debug_msg)

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
        upperx = self.ui.mainplot.get_xlim()
        uppery = [newmaxY, newmaxY]
        setp(self.upperslideline, xdata=upperx, ydata=uppery)
        self.canvas.draw()

        # Set the value to upper slider
        self._upperSlideValue = imaxlogval
        self.ui.verticalSlider.setValue(self._upperSlideValue)

        # Set the value to editor if necessary
        if resetL is True:
            self.ui.lineEdit_6.setText(str(newmaxY))

    def browse_File(self):
        """ Open a file dialog to get file
        """
        filename = QFileDialog.getOpenFileName(self, 'Input File Dialog',
                                               self._defaultdir, "Data (*.nxs *.dat);;All files (*)")
        if isinstance(filename, tuple):
            filename = filename[0]

        self.ui.lineEdit.setText(filename)

        Logger("Filter_Events").information('Selected file: "{}"'.format(filename))

    def load_File(self):
        """ Load the file by file name or run number
        """
        # Get file name from line editor
        filename = str(self.ui.lineEdit.text())

        dataws = self._loadFile(str(filename))
        if dataws is None:
            error_msg = 'Unable to locate run {} in default directory {}.'.format(filename, self._defaultdir)
            Logger("Filter_Events").error(error_msg)
            self._setErrorMsg(error_msg)
        else:
            self._importDataWorkspace(dataws)
            self._defaultdir = os.path.dirname(str(filename))

        # Reset GUI
        self._resetGUI(resetfilerun=False)

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
            error_msg = "Empty log!"
            Logger("Filter_Events").error(error_msg)

        # Convert absolute time to relative time in seconds
        t0 = self._dataWS.getRun().getProperty("proton_charge").times[0]

        # append 1 more log if original log only has 1 value
        tf = self._dataWS.getRun().getProperty("proton_charge").times[-1]
        vectimes = numpy.append(vectimes, tf)
        vecvalue = numpy.append(vecvalue, vecvalue[-1])

        vecreltimes = (vectimes - t0) / numpy.timedelta64(1, 's')

        # Set to plot
        xlim = [vecreltimes.min(), vecreltimes.max()]
        ylim = [vecvalue.min(), vecvalue.max()]
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
        self.canvas.draw()

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

        self.ui.label_meanvalue.setText("%.5e" % (mean))
        self.ui.label_timeAvgValue.setText("%.5e" % (timeavg))
        self.ui.label_freqValue.setText("%.5e" % (freq))
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
            errmsg = 'Workspace {} has invalid sample logs for splitting. Loading \
                    failure! \n{}\n'.format(dataws, errmsg)
            self._setErrorMsg(errmsg)
            return False

        # Import log
        self._sampleLogNames = [""]

        run = dataws.getRun()
        plist = run.getProperties()
        for p in plist:
            try:
                times = p.times
                if len(times) > 1 and numpy.isreal(p.value[0]):
                    self._sampleLogNames.append(p.name)
            # This is here for FloatArrayProperty. If a log value is of this type it does not have times
            except AttributeError:
                pass
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

        # Hide 'log name' above the graphic view
        self.ui.label_logname.hide()
        self.ui.label_lognamevalue.hide()

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
                error_msg = 'Run number cannot be less or equal to zero.  User gives {}.'.format(filename)
                Logger("Filter_Events").error(error_msg)
                return None
            else:
                ishort = config.getInstrument(self._instrument).shortName()
                filename = '{}_{}'.format(ishort, filename)
                wsname = filename + "_event"

        elif filename.count(".") > 0:
            # A proper file name
            wsname = os.path.splitext(os.path.split(filename)[1])[0]

        elif filename.count("_") == 1:
            # A short one as instrument_runnumber
            iname = filename.split("_")[0]
            str_runnumber = filename.split("_")[1]
            if str_runnumber.isdigit() is True and int(str_runnumber) > 0:
                # Accepted format
                ishort = config.getInstrument(iname).shortName()
                wsname = '{}_{}_event'.format(ishort, str_runnumber)
            else:
                # Non-supported
                error_msg = 'File name / run number in such format {} is not supported.'.format(filename)
                Logger("Filter_Events").error(error_msg)

                return None

        else:
            # Unsupported format
            error_msg = 'File name / run number in such format {} is not supported.'.format(filename)
            Logger("Filter_Events").error(error_msg)

            return None

        # Load
        try:
            ws = api.Load(Filename=filename, OutputWorkspace=wsname)
        except RuntimeError as e:
            ws = None
            return str(e)

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

            # Calculate
            dt = tf-t0
            timeduration = dt.days*3600*24 + dt.seconds

            timeres = float(timeduration)/MAXTIMEBINSIZE
            if timeres < 1.0:
                timeres = 1.0

            sumwsname = '_Summed_{}'.format(wksp)
            if AnalysisDataService.doesExist(sumwsname) is False:
                sumws = api.SumSpectra(InputWorkspace=wksp, OutputWorkspace=sumwsname)
                sumws = api.RebinByPulseTimes(InputWorkspace=sumws, OutputWorkspace=sumwsname,
                                              Params='{}'.format(timeres))
                sumws = api.ConvertToPointData(InputWorkspace=sumws, OutputWorkspace=sumwsname)
            else:
                sumws = AnalysisDataService.retrieve(sumwsname)
        except RuntimeError as e:
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
        self.canvas.draw()

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
        fastLog = self.ui.checkBox_fastLog.isChecked()

        splitws, infows = api.GenerateEventsFilter(InputWorkspace=self._dataWS,
                                                   UnitOfTime="Seconds",
                                                   TitleOfSplitters=title,
                                                   OutputWorkspace=splitwsname,
                                                   FastLog=fastLog,
                                                   InformationWorkspace=splitinfowsname,
                                                   **kwargs)

        self.splitWksp(splitws, infows)

    def filterByLogValue(self):
        """ Filter by log value
        """
        # Generate event filter
        kwargs = {}
        samplelog = str(self.ui.comboBox_2.currentText())
        if len(samplelog) == 0:
            error_msg = "No sample log is selected!"
            Logger("Filter_Events").error(error_msg)
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
        fastLog = self.ui.checkBox_fastLog.isChecked()

        title = str(self.ui.lineEdit_title.text())

        splitws, infows = api.GenerateEventsFilter(InputWorkspace=self._dataWS,
                                                   UnitOfTime="Seconds",
                                                   TitleOfSplitters=title,
                                                   OutputWorkspace=splitwsname,
                                                   LogName=samplelog,
                                                   FastLog=fastLog,
                                                   InformationWorkspace=splitinfowsname,
                                                   **kwargs)

        try:
            self.splitWksp(splitws, infows)
        except RuntimeError as e:
            self._setErrorMsg("Splitting Failed!\n %s" % (str(e)))

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

        api.FilterEvents(InputWorkspace=self._dataWS,
                         SplitterWorkspace=splitws,
                         InformationWorkspace=infows,
                         OutputWorkspaceBaseName=outbasewsname,
                         GroupWorkspaces=dogroupws,
                         FilterByPulseTime=filterbypulse,
                         CorrectionToSample=corr2sample,
                         SpectrumWithoutDetector=how2skip,
                         SplitSampleLogs=splitsamplelog,
                         OutputWorkspaceIndexedFrom1=startfrom1,
                         OutputTOFCorrectionWorkspace='TOFCorrTable', **kwargs)

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

    def _searchTableWorkspaces(self):
        """ Search table workspaces and add to 'comboBox_corrWS'
        """
        wsnames = AnalysisDataService.getObjectNames()

        tablewsnames = []
        for wsname in wsnames:
            wksp = AnalysisDataService.retrieve(wsname)
            if isinstance(wksp, mantid.api.ITableWorkspace):
                tablewsnames.append(wsname)
        # ENDFOR

        self.ui.comboBox_corrWS.clear()
        if len(tablewsnames) > 0:
            self.ui.comboBox_corrWS.addItems(tablewsnames)

    def _setErrorMsg(self, errmsg):
        """ Clear error message
        """
        self._errMsgWindow = QMessageBox()
        self._errMsgWindow.setIcon(QMessageBox.Critical)
        self._errMsgWindow.setWindowTitle('Error')
        self._errMsgWindow.setStandardButtons(QMessageBox.Ok)
        self._errMsgWindow.setText(errmsg)
        result = self._errMsgWindow.exec_()

        return result

    def helpClicked(self):
        try:
            from pymantidplot.proxies import showCustomInterfaceHelp
            showCustomInterfaceHelp("Filter Events")
        except ImportError:
            url = ("http://docs.mantidproject.org/nightly/interfaces/{}.html"
                   "".format("Filter Events"))
            QDesktopServices.openUrl(QUrl(url))

    def _resetGUI(self, resetfilerun=False):
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

        self.canvas.draw()
