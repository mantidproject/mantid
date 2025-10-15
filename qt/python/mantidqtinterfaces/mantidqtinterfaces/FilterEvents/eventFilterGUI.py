# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-lines, too-many-instance-attributes
import numpy as np

from qtpy.QtWidgets import QFileDialog, QMainWindow, QMessageBox, QVBoxLayout, QWidget
from qtpy.QtGui import QDoubleValidator, QDesktopServices
from qtpy.QtCore import QUrl, QLocale

import mantid
import mantid.simpleapi as api
import mantid.kernel
from mantid.kernel import Logger
from mantid.kernel import ConfigService
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from mantid.simpleapi import AnalysisDataService
from mantidqt.plotting.markers import RangeMarker
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


class DoubleValidator(QDoubleValidator):
    def __init__(self, line_edit, initial_value, allow_empty_edits=False):
        locale = QLocale.c()
        locale.setNumberOptions(QLocale.RejectGroupSeparator)
        super().__init__()
        super().setLocale(locale)
        super().setDecimals(6)
        self._line_edit = line_edit
        self._line_edit.editingFinished.connect(self.on_editing_finished)
        self._last_value = None if not initial_value else initial_value
        self._allow_empty = allow_empty_edits

    @property
    def last_value(self):
        return self._last_value

    @last_value.setter
    def last_value(self, value):
        self._last_value = value

    def on_editing_finished(self):
        txt = self._line_edit.text()
        self._last_value = float(txt) if txt else None
        self.set_txt()

    def set_txt(self):
        self._line_edit.setText(f"{self._last_value:.6f}" if self._last_value else "")

    def fixup(self, txt):
        if txt == "" and self._allow_empty:
            self._line_edit.clear()
        else:
            self.set_txt()


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
        self.ui = load_ui(__file__, "eventFilterGUI.ui", baseinstance=self)
        mpl_layout = QVBoxLayout()
        self.ui.graphicsView.setLayout(mpl_layout)
        self.fig = Figure(figsize=(4, 3), layout="constrained")
        self.canvas = FigureCanvas(self.fig)
        self.ui.mainplot = self.fig.add_subplot(111, projection="mantid", xlabel="x-units", ylabel="y-units")
        mpl_layout.addWidget(self.canvas)

        self.canvas.mpl_connect("button_press_event", self.on_mouse_press)
        self.canvas.mpl_connect("draw_event", self.draw_callback)
        self.canvas.mpl_connect("button_release_event", self.on_mouse_release)
        self.canvas.mpl_connect("motion_notify_event", self.on_mouse_move)

        initial_plot_lims = [0.0, 1.0]
        initial_marker_pos = [0.1, 0.9]
        vertical_marker = RangeMarker(self.canvas, "green", *initial_plot_lims, line_style="--")
        horizontal_marker = RangeMarker(self.canvas, "blue", *initial_plot_lims, range_type="YMinMax", line_style="-.")

        # Do initialize plotting
        self.mainline = self.ui.mainplot.plot(initial_plot_lims, [0.5] * 2, "r-")
        self.ui.mainplot.set_xlim(*initial_plot_lims)
        self.ui.mainplot.set_ylim(*initial_plot_lims)

        # Set up validators for numeric edits
        self.doubleLineEdits = [
            (self.ui.leStartTime, initial_marker_pos[0]),
            (self.ui.leStopTime, initial_marker_pos[1]),
            (self.ui.leMinimumValue, initial_marker_pos[0]),
            (self.ui.leMaximumValue, initial_marker_pos[1]),
            (self.ui.leStepSize, None),
            (self.ui.leValueTolerance, None),
            (self.ui.leTimeTolerance, None),
            (self.ui.leIncidentEnergy, None),
        ]
        for line_edit, ini_value in self.doubleLineEdits:
            line_edit.setValidator(DoubleValidator(line_edit, ini_value, ini_value is None))
            if ini_value:
                line_edit.setText(f"{ini_value:.6f}")
                line_edit.editingFinished.connect(self.update_marker_range)

        self.markers = [vertical_marker, horizontal_marker]
        self.marker_line_edits = [(self.ui.leStartTime, self.ui.leStopTime), (self.ui.leMinimumValue, self.ui.leMaximumValue)]
        for (min_edit, max_edit), marker in zip(self.marker_line_edits, self.markers):
            marker.set_range(*initial_marker_pos)
            self.update_line_edits(min_edit, max_edit, initial_marker_pos, marker)

        # File loader
        self.scanEventWorkspaces()
        self.ui.pushButton_refreshWS.clicked.connect(self.scanEventWorkspaces)
        self.ui.pushButton_browse.clicked.connect(self.browse_File)
        self.ui.pushButton_load.clicked.connect(self.load_File)
        self.ui.pushButton_3.clicked.connect(self.use_existWS)

        # Filter by time
        self.ui.pushButton_filterTime.clicked.connect(self.filterByTime)
        self.ui.lineEdit_timeInterval.returnPressed.connect(self.filterByTime)

        self.ui.comboBox_4.addItems(["Both", "Increase", "Decrease"])
        self.ui.comboBox_5.addItems(["Centre", "Left"])

        self.ui.pushButton_4.clicked.connect(self.plotLogValue)
        self.ui.pushButton_filterLog.clicked.connect(self.filterByLogValue)

        # Set up help button
        self.ui.helpBtn.clicked.connect(self.helpClicked)

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

    def draw_callback(self, _):
        for marker in self.markers:
            marker.redraw()

    def on_mouse_press(self, event):
        """Respond to pick up a value with mouse down event"""
        x, y = event.xdata, event.ydata
        for marker in self.markers:
            marker.mouse_move_start(x, y)

    def on_mouse_move(self, event):
        x, y = event.xdata, event.ydata
        if x and y:
            self.ui.label_xy.setText(f"(x,y)=({x:.3f},{y:.3f})")
        for marker, (min_edit, max_edit) in zip(self.markers, self.marker_line_edits):
            if marker.mouse_move(x, y):
                val_min, val_max = marker.get_range()
                min_edit.setText(f"{val_min:.6f}")
                max_edit.setText(f"{val_max:.6f}")
        self.canvas.draw_idle()

    def on_mouse_release(self, _):
        """Respond to release mouse event"""
        for marker, line_edits, lim_func in zip(
            self.markers, self.marker_line_edits, [self.ui.mainplot.get_xlim, self.ui.mainplot.get_ylim]
        ):
            if marker.is_marker_moving():
                marker.mouse_move_stop()
                self.update_line_edits(*line_edits, lim_func(), marker)

    def update_marker_range(self):
        """Update marker range based on new values on connected line edits"""
        try:
            idx = int(self.sender().objectName() in ["leMinimumValue", "leMaximumValue"])
            marker = self.markers[idx]
            min_edit, max_edit = self.marker_line_edits[idx]

            start = float(min_edit.text())
            stop = float(max_edit.text())

            # Set value to valid range
            marker.set_range(start, stop)
            self.canvas.draw_idle()
        except ValueError:
            Logger("Filter_Events").information("Incorrect value in start or stop time")

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
        samunit = samplelog.units
        self.update_plot(vecreltimes, vecvalue, y_label=logname if len(samunit) == 0 else "%s (%s)" % (logname, samunit))

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
                    failure! \n{}\n".format(dataws, errmsg)
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

        # if there is only one xbin in the summed workspace, that means we have an event file without pulse,
        # and in this case we use the original workspace time limits
        xlim = [min(wksp.readX(0) / 1000000), max(wksp.readX(0) / 1000000)] if len(vecx) == 1 else [min(vecx), max(vecx)]
        self.update_plot(vecx, vecy, "Time(s)", "Counts", xlim)

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
            url = "http://docs.mantidproject.org/nightly/interfaces/{}.html".format("Filter Events")
            QDesktopServices.openUrl(QUrl(url))

    def _resetGUI(self, resetfilerun=False):
        """Reset GUI elements."""
        if resetfilerun is True:
            self.ui.lineEdit.clear()

        # Plot related
        self.ui.lineEdit_outwsname.clear()
        self.ui.lineEdit_title.clear()
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

        self.canvas.draw()

    def update_line_edits(self, min_edit, max_edit, lims, marker):
        min_edit.validator().setRange(lims[0], marker.get_maximum(), 6)
        min_marker = marker.get_minimum()
        min_edit.validator().last_value = min_marker
        min_edit.setText(f"{min_marker:.6f}")

        max_edit.validator().setRange(marker.get_minimum(), lims[1], 6)
        max_marker = marker.get_maximum()
        max_edit.validator().last_value = max_marker
        max_edit.setText(f"{max_marker:.6f}")

    def update_plot(self, x_data, y_data, x_label="Time(s)", y_label="Counts", xlim=None, ylim=None):
        """Updates the plot line, the marker positions and the edits linked to the markers"""

        def calculate_limit(data):
            min_lim, max_lim = min(data), max(data)
            if (abs(max_lim - min_lim)) < 1e-4:  # to avoid matplotlib warning and for visibility
                min_lim = min_lim - 0.001 * min_lim
                max_lim = max_lim + 0.001 * max_lim
            return [min_lim, max_lim]

        xlim = calculate_limit(x_data) if not xlim else xlim
        ylim = calculate_limit(y_data) if not ylim else ylim

        setp(self.mainline, xdata=x_data, ydata=y_data)
        self.ui.mainplot.set_xlim(*xlim)
        self.ui.mainplot.set_ylim(*ylim)
        self.ui.mainplot.set_ylabel(y_label, fontsize=10)
        self.ui.mainplot.set_xlabel(x_label, fontsize=10)
        for marker, edits, lims in zip(self.markers, self.marker_line_edits, [xlim, ylim]):
            min_marker_pos = lims[0] + 0.1 * (abs(lims[1] - lims[0]))
            max_marker_pos = lims[1] - 0.1 * (abs(lims[1] - lims[0]))
            marker.set_bounds(lims[0], lims[1])
            marker.set_range(min_marker_pos, max_marker_pos)
            self.update_line_edits(*edits, lims, marker)

        self.canvas.draw()
