# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-lines, too-many-instance-attributes
import numpy as np

from qtpy.QtWidgets import QFileDialog, QMainWindow, QMessageBox, QSlider, QVBoxLayout, QWidget
from qtpy.QtGui import QDoubleValidator, QDesktopServices
from qtpy.QtCore import QUrl, QLocale

import mantid
import mantid.simpleapi as api
import mantid.kernel
from mantid.kernel import Logger
from mantid.simpleapi import AnalysisDataService

from mantid.kernel import ConfigService
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from matplotlib.pyplot import Figure, setp
import os

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("Filter_Events").information("Using legacy ui importer")
    from mantidplot import load_ui

HUGE_FAST = 10000
HUGE_PARALLEL = 100000
MAXTIMEBINSIZE = 3000


class MainWindow(QMainWindow):
    """Class of Main Window (top)"""

    _errMsgWindow = None

    def __init__(self, parent=None, window_flags=None):
        """Initialization and set up"""
        # Base class
        QMainWindow.__init__(self, parent)

        if window_flags:
            self.setWindowFlags(window_flags)

        # Mantid configuration
        config = ConfigService.Instance()
        self._instrument = config["default.instrument"]

        # Central widget
        self.centralwidget = QWidget(self)

        # UI Window (from Qt Designer)
        self.ui = load_ui(__file__, "MainWindow.ui", baseinstance=self)
        mpl_layout = QVBoxLayout()
        self.ui.graphicsView.setLayout(mpl_layout)
        self.fig = Figure(figsize=(4, 3), layout="constrained")
        self.canvas = FigureCanvas(self.fig)
        self.ui.mainplot = self.fig.add_subplot(111, projection="mantid", xlabel="x-units", ylabel="y-units")
        mpl_layout.addWidget(self.canvas)

        # Do initialize plotting
        vecx, vecy, xlim, ylim = self.computeMock()

        self.mainline = self.ui.mainplot.plot(vecx, vecy, "r-")

        leftx = [xlim[0], xlim[0]]
        lefty = [ylim[0], ylim[1]]
        self.leftslideline = self.ui.mainplot.plot(leftx, lefty, "b--")
        rightx = [xlim[1], xlim[1]]
        righty = [ylim[0], ylim[1]]
        self.rightslideline = self.ui.mainplot.plot(rightx, righty, "g--")
        upperx = [xlim[0], xlim[1]]
        uppery = [ylim[1], ylim[1]]
        self.upperslideline = self.ui.mainplot.plot(upperx, uppery, "b--")
        lowerx = [xlim[0], xlim[1]]
        lowery = [ylim[0], ylim[0]]
        self.lowerslideline = self.ui.mainplot.plot(lowerx, lowery, "g--")

        self.canvas.mpl_connect("button_press_event", self.on_mouseDownEvent)

        # Set up horizontal slide (integer) and string value
        self._leftSlideValue = 0
        self._rightSlideValue = 100

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

        # File loader
        self.scanEventWorkspaces()
        self.ui.pushButton_refreshWS.clicked.connect(self.scanEventWorkspaces)
        self.ui.pushButton_browse.clicked.connect(self.browse_File)
        self.ui.pushButton_load.clicked.connect(self.load_File)
        self.ui.pushButton_3.clicked.connect(self.use_existWS)

        # Filter by time
        self.ui.pushButton_filterTime.clicked.connect(self.filterByTime)
        self.ui.lineEdit_timeInterval.returnPressed.connect(self.filterByTime)

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

        # Set up validators for numeric edits
        self.dvalidator = QDoubleValidator()
        locale = QLocale.c()
        locale.setNumberOptions(QLocale.RejectGroupSeparator)
        self.dvalidator.setLocale(locale)
        self.dvalidator.setDecimals(6)
        self.populate_line_edits_default()
        self.doubleLineEdits = [
            (self.ui.leStartTime, self.set_startTime),
            (self.ui.leStopTime, self.set_stopTime),
            (self.ui.leMinimumValue, self.set_minLogValue),
            (self.ui.leMaximumValue, self.set_maxLogValue),
            (self.ui.leStepSize, None),
            (self.ui.leValueTolerance, None),
            (self.ui.leTimeTolerance, None),
            (self.ui.leIncidentEnergy, None),
        ]

        for edit in self.doubleLineEdits:
            edit[0].setValidator(self.dvalidator)
            edit[0].callback = edit[1]
        self.connect_signals()

        # Set up for filtering (advanced setup)
        self._tofcorrection = False
        self.ui.checkBox_fastLog.setChecked(False)
        self.ui.checkBox_filterByPulse.setChecked(False)
        self.ui.checkBox_from1.setChecked(False)
        self.ui.checkBox_groupWS.setChecked(True)

        self.ui.comboBox_tofCorr.currentIndexChanged.connect(self.showHideEi)
        self.ui.pushButton_refreshCorrWSList.clicked.connect(self._searchTableWorkspaces)

        self.ui.label_Ei.hide()
        self.ui.leIncidentEnergy.hide()
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
        mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface, "EventFilter", False)

    def on_mouseDownEvent(self, event):
        """Respond to pick up a value with mouse down event"""
        x = event.xdata
        y = event.ydata

        if x is not None and y is not None:
            msg = "You've clicked on a bar with coords:\n %f, %f" % (x, y)
            QMessageBox.information(self, "Click!", msg)

    def reformat(self):
        self.sender().setText(f"{float(self.sender().text()):.6f}")

    def callback(self):
        sender_edit = getattr(self.ui, self.sender().objectName())
        if sender_edit.callback is not None:
            sender_edit.callback()

    def populate_line_edits_default(self):
        ylim = self.ui.mainplot.get_ylim()
        xlim = self.ui.mainplot.get_xlim()

        self.ui.leStartTime.setText(f"{xlim[0]:.6f}")
        self.ui.leStopTime.setText(f"{xlim[1]:.6f}")
        self.ui.leMinimumValue.setText(f"{ylim[0]:.6f}")
        self.ui.leMaximumValue.setText(f"{ylim[1]:.6f}")

    def computeMock(self):
        """Compute vecx and vecy as mocking"""
        x0 = 0.0
        xf = 1.0
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
        ylim = [-1.0, 1]

        return (vecx, vecy, xlim, ylim)

    def move_leftSlider(self):
        """Re-setup left range line in figure.
        Triggered by a change in Qt Widget.  NO EVENT is required.
        """
        newx = self.ui.horizontalSlider.value()

        if newx <= self._rightSlideValue and newx != self._leftSlideValue:
            # Allowed value: move the value bar
            self._leftSlideValue = newx

            # Move the vertical line
            xlim = self.ui.mainplot.get_xlim()

            if self.ui.leStopTime.text():
                newx = min(xlim[0] + newx * (xlim[1] - xlim[0]) * 0.01, float(self.ui.leStopTime.text()))
            else:
                newx = xlim[0] + newx * (xlim[1] - xlim[0]) * 0.01

            leftx = [newx, newx]
            lefty = self.ui.mainplot.get_ylim()
            setp(self.leftslideline, xdata=leftx, ydata=lefty)
            self.canvas.draw()

            # Change value
            self.ui.leStartTime.setText(f"{newx:.6f}")

        else:
            # Reset the value to original value
            self.ui.horizontalSlider.setValue(self._leftSlideValue)

    def set_startTime(self):
        """Set the starting time and left slide bar"""
        inps = float(self.ui.leStartTime.text())
        info_msg = "Starting time = %s" % inps
        Logger("Filter_Events").information(info_msg)

        xlim = self.ui.mainplot.get_xlim()
        # Set value to valid range
        newtime0 = np.clip(inps, xlim[0], float(self.ui.leStopTime.text()))
        # Convert to integer slide value
        ileftvalue = np.clip(int((newtime0 - xlim[0]) / (xlim[1] - xlim[0]) * 100), 0, self._rightSlideValue)
        debug_msg = "iLeftSlide = %s" % str(ileftvalue)
        Logger("Filter_Events").debug(debug_msg)

        info_msg = "Corrected iLeftSlide = {} (vs. right = {})".format(ileftvalue, self._rightSlideValue)
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
        if newtime0 != inps:
            self.ui.leStartTime.setText(f"{newtime0:.6f}")

    def move_rightSlider(self):
        """Re-setup left range line in figure.
        Triggered by a change in Qt Widget.  NO EVENT is required.
        """
        newx = self.ui.horizontalSlider_2.value()
        if newx >= self._leftSlideValue and newx != self._rightSlideValue:
            # Allowed value: move the value bar
            self._rightSlideValue = newx

            xlim = self.ui.mainplot.get_xlim()

            if self.ui.leStartTime.text():
                # that is not entirely fool proof, as the user could still remove the value in the field after putting
                # a non round percent, but this a) is unlikely and b) will not crash mantid, only show an artifact
                newx = max(xlim[0] + newx * (xlim[1] - xlim[0]) * 0.01, float(self.ui.leStartTime.text()))
            else:
                newx = xlim[0] + newx * (xlim[1] - xlim[0]) * 0.01

            leftx = [newx, newx]
            lefty = self.ui.mainplot.get_ylim()
            setp(self.rightslideline, xdata=leftx, ydata=lefty)
            self.canvas.draw()

            # Change value
            self.ui.leStopTime.setText(f"{newx:.6f}")

        else:
            # Reset the value
            self.ui.horizontalSlider_2.setValue(self._rightSlideValue)

    def set_stopTime(self):
        """Set the stopping time and right slide bar"""
        inps = float(self.ui.leStopTime.text())
        Logger("Filter_Events").information("Stopping time = {}".format(inps))

        xlim = self.ui.mainplot.get_xlim()
        # Set value to valid range
        newtimef = np.clip(inps, float(self.ui.leStartTime.text()), xlim[1])
        # Convert to integer slide value
        irightvalue = np.clip(int((newtimef - xlim[0]) / (xlim[1] - xlim[0]) * 100), self._leftSlideValue, 100)
        Logger("Filter_Events").information("iRightSlide = {}".format(irightvalue))

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
        if newtimef != inps:
            self.ui.leStopTime.setText(f"{newtimef:.6f}")

    def move_lowerSlider(self):
        """Re-setup upper range line in figure.
        Triggered by a change in Qt Widget.  NO EVENT is required.
        """
        inewy = self.ui.verticalSlider_2.value()
        debug_msg = "LowerSlFider is set with value {} vs. class variable {}".format(inewy, self._lowerSlideValue)
        Logger("Filter_Events").debug(debug_msg)

        # Return with no change
        if inewy == self._lowerSlideValue:
            # No change
            return

        if inewy >= self._upperSlideValue:
            # Out of upper range
            inewy = self._upperSlideValue - 1

        setLineEdit = inewy != 0 or self._lowerSlideValue >= 0

        # Move the lower vertical bar
        ylim = self.ui.mainplot.get_ylim()
        newy = ylim[0] + inewy * (ylim[1] - ylim[0]) * 0.01
        lowerx = self.ui.mainplot.get_xlim()
        lowery = [newy, newy]
        setp(self.lowerslideline, xdata=lowerx, ydata=lowery)
        self.canvas.draw()

        # Set line edit input
        if setLineEdit is True:
            # Change value to line edit (5)
            self.ui.leMinimumValue.setText(f"{newy:.6f}")
            # Reset the class variable
            self._lowerSlideValue = inewy

    def set_minLogValue(self):
        """Set the starting time and left slide bar"""
        inps = float(self.ui.leMinimumValue.text())
        debug_msg = "Minimum Log Value = {}".format(self.ui.leMinimumValue.text())
        Logger("Filter_Events").debug(debug_msg)

        ylim = self.ui.mainplot.get_ylim()
        # Set value to valid range
        newminY = np.clip(inps, ylim[0], float(self.ui.leMaximumValue.text()))
        # Convert to integer slide value
        iminlogval = np.clip(int((newminY - ylim[0]) / (ylim[1] - ylim[0]) * 100), 0, self._upperSlideValue)
        Logger("Filter_Events").debug("ilowerSlide = {}".format(iminlogval))

        # Move the vertical line
        lowerx = self.ui.mainplot.get_xlim()
        lowery = [newminY, newminY]
        setp(self.lowerslideline, xdata=lowerx, ydata=lowery)
        self.canvas.draw()

        # Move the slide bar (lower)
        self._lowerSlideValue = iminlogval
        debug_msg = "LineEdit5 set slide to {}".format(self._lowerSlideValue)
        Logger("Filter_Events").debug(debug_msg)
        self.ui.verticalSlider_2.setValue(self._lowerSlideValue)

        # Reset line Edit if using default
        if newminY != inps:
            self.ui.leMinimumValue.setText(f"{newminY:.6f}")

    def move_upperSlider(self):
        """Re-setup upper range line in figure.
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
        newy = ylim[0] + inewy * (ylim[1] - ylim[0]) * 0.01
        upperx = self.ui.mainplot.get_xlim()
        uppery = [newy, newy]
        setp(self.upperslideline, xdata=upperx, ydata=uppery)
        self.canvas.draw()

        # Change value
        if setLineEdit:
            self.ui.leMaximumValue.setText(f"{newy:.6f}")
            self._upperSlideValue = inewy

    def set_maxLogValue(self):
        """Set maximum log value from line-edit"""
        inps = float(self.ui.leMaximumValue.text())
        debug_msg = "Maximum Log Value = {}".format(inps)
        Logger("Filter_Events").debug(debug_msg)

        ylim = self.ui.mainplot.get_ylim()
        # Set value to valid range
        newmaxY = np.clip(inps, float(self.ui.leMinimumValue.text()), ylim[1])
        # Convert to integer slide value
        imaxlogval = np.clip(int((newmaxY - ylim[0]) / (ylim[1] - ylim[0]) * 100), self._lowerSlideValue, 100)

        debug_msg = "iUpperSlide = {}".format(imaxlogval)
        Logger("Filter_Events").debug(debug_msg)

        # Move the vertical line
        upperx = self.ui.mainplot.get_xlim()
        uppery = [newmaxY, newmaxY]
        setp(self.upperslideline, xdata=upperx, ydata=uppery)
        self.canvas.draw()

        # Set the value to upper slider
        self._upperSlideValue = imaxlogval
        self.ui.verticalSlider.setValue(self._upperSlideValue)

        # Set the value to editor if necessary
        if newmaxY != inps:
            self.ui.leMaximumValue.setText(f"{newmaxY:.6f}")

    def browse_File(self):
        """Open a file dialog to get file"""
        filename = QFileDialog.getOpenFileName(self, "Input File Dialog", self._defaultdir, "Data (*.nxs *.dat);;All files (*)")
        if isinstance(filename, tuple):
            filename = filename[0]

        self.ui.lineEdit.setText(filename)

        Logger("Filter_Events").information('Selected file: "{}"'.format(filename))

    def load_File(self):
        """Load the file by file name or run number"""
        # Get file name from line editor
        filename = str(self.ui.lineEdit.text())

        dataws = self._loadFile(str(filename))
        if dataws is None:
            error_msg = "Unable to locate run {} in default directory {}.".format(filename, self._defaultdir)
            Logger("Filter_Events").error(error_msg)
            self._setErrorMsg(error_msg)
        else:
            self._importDataWorkspace(dataws)
            self._defaultdir = os.path.dirname(str(filename))

        # Reset GUI
        self._resetGUI(resetfilerun=False)

    def use_existWS(self):
        """Set up workspace to an existing one"""
        wsname = str(self.ui.comboBox.currentText())

        try:
            dataws = AnalysisDataService.retrieve(wsname)
            self._importDataWorkspace(dataws)
        except KeyError:
            pass

        # Reset GUI
        self._resetGUI(resetfilerun=True)

    def plotLogValue(self):
        """Plot log value"""
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
        vectimes = np.append(vectimes, tf)
        vecvalue = np.append(vecvalue, vecvalue[-1])

        vecreltimes = (vectimes - t0) / np.timedelta64(1, "s")

        # Set to plot
        self.disconnect_signals()
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

        setp(self.upperslideline, xdata=xlim, ydata=[ylim[1], ylim[1]])
        self._upperSlideValue = 100
        self.ui.verticalSlider.setValue(self._upperSlideValue)
        self.populate_line_edits_default()
        self.connect_signals()
        self.canvas.draw()

        # Load property's statistic and give suggestion on parallel and fast log
        timeavg = self._dataWS.getRun().getTimeAveragedValue(logname)
        numentries = samplelog.size()
        stat = samplelog.getStatistics()

        duration = stat.duration
        mean = stat.mean
        freq = float(numentries) / float(duration)

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
        """Import data workspace for filtering"""
        if dataws is None:
            return

        # Plot time counts
        errmsg = self._plotTimeCounts(dataws)
        if errmsg is not None:
            errmsg = "Workspace {} has invalid sample logs for splitting. Loading \
                    failure! \n{}\n".format(
                dataws, errmsg
            )
            self._setErrorMsg(errmsg)
            return False

        # Import log
        self._sampleLogNames = [""]

        run = dataws.getRun()
        plist = run.getProperties()
        for p in plist:
            try:
                times = p.times
                if len(times) > 1 and np.isreal(p.value[0]):
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
        """ """
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
        """Load file or run
        File will be loaded to a workspace shown in MantidPlot
        """
        config = ConfigService

        # Check input file name and output workspace name
        if filename.isdigit() is True:
            # Construct a file name from run number
            runnumber = int(filename)
            if runnumber <= 0:
                error_msg = "Run number cannot be less or equal to zero.  User gives {}.".format(filename)
                Logger("Filter_Events").error(error_msg)
                return None
            else:
                ishort = config.getInstrument(self._instrument).shortName()
                filename = "{}_{}".format(ishort, filename)
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
                wsname = "{}_{}_event".format(ishort, str_runnumber)
            else:
                # Non-supported
                error_msg = "File name / run number in such format {} is not supported.".format(filename)
                Logger("Filter_Events").error(error_msg)

                return None

        else:
            # Unsupported format
            error_msg = "File name / run number in such format {} is not supported.".format(filename)
            Logger("Filter_Events").error(error_msg)

            return None

        # Load
        try:
            ws = api.Load(Filename=filename, OutputWorkspace=wsname)
        except RuntimeError as e:
            return str(e)

        return ws

    def _plotTimeCounts(self, wksp):
        """Plot time/counts"""
        import datetime

        # Rebin events by pulse time
        try:
            # Get run start
            if wksp.getRun().hasProperty("run_start"):
                runstart = wksp.getRun().getProperty("run_start").value
            elif wksp.getRun().hasProperty("proton_charge"):
                runstart = wksp.getRun().getProperty("proton_charge").times[0]
            else:
                runstart = wksp.getRun().getProperty("start_time").value

            # get run stop
            if wksp.getRun().hasProperty("proton_charge"):
                runstop = wksp.getRun().getProperty("proton_charge").times[-1]
                runstop = str(runstop).split(".")[0].strip()
                tf = datetime.datetime.strptime(runstop, "%Y-%m-%dT%H:%M:%S")
            else:
                last_pulse = wksp.getPulseTimeMax().toISO8601String()
                tf = datetime.datetime.strptime(last_pulse[:19], "%Y-%m-%dT%H:%M:%S")
                tf += datetime.timedelta(0, wksp.getTofMax() / 1000000)

            runstart = str(runstart).split(".")[0].strip()

            t0 = datetime.datetime.strptime(runstart, "%Y-%m-%dT%H:%M:%S")

            # Calculate
            dt = tf - t0
            timeduration = dt.days * 3600 * 24 + dt.seconds
            timeres = float(timeduration) / MAXTIMEBINSIZE
            if timeres < 1.0:
                timeres = 1.0

            sumwsname = "_Summed_{}".format(wksp)
            if not AnalysisDataService.doesExist(sumwsname):
                sumws = api.SumSpectra(InputWorkspace=wksp, OutputWorkspace=sumwsname)
                sumws = api.RebinByPulseTimes(InputWorkspace=sumws, OutputWorkspace=sumwsname, Params="{}".format(timeres))
                sumws = api.ConvertToPointData(InputWorkspace=sumws, OutputWorkspace=sumwsname)
            else:
                sumws = AnalysisDataService.retrieve(sumwsname)
        except RuntimeError as e:
            return str(e)

        vecx = sumws.readX(0)
        vecy = sumws.readY(0)

        # if there is only one xbin in the summed workspace, that means we have an evetn file without pulse,
        # and in this case we use the original workspace time limits
        if len(vecx) == 1:
            xmin = min(wksp.readX(0)) / 1000000
            xmax = max(wksp.readX(0)) / 1000000
        else:
            xmin = min(vecx)
            xmax = max(vecx)

        ymin = min(vecy)
        ymax = max(vecy)

        # Reset graph
        self.ui.mainplot.set_xlim(xmin, xmax)
        self.ui.mainplot.set_ylim(ymin, ymax)

        self.ui.mainplot.set_xlabel("Time (seconds)")
        self.ui.mainplot.set_ylabel("Counts")

        # Set up main line
        setp(self.mainline, xdata=vecx, ydata=vecy)

        # Reset slide
        newslidery = [min(vecy), max(vecy)]

        newleftx = xmin + (xmax - xmin) * self._leftSlideValue * 0.01
        setp(self.leftslideline, xdata=[newleftx, newleftx], ydata=newslidery)

        newrightx = xmin + (xmax - xmin) * self._rightSlideValue * 0.01
        setp(self.rightslideline, xdata=[newrightx, newrightx], ydata=newslidery)
        self.canvas.draw()

    def filterByTime(self):
        """Filter by time"""
        # Generate event filters
        if not self._dataWS:
            error_msg = "No workspace has been loaded for use!"
            Logger("Filter_Events").error(error_msg)
            return

        kwargs = {}

        xlim = self.ui.mainplot.get_xlim()
        kwargs["StartTime"] = self.ui.leStartTime.text() if self.ui.leStartTime.text() != "" else str(xlim[0])
        kwargs["StopTime"] = self.ui.leStopTime.text() if self.ui.leStopTime.text() != "" else str(xlim[1])

        if self.ui.lineEdit_timeInterval.text() != "":
            kwargs["TimeInterval"] = self.ui.lineEdit_timeInterval.text()
        kwargs["useReverseLogarithmic"] = self.ui.useReverseLogarithmic.isChecked()

        splitwsname = str(self._dataWS) + "_splitters"
        splitinfowsname = str(self._dataWS) + "_info"

        title = str(self.ui.lineEdit_title.text())
        fastLog = self.ui.checkBox_fastLog.isChecked()

        try:
            splitws, infows = api.GenerateEventsFilter(
                InputWorkspace=self._dataWS,
                UnitOfTime="Seconds",
                TitleOfSplitters=title,
                OutputWorkspace=splitwsname,
                FastLog=fastLog,
                InformationWorkspace=splitinfowsname,
                **kwargs,
            )
            self.splitWksp(splitws, infows)
        except (RuntimeError, ValueError) as e:
            Logger("Filter_Events").error("Splitting failed ! \n {0}".format(e))
            return

    def filterByLogValue(self):
        """Filter by log value"""
        # Generate event filter
        kwargs = {}
        samplelog = str(self.ui.comboBox_2.currentText())
        if len(samplelog) == 0:
            error_msg = "No sample log is selected!"
            Logger("Filter_Events").error(error_msg)
            return

        if self.ui.leStartTime.text() != "":
            rel_starttime = float(self.ui.leStartTime.text())
            kwargs["StartTime"] = str(rel_starttime)

        if self.ui.leStopTime.text() != "":
            rel_stoptime = float(self.ui.leStopTime.text())
            kwargs["StopTime"] = str(rel_stoptime)

        if self.ui.leMinimumValue.text() != "":
            minlogvalue = float(self.ui.leMinimumValue.text())
            kwargs["MinimumLogValue"] = minlogvalue

        if self.ui.leMaximumValue.text() != "":
            maxlogvalue = float(self.ui.leMaximumValue.text())
            kwargs["MaximumLogValue"] = maxlogvalue

        if self.ui.leStepSize.text() != "":
            logvalueintv = float(self.ui.leStepSize.text())
            kwargs["LogValueInterval"] = logvalueintv
        logvalchangedir = str(self.ui.comboBox_4.currentText())
        kwargs["FilterLogValueByChangingDirection"] = logvalchangedir

        if self.ui.leTimeTolerance.text() != "":
            logvalueintv = float(self.ui.leTimeTolerance.text())
            kwargs["TimeTolerance"] = logvalueintv
        logboundtype = str(self.ui.comboBox_5.currentText())
        kwargs["LogBoundary"] = logboundtype

        if self.ui.leValueTolerance.text() != "":
            logvaluetol = float(self.ui.leValueTolerance.text())
            kwargs["LogValueTolerance"] = logvaluetol

        splitwsname = str(self._dataWS) + "_splitters"
        splitinfowsname = str(self._dataWS) + "_info"
        fastLog = self.ui.checkBox_fastLog.isChecked()

        title = str(self.ui.lineEdit_title.text())
        try:
            splitws, infows = api.GenerateEventsFilter(
                InputWorkspace=self._dataWS,
                UnitOfTime="Seconds",
                TitleOfSplitters=title,
                OutputWorkspace=splitwsname,
                LogName=samplelog,
                FastLog=fastLog,
                InformationWorkspace=splitinfowsname,
                **kwargs,
            )
            self.splitWksp(splitws, infows)
        except RuntimeError as e:
            self._setErrorMsg("Splitting Failed!\n %s" % (str(e)))

    def splitWksp(self, splitws, infows):
        """Run FilterEvents"""
        dogroupws = self.ui.checkBox_groupWS.isChecked()
        filterbypulse = self.ui.checkBox_filterByPulse.isChecked()
        startfrom1 = self.ui.checkBox_from1.isChecked()
        splitsamplelog = self.ui.checkBox_splitLog.isChecked()

        corr2sample = str(self.ui.comboBox_tofCorr.currentText())
        how2skip = str(self.ui.comboBox_skipSpectrum.currentText())

        kwargs = {}
        if corr2sample == "Direct":
            if not self.ui.leIncidentEnergy.text():
                raise RuntimeError("To use direct corrections, incident energy field must have a valid value")
            ei = float(self.ui.leIncidentEnergy.text())
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
            InputWorkspace=self._dataWS,
            SplitterWorkspace=splitws,
            InformationWorkspace=infows,
            OutputWorkspaceBaseName=outbasewsname,
            GroupWorkspaces=dogroupws,
            FilterByPulseTime=filterbypulse,
            CorrectionToSample=corr2sample,
            SpectrumWithoutDetector=how2skip,
            SplitSampleLogs=splitsamplelog,
            OutputWorkspaceIndexedFrom1=startfrom1,
            OutputTOFCorrectionWorkspace="TOFCorrTable",
            **kwargs,
        )

    def showHideEi(self):
        """ """
        corrtype = str(self.ui.comboBox_tofCorr.currentText())

        # Incident energy
        if corrtype == "Direct":
            self.ui.label_Ei.show()
            self.ui.leIncidentEnergy.show()
        else:
            self.ui.label_Ei.hide()
            self.ui.leIncidentEnergy.hide()

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
        """Search table workspaces and add to 'comboBox_corrWS'"""
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
        """Clear error message"""
        self._errMsgWindow = QMessageBox()
        self._errMsgWindow.setIcon(QMessageBox.Critical)
        self._errMsgWindow.setWindowTitle("Error")
        self._errMsgWindow.setStandardButtons(QMessageBox.Ok)
        self._errMsgWindow.setText(errmsg)
        result = self._errMsgWindow.exec_()

        return result

    def helpClicked(self):
        try:
            import mantidqt

            mantidqt.interfacemanager.InterfaceManager().showCustomInterfaceHelp("Filter Events", "utility")
        except ImportError:
            url = "http://docs.mantidproject.org/nightly/interfaces/{}.html" "".format("Filter Events")
            QDesktopServices.openUrl(QUrl(url))

    def connect_signals(self):
        for edit in self.doubleLineEdits:
            edit[0].editingFinished.connect(self.reformat)
            edit[0].editingFinished.connect(self.callback)

    def disconnect_signals(self):
        for edit in self.doubleLineEdits:
            edit[0].editingFinished.disconnect(self.reformat)
            edit[0].editingFinished.disconnect(self.callback)

    def _resetGUI(self, resetfilerun=False):
        """Reset GUI including all text edits and etc."""
        if resetfilerun is True:
            self.ui.lineEdit.clear()

        # Plot related
        self.disconnect_signals()
        self.ui.horizontalSlider.setValue(0)
        self.ui.horizontalSlider_2.setValue(100)
        self.ui.verticalSlider_2.setValue(0)
        self.ui.verticalSlider.setValue(100)
        self.populate_line_edits_default()
        self.ui.lineEdit_outwsname.clear()
        self.ui.lineEdit_title.clear()
        self.connect_signals()

        ylim = self.ui.mainplot.get_ylim()
        miny = ylim[0]
        maxy = ylim[1]
        xlim = self.ui.mainplot.get_xlim()
        setp(self.lowerslideline, xdata=xlim, ydata=[miny, miny])
        setp(self.upperslideline, xdata=xlim, ydata=[maxy, maxy])

        self.ui.leStepSize.clear()
        self.ui.leValueTolerance.clear()
        self.ui.leTimeTolerance.clear()

        # Filter by time
        self.ui.lineEdit_timeInterval.clear()

        # Advanced setup
        self.ui.comboBox_tofCorr.setCurrentIndex(0)
        self.ui.leIncidentEnergy.clear()

        self.ui.checkBox_fastLog.setCheckState(False)
        self.ui.checkBox_doParallel.setCheckState(False)

        self.ui.comboBox_skipSpectrum.setCurrentIndex(0)

        self.ui.checkBox_filterByPulse.setCheckState(False)
        self.ui.checkBox_from1.setCheckState(False)
        self.ui.checkBox_groupWS.setCheckState(True)
        self.ui.checkBox_splitLog.setCheckState(False)

        self.canvas.draw()
