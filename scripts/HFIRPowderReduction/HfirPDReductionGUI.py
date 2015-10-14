#pylint: disable=invalid-name, relative-import, too-many-lines,too-many-instance-attributes,too-many-arguments
################################################################################
# Main class for HFIR powder reduction GUI
# Key word for future developing: FUTURE, NEXT, REFACTOR, RELEASE 2.0
################################################################################

import numpy
import os

from ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

import mantid
from HfirPDReductionControl import *

#----- default configuration ---------------
DEFAULT_SERVER = 'http://neutron.ornl.gov/user_data'
DEFAULT_INSTRUMENT = 'hb2a'
DEFAULT_WAVELENGTH = 2.4100
#-------------------------------------------

class EmptyError(Exception):
    """ Exception for finding empty input for integer or float
    """
    def __init__(self, value):
        """ Init
        """
        Exception.__init__(self)
        self.value = value

    def __str__(self):
        return repr(self.value)


class MultiScanTabState(object):
    """ Description of the state of the multi-scan-tab is in
    """
    NO_OPERATION = 0
    RELOAD_DATA = 1
    REDUCE_DATA = 2

    def __init__(self):
        """ Initialization
        :return:
        """
        self._expNo = -1
        self._scanList = []
        self._xMin = None
        self._xMax = None
        self._binSize = 0
        self._unit = ''
        self._plotRaw = False
        self._useDetEfficiencyCorrection = False
        self._excludeDetectors = []

    def compare_state(self, tab_state):
        """ Compare this tab state and another tab state
        :param tab_state:
        :return:
        """
        if isinstance(tab_state, MultiScanTabState) is False:
            raise NotImplementedError('compare_state must have MultiScanTabStatus as input.')

        if self._expNo != tab_state.getExpNumber() or self._scanList != tab_state.getScanList:
            return self.RELOAD_DATA

        for attname in self.__dict__.keys():
            if self.__getattribute__(attname) != tab_state.__getattribute__(attname):
                return self.REDUCE_DATA

        return self.NO_OPERATION

    def getExpNumber(self):
        """ Get experiment number
        :return:
        """
        return self._expNo

    def getScanList(self):
        """ Get the list of scans
        :return:
        """
        return self._scanList[:]

    #pyline: disable=too-many-arguments
    def setup(self, exp_no, scan_list, min_x, max_x, bin_size, unit, raw, correct_det_eff, exclude_dets):
        """
        Set up the object
        :param exp_no:
        :param scan_list:
        :param min_x:
        :param max_x:
        :param bin_size:
        :param unit:
        :param raw:
        :param correct_det_eff:
        :param exclude_dets:
        :return:
        """
        self._expNo = int(exp_no)
        if isinstance(scan_list, list) is False:
            raise NotImplementedError('Scan_List must be list!')
        self._scanList = scan_list
        self._xMin = min_x
        self._xMax = max_x
        self._binSize = float(bin_size)
        self._unit = str(unit)
        self._plotRaw = raw
        self._useDetEfficiencyCorrection = correct_det_eff
        self._excludeDetectors = exclude_dets

        return


#pylint: disable=too-many-public-methods,too-many-branches,too-many-locals,too-many-statements
class MainWindow(QtGui.QMainWindow):
    """ Class of Main Window (top)
    """

    # Copy to ui.setupUI
    # # Version 3.0 + Import for ui_MainWindow.py
    #     from MplFigureCanvas import Qt4MplCanvas

    #     # Replace 'self.graphicsView = QtGui.QtGraphicsView' with the following
    #     self.graphicsView = Qt4MplCanvas(self.centralwidget)
    #     self.mainplot = self.graphicsView.getPlot()

    def __init__(self, parent=None):
        """ Initialization and set up
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # UI Window (from Qt Designer)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Define gui-event handling

        # menu
        self.connect(self.ui.actionQuit, QtCore.SIGNAL('triggered()'),
                     self.doExist)
        self.connect(self.ui.actionFind_Help, QtCore.SIGNAL('triggered()'),
                self.doHelp)

        # main
        self.connect(self.ui.comboBox_wavelength, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.doUpdateWavelength)
        self.connect(self.ui.pushButton_browseExcludedDetFile, QtCore.SIGNAL('clicked()'),
                     self.doBrowseExcludedDetetorFile)
        self.connect(self.ui.checkBox_useDetExcludeFile, QtCore.SIGNAL('stateChanged(int)'),
                     self.do_enable_excluded_dets)

        # tab 'Raw Detectors'
        self.connect(self.ui.pushButton_plotRaw, QtCore.SIGNAL('clicked()'),
                     self.doPlotRawPtMain)
        self.connect(self.ui.pushButton_ptUp, QtCore.SIGNAL('clicked()'),
                     self.do_plot_raw_pt_prev)
        self.connect(self.ui.pushButton_ptDown, QtCore.SIGNAL('clicked()'),
                     self.doPlotRawPtNext)
        self.connect(self.ui.pushButton_clearRawDets, QtCore.SIGNAL('clicked()'),
                     self.doClearRawDetCanvas)

        # tab 'Individual Detectors'
        self.connect(self.ui.pushButton_plotIndvDet, QtCore.SIGNAL('clicked()'),
                     self.doPlotIndvDetMain)
        self.connect(self.ui.pushButton_plotPrevDet, QtCore.SIGNAL('clicked()'),
                     self.doPlotIndvDetPrev)
        self.connect(self.ui.pushButton_plotNextDet, QtCore.SIGNAL('clicked()'),
                     self.doPlotIndvDetNext)
        self.connect(self.ui.pushButton_clearCanvasIndDet, QtCore.SIGNAL('clicked()'),
                     self.doClearIndDetCanvas)
        self.connect(self.ui.pushButton_plotLog , QtCore.SIGNAL('clicked()'),
                     self.do_plot_sample_log)

        # tab 'Normalized'
        self.connect(self.ui.pushButton_loadData, QtCore.SIGNAL('clicked()'),
                self.doLoadData)
        self.connect(self.ui.pushButton_prevScan, QtCore.SIGNAL('clicked()'),
                self.doLoadReduceScanPrev)
        self.connect(self.ui.pushButton_nextScan, QtCore.SIGNAL('clicked()'),
                self.doLoadReduceScanNext)
        self.connect(self.ui.pushButton_unit2theta, QtCore.SIGNAL('clicked()'),
                self.doReduce2Theta)
        self.connect(self.ui.pushButton_unitD, QtCore.SIGNAL('clicked()'),
                self.doReduceDSpacing)
        self.connect(self.ui.pushButton_unitQ, QtCore.SIGNAL('clicked()'),
                self.doReduceQ)
        self.connect(self.ui.pushButton_saveData, QtCore.SIGNAL('clicked()'),
                self.doSaveData)
        self.connect(self.ui.pushButton_clearTab2Canvas, QtCore.SIGNAL('clicked()'),
                self.doClearCanvas)

        # tab 'Multiple Scans'
        self.connect(self.ui.pushButton_loadMultData, QtCore.SIGNAL('clicked()'),
                     self.doLoadSetData)
        self.connect(self.ui.pushButton_mscanBin, QtCore.SIGNAL('clicked()'),
                self.doReduceSetData)
        self.connect(self.ui.pushButton_mergeScans, QtCore.SIGNAL('clicked()'),
                self.doMergeScans)
        self.connect(self.ui.pushButton_viewMScan1D, QtCore.SIGNAL('clicked()'),
                self.doMergeScanView1D)
        self.connect(self.ui.pushButton_view2D, QtCore.SIGNAL('clicked()'),
                self.doMergeScanView2D)
        self.connect(self.ui.pushButton_viewMerge, QtCore.SIGNAL('clicked()'),
                self.doMergeScanViewMerged)
        self.connect(self.ui.pushButton_clearMultCanvas, QtCore.SIGNAL('clicked()'),
                self.doClearMultiRunCanvas)
        self.connect(self.ui.pushButton_saveAllIndScans, QtCore.SIGNAL('clicked()'),
                self.doSaveMultipleScans)
        self.connect(self.ui.pushButton_saveMerge, QtCore.SIGNAL('clicked()'),
                self.doSaveMergedScan)
        self.connect(self.ui.pushButton_plotRawMultiScans, QtCore.SIGNAL('clicked()'),
                     self.do_convert_plot_multi_scans)

        # tab 'Vanadium'
        self.connect(self.ui.pushButton_stripVanPeaks, QtCore.SIGNAL('clicked()'),
                self.doStripVandiumPeaks)
        self.connect(self.ui.pushButton_saveVanRun, QtCore.SIGNAL('clicked()'),
                self.doSaveVanRun)
        self.connect(self.ui.pushButton_rebin2Theta, QtCore.SIGNAL('clicked()'),
                self.doReduceVanadium2Theta)
        self.connect(self.ui.pushButton_smoothVanData, QtCore.SIGNAL('clicked()'),
                self.doSmoothVanadiumData)
        self.connect(self.ui.pushButton_applySmooth, QtCore.SIGNAL('clicked()'),
                self.doSmoothVanadiumApply)
        self.connect(self.ui.pushButton_undoSmooth, QtCore.SIGNAL('clicked()'),
                self.doSmoothVanadiumUndo)

        # tab 'Advanced Setup'
        self.connect(self.ui.pushButton_browseCache, QtCore.SIGNAL('clicked()'),
                self.doBrowseCache)
        self.connect(self.ui.radioButton_useServer, QtCore.SIGNAL('clicked()'),
                self.doChangeSrcLocation)
        self.connect(self.ui.radioButton_useLocal, QtCore.SIGNAL('clicked()'),
                self.doChangeSrcLocation)
        self.connect(self.ui.pushButton_browseLocalSrc, QtCore.SIGNAL('clicked()'),
                self.doBrowseLocalDataSrc)
        self.connect(self.ui.pushButton_chkServer, QtCore.SIGNAL('clicked()'),
                self.doCheckSrcServer)

        # Define signal-event handling

        # define event handlers for matplotlib canvas
        self.ui.graphicsView_mergeRun.canvas.mpl_connect('button_press_event', \
                self.on_mouseDownEvent)
        self.ui.graphicsView_mergeRun.canvas.mpl_connect('motion_notify_event', \
                self.on_mouseMotion)

        # Widget type definition
        validator0 = QtGui.QIntValidator(self.ui.lineEdit_expNo)
        validator0.setBottom(1)
        self.ui.lineEdit_expNo.setValidator(validator0)

        validator1 = QtGui.QIntValidator(self.ui.lineEdit_expNo)
        validator1.setBottom(1)
        self.ui.lineEdit_scanNo.setValidator(validator1)

        validator2 = QtGui.QDoubleValidator(self.ui.lineEdit_wavelength)
        validator2.setBottom(0.)
        self.ui.lineEdit_wavelength.setValidator(validator2)

        validator3 = QtGui.QDoubleValidator(self.ui.lineEdit_xmin)
        validator3.setBottom(0.)
        self.ui.lineEdit_xmin.setValidator(validator3)

        validator4 = QtGui.QDoubleValidator(self.ui.lineEdit_xmax)
        validator4.setBottom(0.)
        self.ui.lineEdit_xmax.setValidator(validator4)

        validator5 = QtGui.QDoubleValidator(self.ui.lineEdit_binsize)
        validator5.setBottom(0.)
        self.ui.lineEdit_binsize.setValidator(validator5)

        validator6 = QtGui.QDoubleValidator(self.ui.lineEdit_ptNo)
        validator6.setBottom(0)
        self.ui.lineEdit_ptNo.setValidator(validator6)

        validator7 = QtGui.QDoubleValidator(self.ui.lineEdit_detID)
        validator7.setBottom(0)
        self.ui.lineEdit_detID.setValidator(validator7)

        validator8 = QtGui.QDoubleValidator(self.ui.lineEdit_min2Theta)
        validator8.setBottom(0.)
        self.ui.lineEdit_min2Theta.setValidator(validator8)

        validator9 = QtGui.QDoubleValidator(self.ui.lineEdit_max2Theta)
        validator9.setBottom(0.)
        self.ui.lineEdit_max2Theta.setValidator(validator9)

        validator10 = QtGui.QDoubleValidator(self.ui.lineEdit_binsize2Theta)
        validator10.setBottom(0.)
        self.ui.lineEdit_binsize2Theta.setValidator(validator10)

        validator11 = QtGui.QIntValidator(self.ui.lineEdit_scanStart)
        validator11.setBottom(1)
        self.ui.lineEdit_scanStart.setValidator(validator11)

        validator12 = QtGui.QIntValidator(self.ui.lineEdit_scanEnd)
        validator12.setBottom(1)
        self.ui.lineEdit_scanEnd.setValidator(validator12)

        validator13 = QtGui.QDoubleValidator(self.ui.lineEdit_normalizeMonitor)
        validator13.setBottom(0.)
        self.ui.lineEdit_normalizeMonitor.setValidator(validator13)

        validator14 = QtGui.QDoubleValidator(self.ui.lineEdit_mergeMinX)
        validator14.setBottom(0.)
        self.ui.lineEdit_mergeMinX.setValidator(validator14)

        validator15 = QtGui.QDoubleValidator(self.ui.lineEdit_mergeMaxX)
        validator15.setBottom(0.)
        self.ui.lineEdit_mergeMaxX.setValidator(validator15)

        validator16 = QtGui.QDoubleValidator(self.ui.lineEdit_mergeBinSize)
        validator16.setBottom(0.)
        self.ui.lineEdit_mergeBinSize.setValidator(validator16)

        # Get initial setup
        # RELEASE 2.0 - This part will be implemented soon as default configuration is made
        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())

        # UI widgets setup
        self.ui.comboBox_outputFormat.addItems(['Fullprof']) # Supports Fullprof only now, 'GSAS', 'Fullprof+GSAS'])

        # RELEASE 2.0 : Need to disable some widgets... consider to refactor the code
        self.ui.radioButton_useServer.setChecked(True)
        self.ui.radioButton_useLocal.setChecked(False)
        self.ui.checkBox_useDetExcludeFile.setChecked(True)

        self.ui.comboBox_wavelength.setCurrentIndex(0)
        self.ui.lineEdit_wavelength.setText('2.41')

        self.ui.pushButton_unit2theta.setText(r'$2\theta$')

        # vanadium spectrum smooth parameters
        self.ui.lineEdit_smoothParams.setText('20,2')

        # Set up data source
        self._serverAddress = DEFAULT_SERVER
        self._srcFromServer = True
        self._localSrcDataDir = None
        self._srcAtLocal = False

        self._currUnit = '2theta'

        # Workspaces
        self._myControl = HFIRPDRedControl()

        # Interactive graphics
        self._viewMerge_X = None
        self._viewMerge_Y = None

        # Control of plots: key = canvas, value = list of 2-integer-tuple (expno, scanno)
        self._tabLineDict = {}
        self._tabBinParamDict = {}
        for key in [2]:
            self._tabLineDict[key] = []
        for key in [2, 3, 4]:
            self._tabBinParamDict[key] = [None, None, None]

        self._lastMergeLabel = ""
        self._lastMergeIndex = -1

        self._expNo = None
        self._scanNo = None
        self._detID = None
        self._indvXLabel = None

        self._rawDetExpNo = None
        self._rawDetScanNo = None
        self._rawDetPlotMode = None
        self._rawDetPtNo = None

        self._indvDetCanvasMode = 'samplelog'

        # Multiple scan tab
        self._multiScanExp = None
        self._multiScanList = []

        #help
        self.assistantProcess = QtCore.QProcess(self)
        # pylint: disable=protected-access
        self.collectionFile=os.path.join(mantid._bindir,'../docs/qthelp/MantidProject.qhc')
        version = ".".join(mantid.__version__.split(".")[:2])
        self.qtUrl='qthelp://org.sphinx.mantidproject.'+version+'/doc/interfaces/HFIRPowderReduction.html'
        self.externalUrl='http://docs.mantidproject.org/nightly/interfaces/HFIRPowderReduction.html'

        # Initial setup for tab
        self.ui.tabWidget.setCurrentIndex(0)
        cache_dir = str(self.ui.lineEdit_cache.text()).strip()
        if len(cache_dir) == 0 or os.path.exists(cache_dir) is False:
            invalid_cache = cache_dir
            if False:
                cache_dir = os.path.expanduser('~')
            else:
                cache_dir = os.getcwd()
            self.ui.lineEdit_cache.setText(cache_dir)
            self._logWarning("Cache directory %s is not valid. "
                             "Using current workspace directory %s as cache." %
                             (invalid_cache, cache_dir))

        # Get on hold of raw data file
        useserver = self.ui.radioButton_useServer.isChecked()
        uselocal = self.ui.radioButton_useLocal.isChecked()
        if useserver == uselocal:
            self._logWarning("It is logically wrong to set up (1) neither server or local dir to "
                             "access data or (2) both server and local dir to retrieve data. "
                             "As default, it is set up to download data from server.")
            useserver = True
            uselocal = False
            self.ui.radioButton_useServer.setChecked(True)
            self.ui.radioButton_useLocal.setChecked(False)
        # ENDIF

        return


    #-- Event Handling ----------------------------------------------------

    def doBrowseCache(self):
        """ Pop out a dialog to let user specify the directory to
        cache downloaded data
        """
        # home directory
        homedir = str(self.ui.lineEdit_cache.text()).strip()
        if len(homedir) > 0 and os.path.exists(homedir):
            home = homedir
        else:
            home = os.getcwd()

        # pop out a dialog
        dirs = str(QtGui.QFileDialog.getExistingDirectory(self,'Get Directory',home))

        # set to line edit
        if dirs != home:
            self.ui.lineEdit_cache.setText(dirs)

        return

    def doBrowseExcludedDetetorFile(self):
        """ Browse excluded detector's file
        Return :: None
        """
        # Get file name
        filefilter = "Text (*.txt);;Data (*.dat);;All files (*.*)"
        curDir = os.getcwd()
        excldetfnames = QtGui.QFileDialog.getOpenFileNames(self, 'Open File(s)', curDir, filefilter)
        try:
            excldetfname = excldetfnames[0]
            self.ui.lineEdit_excludedDetFileName.setText(excldetfname)
        except IndexError:
            # return if there is no file selected
            return

        # Parse det exclusion file
        print "Detector exclusion file name is %s." % (excldetfname)
        excludedetlist, errmsg = self._myControl.parseExcludedDetFile('HB2A', excldetfname)
        if len(errmsg) > 0:
            self._logError(errmsg)
        textbuf = ""
        for detid in excludedetlist:
            textbuf += "%d," % (detid)
        if len(textbuf) > 0:
            textbuf = textbuf[:-1]
            self.ui.lineEdit_detExcluded.setText(textbuf)
        # ENDIF

        return

    def doBrowseLocalDataSrc(self):
        """ Browse local data storage
        """
        msg = "Browse local data storage location. Implement ASAP"
        QtGui.QMessageBox.information(self, "Click!", msg)
        return


    def doChangeSrcLocation(self):
        """ Source file location is changed
        """
        useserver = self.ui.radioButton_useServer.isChecked()
        uselocal = self.ui.radioButton_useLocal.isChecked()

        print "Use Server: ", useserver
        print "Use Local : ", uselocal

        if (useserver is True and uselocal is True) or \
            (useserver is False and uselocal is False):
            raise NotImplementedError("Impossible for radio buttons")

        self._srcAtLocal = uselocal
        self._srcFromServer = useserver

        if uselocal is True:
            self.ui.lineEdit_dataIP.setDisabled(True)
            self.ui.pushButton_chkServer.setDisabled(True)
            self.ui.lineEdit_localSrc.setDisabled(False)
            self.ui.pushButton_browseLocalSrc.setDisabled(False)

        else:
            self.ui.lineEdit_dataIP.setDisabled(False)
            self.ui.pushButton_chkServer.setDisabled(False)
            self.ui.lineEdit_localSrc.setDisabled(True)
            self.ui.pushButton_browseLocalSrc.setDisabled(True)

        return


    def doCheckSrcServer(self):
        """" Check source data server's availability
        """
        msg = "Check source data server! Implement ASAP"
        QtGui.QMessageBox.information(self, "Click!", msg)

        return

    def doClearCanvas(self):
        """ Clear canvas
        """
        itab = self.ui.tabWidget.currentIndex()
        if itab == 2:
            self.ui.graphicsView_reducedData.clearAllLines()
            self._tabLineDict[itab] = []

        return

    def doClearIndDetCanvas(self):
        """ Clear the canvas in tab 'Individual Detector' and current plotted lines
        in managing dictionary
        """
        # Clear all lines on canvas
        self.ui.graphicsView_indvDet.clearAllLines()
        # Remove their references in dictionary
        if self._tabLineDict.has_key(self.ui.graphicsView_indvDet):
            self._tabLineDict[self.ui.graphicsView_indvDet] = []
        # Reset colur schedule
        self.ui.graphicsView_indvDet.resetLineColorStyle()

        return


    def doClearMultiRunCanvas(self):
        """ Clear the canvas in tab 'Multiple Run'

        This canvas is applied to both 1D and 2D image.
        Clear-all-lines might be not enough to clear 2D image
        """
        self.ui.graphicsView_mergeRun.clearCanvas()

        return


    def doClearRawDetCanvas(self):
        """ Clear the canvas in tab 'Raw Detector':
        only need to clear lines
        """
        self.ui.graphicsView_Raw.clearAllLines()
        self._tabLineDict[self.ui.graphicsView_Raw] = []

        return


    def doClearVanadiumCanvas(self):
        """ Clear the canvas in tab 'Vanadium'
        """
        self.ui.graphicsView_vanPeaks.clearAllLines()

        return


    def doExist(self):
        """ Exist the application
        """
        clearcache = self.ui.checkBox_delCache.isChecked()

        if clearcache is True:
            delAllFile(self._cache)

        self.close()

        return

    def doHelp(self):
        """ Show help
        Copied from DGSPlanner
        """
        self.assistantProcess.close()
        self.assistantProcess.waitForFinished()
        helpapp = QtCore.QLibraryInfo.location(QtCore.QLibraryInfo.BinariesPath) + QtCore.QDir.separator()
        helpapp += 'assistant'
        args = ['-enableRemoteControl', '-collectionFile',self.collectionFile,'-showUrl',self.qtUrl]
        if os.path.isfile(helpapp):
            self.assistantProcess.close()
            self.assistantProcess.waitForFinished()
            self.assistantProcess.start(helpapp, args)
            print "Show help from (app) ", helpapp
        else:
            QtGui.QDesktopServices.openUrl(QtCore.QUrl(self.externalUrl))
            print "Show help from (url)", QtCore.QUrl(self.externalUrl)

        return

    def doLoadData(self, exp=None, scan=None):
        """ Load and reduce data
        It does not support for tab 'Advanced Setup'
        For tab 'Raw Detector' and 'Individual Detector', this method will load data to MDEventWorkspaces
        For tab 'Normalized' and 'Vanadium', this method will load data to MDEVentWorkspaces but NOT reduce to single spectrum
        """
        # Kick away unsupported tabs
        itab = self.ui.tabWidget.currentIndex()
        tabtext = str(self.ui.tabWidget.tabText(itab))
        print "[DB] Current active tab is No. %d as %s." % (itab, tabtext)

        # Rule out unsupported tab
        if itab == 5:
            # 'advanced'
            msg = "Tab %s does not support 'Load Data'. Request is ambiguous." % tabtext
            QtGui.QMessageBox.information(self, "Click!", msg)
            return

        # Get exp number and scan number
        if isinstance(exp, int) is True and isinstance(scan, int) is True:
            # use input
            expno = exp
            scanno = scan
        else:
            # read from GUI
            try:
                expno, scanno = self._uiGetExpScanNumber()
                self._logDebug("Attending to load Exp %d Scan %d." % (expno, scanno))
            except NotImplementedError as ne:
                self._logError("Error to get Exp and Scan due to %s." % (str(ne)))
                return
        # ENDIF

        # Form data file name and download data
        status, datafilename = self._uiDownloadDataFile(exp=expno, scan=scanno)
        if status is False:
            self._logError("Unable to download or locate local data file for Exp %d \
                Scan %d." % (expno, scanno))
        # ENDIF(status)

        # (Load data for tab 0, 1, 2 and 4)
        if itab not in [0, 1, 2, 3, 4]:
            # Unsupported Tabs: programming error!
            errmsg = "%d-th tab should not get this far.\n"%(itab)
            errmsg += 'GUI has been changed, but the change has not been considered! iTab = %d' % (itab)
            raise NotImplementedError(errmsg)

        # Load SPICE data to raw table (step 1)
        try:
            execstatus = self._myControl.loadSpicePDData(expno, scanno, datafilename)
            if execstatus is False:
                cause = "Load data failed."
            else:
                cause = None
        except NotImplementedError as ne:
            execstatus = False
            cause = str(ne)
        # END-TRY-EXCEPT

        # Return as failed to load data
        if execstatus is False:
            self._logError(cause)
            return

        # Obtain the correction file names and wavelength from SPICE file
        wavelengtherror = False
        errmsg = ""
        localdir = os.path.dirname(datafilename)
        try:
            status, returnbody = self._myControl.retrieveCorrectionData(instrument='HB2A',
                                                                        exp=expno, scan=scanno,
                                                                        localdatadir=localdir)
        except NotImplementedError as e:
            errmsg = str(e)
            if errmsg.count('m1') > 0:
                # error is about wavelength
                status = False
                wavelengtherror = True
            else:
                # other error
                raise e
        # ENDTRY

        if status is True:
            autowavelength = returnbody[0]
            vancorrfname = returnbody[1]
            excldetfname = returnbody[2]

            if vancorrfname is not None:
                self.ui.lineEdit_vcorrFileName.setText(vancorrfname)
            if excldetfname is not None:
                self.ui.lineEdit_excludedDetFileName.setText(excldetfname)
        else:
            autowavelength = None
            vancorrfname = None
            excldetfname = None
        # ENDIF

        # Set wavelength to GUI except 'multiple scans'
        if autowavelength is None:
            # unable to get wavelength from SPICE data
            self.ui.comboBox_wavelength.setCurrentIndex(4)
            if wavelengtherror is True:
                self.ui.lineEdit_wavelength.setText(errmsg)
            else:
                self.ui.lineEdit_wavelength.setText(self.ui.comboBox_wavelength.currentText())
            self._myControl.setWavelength(expno, scanno, wavelength=None)
        else:
            # get wavelength from SPICE data.  set value to GUI
            self.ui.lineEdit_wavelength.setText(str(autowavelength))
            allowedwavelengths = [2.41, 1.54, 1.12]
            numitems = self.ui.comboBox_wavelength.count()
            good = False
            for ic in xrange(numitems-1):
                if abs(autowavelength - allowedwavelengths[ic]) < 0.01:
                    good = True
                    self.ui.comboBox_wavelength.setCurrentIndex(ic)
            # ENDFOR

            if good is False:
                self.ui.comboBox_wavelength.setCurrentIndex(numitems-1)
            # ENDIF

            self._myControl.setWavelength(expno, scanno, wavelength=autowavelength)
        # ENDIFELSE

        # Optionally obtain and parse det effecient file
        if self.ui.checkBox_useDetEffCorr.isChecked() is True:
            # Apply detector efficiency correction
            if vancorrfname is None:
                # browse vanadium correction file
                filefilter = "Text (*.txt);;Data (*.dat);;All files (*.*)"
                curDir = os.getcwd()
                vancorrfnames = QtGui.QFileDialog.getOpenFileNames(self, 'Open File(s)', curDir, filefilter)
                if len(vancorrfnames) > 0:
                    vancorrfname = vancorrfnames[0]
                    self.ui.lineEdit_vcorrFileName.setText(str(vancorrfname))
                else:
                    self._logError("User does not specify any vanadium correction file.")
                    self.ui.checkBox_useDetEffCorr.setChecked(False)
                # ENDIF-len()
            # ENDIF vancorrfname

            # Parse if it is not None
            if vancorrfname is not None:
                detefftablews, errmsg = self._myControl.parseDetEffCorrFile('HB2A', vancorrfname)
                if detefftablews is None:
                    print "Parsing detectors efficiency file error: %s." % (errmsg)
            else:
                detefftablews = None
            # ENDIF

        else:
            # Not chosen to apply detector efficiency correction:w
            detefftablews = None
        # ENDIF

        # Parse SPICE data to MDEventWorkspaces
        try:
            print "Det Efficiency Table WS: ", str(detefftablews)
            execstatus = self._myControl.parseSpiceData(expno, scanno, detefftablews)
            if execstatus is False:
                cause = "Parse data failed."
            else:
                cause = None
        except NotImplementedError as e:
            execstatus = False
            cause = str(e)
        # END-TRY-EXCEPT-FINALLY

        # Return if data parsing is error
        if execstatus is False:
            self._logError(cause)
            return

        # Optionally parse detector exclusion file and set to line text
        if excldetfname is not None:
            excludedetlist, errmsg = self._myControl.parseExcludedDetFile('HB2A', excldetfname)

            textbuf = ""
            for detid in excludedetlist:
                textbuf += "%d," % (detid)
            if len(textbuf) > 0:
                textbuf = textbuf[:-1]
                self.ui.lineEdit_detExcluded.setText(textbuf)
        # ENDIF

        # Set up some widgets for raw detector data.  Won't be applied to tab 3
        if itab != 3:
            floatsamplelognamelist = self._myControl.getSampleLogNames(expno, scanno)
            self.ui.comboBox_indvDetXLabel.clear()
            self.ui.comboBox_indvDetXLabel.addItem("2theta/Scattering Angle")
            self.ui.comboBox_indvDetXLabel.addItems(floatsamplelognamelist)
            self.ui.comboBox_indvDetYLabel.clear()
            self.ui.comboBox_indvDetYLabel.addItems(floatsamplelognamelist)

        return True

    def doLoadSetData(self):
        """ Load a set of data
        This is the first step of doing multiple scans processing
        """
        # Get inputs for exp number and scans
        try:
            rtup = self._uiGetExpScanTabMultiScans()
            expno = rtup[0]
            scanlist = rtup[1]
        except NotImplementedError as nie:
            self._logError("Unable to load data set in multiple scans due to %s." % (str(nie)))

        # Load and reduce data
        loadstatus = True
        for scan in sorted(scanlist):
            tempstatus = self.doLoadData(expno, scan)
            if tempstatus is False:
                self.ui.label_mergeMessage.setText('Error to load Exp %d Scan %d.'%(expno, scan))
                loadstatus = False
            else:
                message = 'Loaded Exp %d Scan %d.' % (expno, scan)
                self.ui.label_mergeMessage.setText(message)
        # ENDFOR

        # Load status
        if loadstatus is True:
            self.ui.label_mergeMessage.setText('All data files are loaded')
        else:
            self.ui.label_mergeMessage.setText('Not all data files are loaded')

        # Wave length
        haswavelength = True
        for scan in scanlist:
            if self._myControl.getWavelength(expno, scan) is None:
                self._logNotice("Exp %d Scan %d has no wavelength set up." % (expno, scan))
                haswavelength = False
                break
        # ENDFOR

        # Set unit box
        if haswavelength is True:
            self.ui.comboBox_mscanUnit.clear()
            self.ui.comboBox_mscanUnit.addItems(['2theta', 'dSpacing', 'Momentum Transfer (Q)'])
        else:
            self.ui.comboBox_mscanUnit.clear()
            self.ui.comboBox_mscanUnit.addItems(['2theta'])

        return


    def doLoadReduceScanPrev(self):
        """ Load and reduce previous scan for tab 'Normalized'
        """
        # Reduce scan number by 1
        try:
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return
        else:
            scanno = scanno - 1
            if scanno < 1:
                self._logWarning("Scan number is 1 already.  Cannot have previous scan")
                return
            self.ui.lineEdit_scanNo.setText(str(scanno))

        # Load data
        self.ui.lineEdit_scanNo.setText(str(scanno))
        self.doLoadData()

        # Reduce data
        self._uiReducePlotNoramlized(self._currUnit)

        return


    def doLoadReduceScanNext(self):
        """ Load and reduce next scan for tab 'Normalized'
        """
        # Advance scan number by 1
        try:
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return False
        else:
            scanno = scanno + 1
            if scanno < 1:
                self._logWarning("Scan number is 1 already.  Cannot have previous scan")
                return False

        # Load data
        self.ui.lineEdit_scanNo.setText(str(scanno))
        execstatus = self.doLoadData()
        print "[DB] Load data : ", execstatus

        # Reduce data
        self._uiReducePlotNoramlized(self._currUnit)

        return


    def doMergeScans(self):
        """ Merge several scans for tab 'merge'
        """
        # Get exp number and list of scans
        try:
            r = self._uiGetExpScanTabMultiScans()
            expno = r[0]
            scanlist = r[1]
        except NotImplementedError as ne:
            self._logError(str(ne))
            return False

        # Check whether the wavelengths are same to merge
        try:
            wl_list = []
            for scanno in scanlist:
                print "Exp %d Scan %d. Wavelength = %s." % (expno, scanno, str(self._myControl.getWavelength(expno, scanno)))
                wl_list.append(float(self._myControl.getWavelength(expno, scanno)))

            wl_list = sorted(wl_list)
            min_wl = wl_list[0]
            max_wl = wl_list[-1]
            if max_wl - min_wl > 1.0:
                self._logWarning("All scans do not have same wavelengths!")
        except TypeError:
            self._logError('Not all scans have wavelength set up.  Unable to merge scans.')
            return

        # Check!
        try:
            unit = str(self.ui.comboBox_mscanUnit.currentText())
            xmin, binsize, xmax = self._uiGetBinningParams(itab=3)
            #wavelength = min_wl
            mindex = self._myControl.mergeReduceSpiceData(expno, scanlist, unit, xmin, xmax, binsize)
        except Exception as e:
            raise e

        label = "Exp %d, Scan %s." % (expno, str(scanlist))
        self._plotMergedReducedData(mindex, label)
        self._lastMergeIndex = mindex
        self._lastMergeLabel = label

        return


    def doMergeScanView1D(self):
        """ Change the multiple runs to 1D
        """
        # Highlight the button's color
        self.ui.pushButton_view2D.setStyleSheet('QPushButton {background-color: yellow; color: red;}')
        self.ui.pushButton_view2D.setEnabled(True)
        self.ui.pushButton_viewMScan1D.setStyleSheet('QPushButton {background-color: white; color: gray;}')
        self.ui.pushButton_viewMScan1D.setEnabled(False)

        # Process input experiment number and scan list
        try:
            r = self._uiGetExpScanTabMultiScans()
            expno = r[0]
            scanlist = r[1]
        except NotImplementedError as e:
            self._logError(str(e))
            return False

        # Clear image
        canvas = self.ui.graphicsView_mergeRun
        canvas.clearAllLines()
        canvas.clearCanvas()

        # Plot data
        unit = str(self.ui.comboBox_mscanUnit.currentText())
        xlabel = self._getXLabelFromUnit(unit)

        for scanno in scanlist:
            label = "Exp %s Scan %s"%(str(expno), str(scanno))
            self._plotReducedData(expno, scanno, canvas, xlabel, label=label, clearcanvas=False)
        # ENDFOR

        return

    def doMergeScanView2D(self):
        """ Change the merged run's view to 2D plot
        """
        # Highlight button color and change the color of another one
        self.ui.pushButton_view2D.setStyleSheet('QPushButton {background-color: white; color: gray;}')
        self.ui.pushButton_view2D.setEnabled(False)
        self.ui.pushButton_viewMScan1D.setStyleSheet('QPushButton {background-color: yellow; color: red;}')
        self.ui.pushButton_viewMScan1D.setEnabled(True)

        # Get list of data to plot
        r = self._uiGetExpScanTabMultiScans()
        expno = r[0]
        scanlist = r[1]

        # Convert the workspaces to 2D vector
        vecylist = []
        yticklabels = []
        xmin = None
        xmax = None
        for scanno in scanlist:
            # put y values to list for constructing 2D array
            vecx, vecy = self._myControl.getVectorToPlot(expno, scanno)

            vecylist.append(vecy)
            yticklabels.append('Exp %d Scan %d' % (expno, scanno))
            #print "[DB] Scan ", scanno, ": X range: ", vecx[0], vecx[-1], " Size X = ", len(vecx)

            # set up range of x
            if xmin is None:
                xmin = vecx[0]
                xmax = vecx[-1]
            # ENDIF
        # ENDFOR

        dim2array = numpy.array(vecylist)

        #print "2D vector: \n",  dim2array
        #print "x range: %f, %f" % (xmin, xmax)
        #print "y labels: ", yticklabels

        # Plot
        holdprev=False
        self.ui.graphicsView_mergeRun.clearAllLines()
        self.ui.graphicsView_mergeRun.addPlot2D(dim2array, xmin=xmin, xmax=xmax, ymin=0, \
            ymax=len(vecylist), holdprev=holdprev, yticklabels=yticklabels)

        return

    def doMergeScanViewMerged(self):
        """ Change the merged run's view to 1D plot
        """
        # Highlight the button's color
        self.ui.pushButton_view2D.setStyleSheet('QPushButton {color: red;}')
        self.ui.pushButton_view2D.setEnabled(True)
        self.ui.pushButton_viewMScan1D.setStyleSheet('QPushButton {color: red;}')
        self.ui.pushButton_viewMScan1D.setEnabled(True)

        # Clear image
        self.ui.graphicsView_mergeRun.clearCanvas()

        # Plot
        self._plotMergedReducedData(mkey=self._lastMergeIndex, label=self._lastMergeLabel)

        return


    def doPlotIndvDetMain(self):
        """ Plot individual detector
        """
        # Get exp and scan numbers and check whether the data has been loaded
        try:
            expno = self._getInteger(self.ui.lineEdit_expNo)
            scanno = self._getInteger(self.ui.lineEdit_scanNo)
        except EmptyError as e:
            self._logError(str(e))
            return

        # Get detector ID and x-label option
        try:
            status, detidlist = self._getIntArray(self.ui.lineEdit_detID.text())
            if status is False:
                errmsg = detidlist
                print "Unable to parse detector IDs due to %s."%(errmsg)
                return
            else:
                print "[DB] Detectors to plot: %s"%(detidlist)
        except EmptyError:
            self._logError("Detector ID must be specified for plotting individual detector.")
            return

        # Over plot previous or clear
        overplot = self.ui.checkBox_overPlotIndvDet.isChecked()
        if overplot is False:
            self.doClearIndDetCanvas()

        xlabel = str(self.ui.comboBox_indvDetXLabel.currentText()).strip()
        if xlabel != "" and xlabel != "Pt." and xlabel != "2theta/Scattering Angle":
            # Plot with sample logs other than Pt.
            self._logNotice("New Feature: X-label %s is supported for plotting individual detector's counts. "
                            " Set to detector angle." % xlabel)
            xlabel = xlabel
        else:
            # Plot with Pt. or detector angles
            if xlabel != "Pt.":
                xlabel = ""
            self._logNotice("X-label for individual detectror is '%s'." % (xlabel))

        # plot
        for detid in sorted(detidlist):
            try:
                self._plot_individual_detector_counts(expno, scanno, detid, xlabel, resetboundary=not overplot)
                self._expNo = expno
                self._scanNo = scanno
                self._detID = detid
                self._indvXLabel = xlabel
            except NotImplementedError as e:
                self._logError(str(e))

        return

    def doPlotIndvDetNext(self):
        """ Plot next raw detector signals for tab 'Individual Detector'
        """
        # Plot
        try:
            currdetid = self._detID + 1

            # Over plot previous or clear
            overplot = self.ui.checkBox_overPlotIndvDet.isChecked()
            if overplot is False:
                self.doClearIndDetCanvas()

            self._plot_individual_detector_counts(self._expNo, self._scanNo, currdetid,
                    self._indvXLabel)
        except KeyError as e:
            self._logError(str(e))
        else:
            self._detID = currdetid

        # Update widget
        self.ui.lineEdit_detID.setText(str(self._detID))

        return

    def doPlotIndvDetPrev(self):
        """ Plot previous individual detector's signal for tab 'Individual Detector'
        """
        # Plot
        try:
            currdetid = self._detID - 1

            # Over plot previous or clear
            overplot = self.ui.checkBox_overPlotIndvDet.isChecked()
            if overplot is False:
                self.doClearIndDetCanvas()

            self._plot_individual_detector_counts(self._expNo, self._scanNo, currdetid,
                    self._indvXLabel)
        except KeyError as e:
            self._logError(str(e))
        else:
            self._detID = currdetid

        # Update widget
        self.ui.lineEdit_detID.setText(str(self._detID))

        return

    def do_convert_plot_multi_scans(self):
        """ Convert individual plots from normalized to raw or vice verse
        """
        # Identify the mode
        if str(self.ui.pushButton_plotRawMultiScans.text()) == 'Plot Raw':
            new_mode = 'Plot Raw'
        else:
            new_mode = 'Plot Normalized'

        # Get information
        try:
            min_x = self._getFloat(self.ui.lineEdit_mergeMinX)
        except EmptyError:
            min_x = None
        try:
            max_x = self._getFloat(self.ui.lineEdit_mergeMaxX)
        except EmptyError:
            max_x = None
        bin_size = self._getFloat(self.ui.lineEdit_mergeBinSize)

        # Process input experiment number and scan list
        try:
            r = self._uiGetExpScanTabMultiScans()
            exp_no = r[0]
            scan_list = r[1]
        except NotImplementedError as e:
            self._logError(str(e))
            return False

        # Re-process the data
        if new_mode == 'Plot Raw':
            if self._multiScanList is None or self._multiScanExp is None:
                raise NotImplementedError('Experiment and scan list are not set up for plot raw.')
            self._myControl.scale_to_raw_monitor_counts(self._multiScanExp, self._multiScanList, min_x, max_x, bin_size)
        else:
            self._myControl.reset_to_normalized(self._multiScanExp, self._multiScanList, min_x, max_x, bin_size)

        # Clear image
        canvas = self.ui.graphicsView_mergeRun
        canvas.clearAllLines()
        canvas.clearCanvas()
        canvas.resetLineColorStyle()

        # Plot data
        unit = str(self.ui.comboBox_mscanUnit.currentText())
        xlabel = self._getXLabelFromUnit(unit)

        for scan_no in scan_list:
            label = "Exp %s Scan %s"%(str(exp_no), str(scan_no))
            self._plotReducedData(exp_no, scan_no, canvas, xlabel, label=label, clearcanvas=False)
        # END_FOR

        # Change the button name
        if new_mode == 'Plot Raw':
            self.ui.pushButton_plotRawMultiScans.setText('Plot Normalized')
        else:
            self.ui.pushButton_plotRawMultiScans.setText('Plot Raw')

        return


    def doPlotRawPtMain(self):
        """ Plot current raw detector signal for a specific Pt.
        """
        # Get experiment number and scan number for data file
        try:
            expno = self._getInteger(self.ui.lineEdit_expNo)
            scanno = self._getInteger(self.ui.lineEdit_scanNo)
        except EmptyError as e:
            self._logError(str(e))
            return

        # plot options
        doOverPlot = bool(self.ui.checkBox_overpltRawDet.isChecked())
        plotmode = str(self.ui.comboBox_rawDetMode.currentText())
        try:
            ptNo = self._getInteger(self.ui.lineEdit_ptNo)
        except EmptyError:
            ptNo = None

        # plot
        print "[DB] Plot Raw Detector: PlotMode = %s." % (plotmode)
        execstatus = self._plotRawDetSignal(expno, scanno, plotmode, ptNo, doOverPlot)

        # set global values if good
        if execstatus is True:
            self._rawDetPtNo = ptNo
            self._rawDetExpNo = expno
            self._rawDetScanNo = scanno
            self._rawDetPlotMode = plotmode
        else:
            print "[Error] Execution fails with signal %s. " % (str(execstatus))

        return


    def doPlotRawPtNext(self):
        """ Plot next raw detector signals
        """
        # Check
        if self._rawDetPtNo is not None:
            ptno = self._rawDetPtNo + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return
        # EndIfElse

        # Get plot mode and plot
        plotmode = str(self.ui.comboBox_rawDetMode.currentText())
        overplot = bool(self.ui.checkBox_overpltRawDet.isChecked())
        execstatus = self._plotRawDetSignal(self._rawDetExpNo, self._rawDetScanNo, plotmode,
                ptno, overplot)

        # update if it is good to plot
        if execstatus is True:
            self._rawDetPtNo = ptno
            self.ui.lineEdit_ptNo.setText(str(ptno))

        return

    def do_enable_excluded_dets(self):
        """ Enable or disable the line editor for excluded detectors
        :return:
        """
        if self.ui.checkBox_useDetExcludeFile.isChecked() is True:
            self.ui.lineEdit_detExcluded.setEnabled(True)
        else:
            self.ui.lineEdit_detExcluded.setDisabled(True)

        return

    def do_plot_raw_pt_prev(self):
        """ Plot previous raw detector
        """
        # Validate input
        if self._rawDetPtNo is not None:
            ptno = self._rawDetPtNo - 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # get plot mode and do plt
        plotmode = str(self.ui.comboBox_rawDetMode.currentText())
        overplot = bool(self.ui.checkBox_overpltRawDet.isChecked())
        execstatus = self._plotRawDetSignal(self._rawDetExpNo, self._rawDetScanNo, plotmode,
                ptno, overplot)

        # update if it is good to plot
        if execstatus is True:
            self._rawDetPtNo = ptno
            self.ui.lineEdit_ptNo.setText(str(ptno))

        return

    def do_plot_sample_log(self):
        """ Plot sample log vs. Pt. in tab 'Individual Detector'
        """
        expNo =  int(self.ui.lineEdit_expNo.text())
        scanno = int(self.ui.lineEdit_scanNo.text())
        logname = str(self.ui.comboBox_indvDetYLabel.currentText())
        self._plotSampleLog(expNo, scanno, logname)

        return


    def doReduce2Theta(self):
        """ Rebin the data and plot in 2theta for tab 'Normalized'
        """
        unit = '2theta'
        self._uiReducePlotNoramlized(unit)

        return


    def doReduceDSpacing(self):
        """ Rebin the data and plot in d-spacing for tab 'Normalized'
        """
        # new unit and information
        unit = "dSpacing"
        self._uiReducePlotNoramlized(unit)

        return


    def doReduceQ(self):
        """ Rebin the data and plot in momentum transfer Q for tab 'Normalized'
        """
        unit = 'Momentum Transfer (Q)'
        self._uiReducePlotNoramlized(unit)

        return


    def doReduceSetData(self):
        """ Reduce multiple data
        """
        # Get exp number and list of scans
        try:
            r = self._uiGetExpScanTabMultiScans()
            expno = r[0]
            scanlist = r[1]
        except NotImplementedError as e:
            self._logError(str(e))
            return False
        else:
            self._multiScanExp = expno
            self._multiScanList = scanlist

        # Reduce and plot data
        unit = str(self.ui.comboBox_mscanUnit.currentText())
        xlabel = self._getXLabelFromUnit(unit)

        canvas = self.ui.graphicsView_mergeRun
        # canvas.clearAllLines() NO NEED
        canvas.clearCanvas()
        canvas.resetLineColorStyle()

        for scan in scanlist:
            r = self._uiReduceData(3, unit, expno, scan)
            good = r[0]
            expno = r[1]
            scanno = r[2]

            if good is True:
                label = "Exp %s Scan %s"%(str(expno), str(scanno))
                self._plotReducedData(expno, scanno, canvas, xlabel, label=label, clearcanvas=False)
            else:
                self._logError('Failed to reduce Exp %s Scan %s'%(str(expno), str(scanno)))
            # ENDIF
        # ENDFOR

        return


    def doReduceVanadium2Theta(self):
        """ Rebin MDEventWorkspaces in 2-theta. for pushButton_rebinD
        in vanadium peak strip tab

        Suggested workflow
        1. Rebin data
        2. Calculate vanadium peaks in 2theta
        3.
        """
        # Reduce data
        unit = '2theta'
        itab = 4
        r = self._uiReduceData(itab, unit)
        good = r[0]
        expno = r[1]
        scanno = r[2]

        # Plot reduced data and vanadium peaks
        if good is True:
            canvas = self.ui.graphicsView_vanPeaks
            xlabel = self._getXLabelFromUnit(unit)
            label = "Exp %s Scan %s"%(str(expno), str(scanno))
            self._plotReducedData(expno, scanno, canvas, xlabel, label=label, clearcanvas=True)

            # plot vanadium peaks
            vanpeakpos = self._myControl.getVanadiumPeaksPos(expno, scanno)
            self.ui.lineEdit_stripVPeaks.setText(str(vanpeakpos))
            self._plotPeakIndicators(self.ui.graphicsView_vanPeaks, vanpeakpos)

        return good


    def doSaveData(self):
        """ Save data
        """
        # get exp number and scan number
        try:
            # exp and scan
            expno, scanno = self._uiGetExpScanNumber()
            # file type
            filetype = str(self.ui.comboBox_outputFormat.currentText())
            # file name
            savedatadir = str(self.ui.lineEdit_outputFileName.text()).strip()
            if savedatadir != None and os.path.exists(savedatadir) is True:
                homedir = savedatadir
            else:
                homedir = os.getcwd()
            # launch a dialog to get data
            filefilter = "All files (*.*);;Fullprof (*.dat);;GSAS (*.gsa)"
            sfilename = str(QtGui.QFileDialog.getSaveFileName(self, 'Save File', homedir, filefilter))
        except NotImplementedError as e:
            self._logError(str(e))
        else:
            self._myControl.savePDFile(expno, scanno, filetype, sfilename)

        return

    def doSaveMergedScan(self):
        """ Save merged scan
        """
        homedir = os.getcwd()
        filefilter = "Fullprof (*.dat)"
        sfilename = str(QtGui.QFileDialog.getSaveFileName(self, 'Save File In Fullprof', homedir, filefilter))

        self._myControl.saveMergedScan(sfilename, mergeindex=self._lastMergeIndex)

        return


    def doSaveMultipleScans(self):
        """ Save multiple scans
        """
        # Get experiment number and scans
        r = self._uiGetExpScanTabMultiScans()
        expno = r[0]
        scanslist = r[1]

        # Get base file name
        homedir = os.getcwd()
        savedir = str(QtGui.QFileDialog.getExistingDirectory(self,'Get Directory To Save Fullprof',homedir))

        for scanno in scanslist:
            sfilename = os.path.join(savedir, "HB2A_Exp%d_Scan%d_FP.dat"%(expno, scanno))
            self._myControl.savePDFile(expno, scanno, 'fullprof', sfilename)
        # ENDFOR

        return


    def doSaveVanRun(self):
        """ Save the vanadium run with peaks removed
        """
        # Get experiment number and scan number
        try:
            expno, scanno = self._uiGetExpScanNumber()
        except NotImplementedError as e:
            self._logError("Unable to get exp number and scan number for smoothing vanadium data due to %s." % (
                str(e)))
            return False

        homedir = os.getcwd()
        filefilter = "Fullprof (*.dat)"
        sfilename = str(QtGui.QFileDialog.getSaveFileName(self, 'Save File In Fullprof', homedir, filefilter))

        self._myControl.saveProcessedVanadium(expno, scanno, sfilename)

        return


    def doSmoothVanadiumData(self):
        """ Smooth vanadium spectrum
        """
        # Get experiment number and scan number
        try:
            expno, scanno = self._uiGetExpScanNumber()
        except NotImplementedError as e:
            self._logError("Unable to get exp number and scan number for smoothing vanadium data due to %s." % (
                str(e)))
            return False

        smoothparams_str = str(self.ui.lineEdit_smoothParams.text())
        # Smooth data
        status = self._myControl.smoothVanadiumSpectrum(expno, scanno, smoothparams_str)
        if status is False:
            self._logError("Failed to smooth vanadium data")

        # Plot
        unit = '2theta'
        xlabel = self._getXLabelFromUnit(unit)
        label = "Vanadium Exp %d Scan %d FFT-Smooth by %s" % (expno, scanno, smoothparams_str)
        self._plotVanadiumRun(expno, scanno, xlabel, label, False, True)

        return

    def doSmoothVanadiumApply(self):
        """ Apply smoothing effect to vanadium data
        """
        # Get experiment number and scan number
        try:
            expno, scanno = self._uiGetExpScanNumber()
        except NotImplementedError as e:
            self._logError("Unable to get exp number and scan number for smoothing vanadium data due to %s." % (
                str(e)))
            return False

        self._myControl.applySmoothVanadium(expno, scanno, True)

        return


    def doSmoothVanadiumUndo(self):
        """ Undo smoothing vanadium
        """
        try:
            expno, scanno = self._uiGetExpScanNumber()
        except NotImplementedError as e:
            self._logError("Unable to get exp number and scan number for smoothing vanadium data due to %s." % (
                str(e)))
            return False

        self._myControl.applySmoothVanadium(expno, scanno, False)

        return

    def doStripVandiumPeaks(self):
        """ Strip vanadium peaks
        """
        # Get exp number an scan number
        try:
            expno, scanno = self._uiGetExpScanNumber()
        except NotImplementedError as e:
            self._logError("Error to get Exp and Scan due to %s." % (str(e)))
            return False

        # Default unit
        unit = '2theta'

        # Get and build binning parameter
        xmin, binsize, xmax = self._uiGetBinningParams(itab=4)
        if xmin is None:
            binparams = '%f'%(binsize)
        else:
            binparams = '%f,%f,%f'%(xmin, binsize, xmax)

        # Strip vanadium peak
        good = self._myControl.stripVanadiumPeaks(expno, scanno, binparams, vanpeakposlist=None)

        # Plot
        if good is True:
            xlabel = self._getXLabelFromUnit(unit)
            label="Exp %d Scan %d Bin = %.5f Vanadium Stripped" % (expno, scanno, binsize)
            self._plotVanadiumRun(expno, scanno, xlabel, label, False)


        return


    def doUpdateWavelength(self):
        """ Update the wavelength to line edit
        """
        index = self.ui.comboBox_wavelength.currentIndex()

        print "Update wavelength to ", index

        if index == 0:
            wavelength = 2.41
        elif index == 1:
            wavelength = 1.54
        elif index == 2:
            wavelength = 1.12
        else:
            wavelength = None

        self.ui.lineEdit_wavelength.setText(str(wavelength))

        return

    def on_mouseDownEvent(self, event):
        """ Respond to pick up a value with mouse down event
        Definition of button_press_event is:
          button_press_event(x, y, button, dblclick=False, guiEvent=None)
        Thus event has x, y and button.

        event.button has 3 values:
         1: left
         2: middle
         3: right
        """
        # FUTURE: Need to make this work
        x = event.xdata
        y = event.ydata
        button = event.button


        if x is not None and y is not None:
            # mouse is clicked within graph
            if button == 1:
                msg = "Mouse 1: You've clicked on a bar with coords:\n %f, %f\n and button %d" % (x, y, button)
                print msg
            elif button == 2:
                msg = "Mouse 2: You've clicked on a bar with coords:\n %f, %f\n and button %d" % (x, y, button)
                QtGui.QMessageBox.information(self, "Click!", msg)

            elif button == 3:
                # right click of mouse will pop up a context-menu
                # menu should be self.ui.menu?
                menu = QtGui.QMenu(self)

                addAction = QtGui.QAction('Add', self)
                addAction.triggered.connect(self.addSomething)
                menu.addAction(addAction)

                rmAction = QtGui.QAction('Remove', self)
                rmAction.triggered.connect(self.rmSomething)
                menu.addAction(rmAction)

                # add other required actions
                menu.popup(QtGui.QCursor.pos())
        # ENDIF

        return

    def on_mouseMotion(self, event):
        """ Event handler for mouse being detected to move
        """
        # prev_x = self._viewMerge_X
        # prev_y = self._viewMerge_Y

        curx = event.xdata
        cury = event.ydata
        if curx is None or cury is None:
            return

        self._viewMerge_X = event.xdata
        self._viewMerge_Y = event.ydata

        #if prey is None or int(prey) != int(self._viewMerge_Y):
        #    print "Mouse is moving to ", event.xdata, event.ydata

        return

    def addSomething(self):
        """
        """
        # FUTURE - Need to implement how to deal with this
        print "Add scan back to merge"

        return

    def rmSomething(self):
        """
        """
        # FUTURE - Need to implement how to deal with this
        print "Remove a scan from merged data."

        return

    #--------------------------------------------------------------------------
    # Private methods to plot data
    #--------------------------------------------------------------------------
    def _plotIndividualDetCountsVsSampleLog(self, expno, scanno, detid, samplename, raw=True):
        """ Plot one specific detector's counts vs. one specified sample log's value
        along with all Pts.
        For example: detector 11's counts vs. sample_b's value
        :param expno:
        :param scanno:
        :param detid:
        :param samplename:
        :param raw: boolean whether the output is normalized by monitor counts
        :return:
        """
        # Validate input
        try:
            expno = int(expno)
            scanno = int(scanno)
            detid = int(detid)
            samplename = str(samplename)
        except ValueError:
            raise NotImplementedError("ExpNo, ScanNo or DetID is not integer.")

        # Get the array for detector counts vs. sample log value by mapping Pt.
        vecx, vecy = self._myControl.getIndividualDetCountsVsSample(expno, scanno,
                                                                    detid, samplename, raw)

        # Clear canvas
        self.ui.graphicsView_indvDet.clearCanvas()

        # Plot
        marker, color = self.ui.graphicsView_indvDet.getNextLineMarkerColorCombo()
        self.ui.graphicsView_indvDet.add_plot1d(vec_x=vecx,
                                                vec_y=vecy,
                                                marker=marker,
                                                color=color,
                                                x_label=samplename,
                                                y_label='Counts',
                                                label='DetID = %d'%(detid))

        # FUTURE: In future, need to find out how to use self._graphIndDevMode
        # self._graphIndDevMode = (samplename, 'Counts')
        return

    def _plot_individual_detector_counts(self, expno, scanno, detid, xaxis, resetboundary=False):
        """ Plot a specific detector's counts along all experiment points (pt)
        :param expno:
        :param scanno:
        :param detid:
        :param xaxis:
        :param resetboundary:
        :return:
        """
        # Validate input
        expno = int(expno)
        scanno = int(scanno)
        detid = int(detid)

        plot_error_bar = self.ui.checkBox_indDetErrorBar.isChecked()
        plot_normal = self.ui.checkBox_indDetNormByMon.isChecked()

        # Reject if data is not loaded
        if self._myControl.hasDataLoaded(expno, scanno) is False:
            self._logError("Data file for Exp %d Scan %d has not been loaded." % (expno, scanno))
            return False

        # Canvas and line information
        canvas = self.ui.graphicsView_indvDet
        if self._tabLineDict.has_key(canvas) is False:
            self._tabLineDict[canvas] = []

        # get data
        self._logNotice("Input x-axis is '%s' for plotting individual detector's counts."%(xaxis))
        if len(xaxis) == 0:
            xaxis = None
        vecx, vecy = self._myControl.getIndividualDetCounts(expno, scanno, detid, xaxis, plot_normal)
        if isinstance(vecx, numpy.ndarray) is False:
            raise NotImplementedError('vecx, vecy must be numpy arrays.')
        if plot_error_bar is True:
            y_err = numpy.sqrt(vecy)
        else:
            y_err = None

        # Plot to canvas
        marker, color = canvas.getNextLineMarkerColorCombo()
        if xaxis == "" or xaxis == "2theta/Scattering Angle":
            xlabel = r'$2\theta$'
        else:
            #xlabel = "Pt."
            xlabel = xaxis
        # FUTURE - If it works with any way of plotting, then refactor Pt. with any other sample names

        label = "Detector ID: %d" % (detid)

        if self._tabLineDict[canvas].count((expno, scanno, detid)) == 0:
            canvas.add_plot1d(vecx, vecy, marker=marker, color=color, x_label=xlabel,
                              y_label='Counts', label=label, y_err=y_err)
            self._tabLineDict[canvas].append((expno, scanno, detid))

            if resetboundary is True:
                # Set xmin and xmax about the data for first time
                xmin = min(vecx)
                xmax = max(vecx)
                ymin = min(vecy)
                ymax = max(vecy)
                resetboundary = False
            else:
                # auto setup for image boundary
                xmin = min(min(vecx), canvas.getXLimit()[0])
                xmax = max(max(vecx), canvas.getXLimit()[1])
                ymin = min(min(vecy), canvas.getYLimit()[0])
                ymax = max(max(vecy), canvas.getYLimit()[1])
            # ENDIFELSE

            dx = xmax-xmin
            dy = ymax-ymin
            canvas.setXYLimit(xmin-dx*0.0001, xmax+dx*0.0001, ymin-dy*0.0001, ymax+dy*0.0001)

        # Set canvas mode
        # FUTURE: Consider how to use self._graphIndDevMode in future
        # self._graphIndDevMode = (xlabel, 'Counts')

        return True


    def _plotPeakIndicators(self, canvas, peakposlist):
        """ Plot indicators for peaks
        """
        print "[DB] Peak indicators are at ", peakposlist

        rangey = canvas.getYLimit()
        rangex = canvas.getXLimit()

        for pos in peakposlist:
            if pos >= rangex[0] and pos <= rangex[1]:
                vecx = numpy.array([pos, pos])
                vecy = numpy.array([rangey[0], rangey[1]])
                canvas.add_plot1d(vecx, vecy, color='black', line_style='--')
        # ENDFOR

        return


    def _plotRawDetSignal(self, expno, scanno, plotmode, ptno, dooverplot):
        """ Plot the counts of all detectors of a certain Pt. in an experiment
        """
        # Validate input
        expno = int(expno)
        scanno = int(scanno)

        # Set up canvas and dictionary
        canvas = self.ui.graphicsView_Raw
        if self._tabLineDict.has_key(canvas) is False:
            self._tabLineDict[canvas] = []

        # Check whether data exists
        if self._myControl.hasDataLoaded(expno, scanno) is False:
            self._logError("File has not been loaded for Exp %d Scan %d.  Load data first!" % (expno, scanno))
            return

        # Get vecx and vecy
        if plotmode == "All Pts.":
            # Plot all Pts.
            vecxylist = self._myControl.getRawDetectorCounts(expno, scanno)

            # Clear previous
            self.ui.graphicsView_Raw.clearAllLines()
            self.ui.graphicsView_Raw.setLineMarkerColorIndex(0)
            self._tabLineDict[canvas] = []

        elif plotmode == "Single Pts.":
            # Plot plot
            ptno = int(ptno)

            if dooverplot is False:
                self.ui.graphicsView_Raw.clearAllLines()
                self.ui.graphicsView_Raw.setLineMarkerColorIndex(0)
                self._tabLineDict[canvas] = []

            # Plot one pts.
            vecxylist = self._myControl.getRawDetectorCounts(expno, scanno, [ptno])

        else:
            # Raise exception
            raise NotImplementedError("Plot mode %s is not supported." % (plotmode))

        # Set up unit/x-label
        unit = r"$2\theta$"

        # plot
        xmin = None
        xmax = None
        ymin = None
        ymax = None
        for ptno, vecx, vecy in vecxylist:
            # FUTURE: Label is left blank as there can be too many labels
            label = 'Pt %d' % (ptno)

            # skip if this plot has existed
            if self._tabLineDict[canvas].count( (expno, scanno, ptno) ) == 1:
                continue

            marker, color = canvas.getNextLineMarkerColorCombo()
            canvas.add_plot1d(vecx, vecy, marker=marker, color=color, x_label=unit, \
                    y_label='intensity',label=label)

            # set up line tuple
            self._tabLineDict[canvas].append( (expno, scanno, ptno) )

            # auto setup for image boundary
            xmin = min(min(vecx), canvas.getXLimit()[0])
            xmax = max(max(vecx), canvas.getXLimit()[1])
            ymin = min(min(vecy), canvas.getYLimit()[0])
            ymax = max(max(vecy), canvas.getYLimit()[1])
        # ENDFOR

        # Reset canvas x-y limit
        if xmin is not None:
            dx = xmax-xmin
            dy = ymax-ymin
            canvas.setXYLimit(xmin-dx*0.0001, xmax+dx*0.0001, ymin-dy*0.0001, ymax+dy*0.0001)

        return True


    def _plotMergedReducedData(self, mkey, label):
        """ Plot the reduced data from merged ...
        """
        # get the data
        try:
            vecx, vecy = self._myControl.getMergedVector(mkey)
        except KeyError as e:
            self._logError("Unable to retrieve merged reduced data due to %s." % (str(e)))
            return

        canvas = self.ui.graphicsView_mergeRun

        # Clear canvas
        canvas.clearAllLines()
        canvas.clearCanvas()

        # Plot
        marker, color = canvas.getNextLineMarkerColorCombo()
        xlabel = self._getXLabelFromUnit(self.ui.comboBox_mscanUnit.currentText())

        canvas.add_plot1d(vecx, vecy, marker=marker, color=color,
            x_label=xlabel, y_label='intensity',label=label)

        xmax = max(vecx)
        xmin = min(vecx)
        dx = xmax-xmin

        ymax = max(vecy)
        ymin = min(vecy)
        dy = ymax-ymin

        canvas.setXYLimit(xmin-dx*0.1, xmax+dx*0.1, ymin-dy*0.1, ymax+dy*0.1)

        return

    def _plotReducedData(self, exp, scan, canvas, xlabel, label=None, clearcanvas=True,
                         spectrum=0, plot_error=False):
        """ Plot reduced data for exp and scan
        """
        if spectrum != 0:
            raise NotImplementedError("Unable to support spectrum = %d case."%(spectrum))

        # whether the data is load
        if self._myControl.hasReducedWS(exp, scan) is False:
            self._logWarning("No data to plot!")
            return

        # get to know whether it is required to clear the image
        if clearcanvas is True:
            canvas.clearAllLines()
            canvas.setLineMarkerColorIndex(0)

        # plot
        vec_x, vec_y = self._myControl.getVectorToPlot(exp, scan)
        if isinstance(vec_x, numpy.ndarray) is False:
            vec_x = numpy.array(vec_x)
            vec_y = numpy.array(vec_y)

        # FUTURE - Should check y_err set up correctly in Mantid or not
        if plot_error is True:
            raise RuntimeError('Implement how to return y_err ASAP.')
        else:
            y_err = None

        # get the marker color for the line
        marker, color = canvas.getNextLineMarkerColorCombo()

        # plot
        if label is None:
            label = "Exp %d Scan %d" % (exp, scan)

        canvas.add_plot1d(vec_x, vec_y, marker=marker, color=color,
                          x_label=xlabel, y_label='intensity',label=label,
                          y_err=y_err)

        if clearcanvas is True:
            xmax = max(vec_x)
            xmin = min(vec_x)
            dx = xmax-xmin

            ymax = max(vec_y)
            ymin = min(vec_y)
            dy = ymax-ymin

            canvas.setXYLimit(xmin-dx*0.1, xmax+dx*0.1, ymin-dy*0.1, ymax+dy*0.1)

        return

    def _plotSampleLog(self, expno, scanno, samplelogname):
        """ Plot the value of a sample log among all Pt.
        """
        # Validate input
        expno = int(expno)
        scanno = int(scanno)
        samplelogname = str(samplelogname)

        # Reject if data is not loaded
        if self._myControl.hasDataLoaded(expno, scanno) is False:
            self._logError("Data file for Exp %d Scan %d has not been loaded." % (expno, scanno))
            return False

        # Canvas and line information
        self._indvDetCanvasMode = 'samplelog'

        # pop out the xlabel list
        # REFACTOR - Only need to set up once if previous plot has the same setup

        if self.ui.comboBox_indvDetXLabel.count() == 0:
            floatsamplelognamelist = self._myControl.getSampleLogNames(expno, scanno)
            self.ui.comboBox_indvDetXLabel.clear()
            self.ui.comboBox_indvDetXLabel.addItems(floatsamplelognamelist)
            raise RuntimeError("This X-label combo box should be set up during loading data before.")

        xlabel=str(self.ui.comboBox_indvDetXLabel.currentText())

        # get data
        vecx, vecy = self._myControl.getSampleLogValue(expno, scanno, samplelogname, xlabel)

        # Plot to canvas
        canvas = self.ui.graphicsView_indvDet
        # FUTURE - Clear canvas (think of a case that no need to clear canvas)
        canvas.clearCanvas()
        # canvas.clearAllLines()

        marker, color = canvas.getNextLineMarkerColorCombo()
        if xlabel is None:
            xlabel = r'Pt'

        label = samplelogname

        canvas.add_plot1d(vecx, vecy, marker=marker, color=color, x_label=xlabel, \
            y_label='Counts',label=label)

        # auto setup for image boundary
        xmin = min(vecx)
        xmax = max(vecx)
        ymin = min(vecy)
        ymax = max(vecy)

        dx = xmax-xmin
        dy = ymax-ymin
        canvas.setXYLimit(xmin-dx*0.0001, xmax+dx*0.0001, ymin-dy*0.0001, ymax+dy*0.0001)

        return True


    def _plotVanadiumRun(self, exp, scan, xlabel, label, clearcanvas=False, TempData=False):
        """ Plot processed vanadium data

        Arguments:
         - TempData :: flag whether the vanadium run is a temporary data set
        """
        # Check whether the data is load
        exp = int(exp)
        scan = int(scan)

        if self._myControl.hasReducedWS(exp, scan) is False:
            self._logWarning("No data to plot!")
            return

        # Get data to plot
        try:
            vecx, vecy = self._myControl.getVectorProcessVanToPlot(exp, scan, TempData)
            if TempData is False:
                vecx, vecyOrig = self._myControl.getVectorToPlot(exp, scan)
                diffY = vecyOrig - vecy
        except NotImplementedError as e:
            errmsg = '[Error] Unable to retrieve processed vanadium spectrum for exp %d scan %d. ' \
                     'Reason: %s' % (exp, scan, str(e))
            QtGui.QMessageBox.information(self, "Return!", errmsg)

            return

        # Get to know whether it is required to clear the image
        canvas = self.ui.graphicsView_vanPeaks
        if TempData is True:
            clearcanvas = False
        if clearcanvas is True:
            canvas.clearAllLines()
            canvas.setLineMarkerColorIndex(0)

        # get the marker color for the line
        if TempData is True:
            marker = None
            color = 'blue'
        else:
            marker, color = canvas.getNextLineMarkerColorCombo()

        # plot
        canvas.add_plot1d(vecx, vecy, marker=marker, color=color,
            x_label=xlabel, y_label='intensity',label=label)

        if TempData is False:
            canvas.add_plot1d(vecx, diffY, marker='+', color='green',
                x_label=xlabel, y_label='intensity',label='Diff')

        # reset canvas limits
        if clearcanvas is True:
            xmax = max(vecx)
            xmin = min(vecx)
            dx = xmax-xmin

            ymax = max(vecy)
            ymin = min(diffY)
            dy = ymax-ymin

            canvas.setXYLimit(xmin-dx*0.1, xmax+dx*0.1, ymin-dy*0.1, ymax+dy*0.1)
        # ENDIF

        return


    def _uiDownloadDataFile(self, exp, scan):
        """ Download data file according to its exp and scan
        Either download the data from a server or copy the data file from local
        disk
        """
        # Get on hold of raw data file
        useserver = self.ui.radioButton_useServer.isChecked()
        uselocal = self.ui.radioButton_useLocal.isChecked()
        if useserver == uselocal:
            self._logError("It is logically wrong to set up server/local dir for data.")
            useserver = True
            uselocal = False
            self.ui.radioButton_useServer.setChecked(True)
            self.ui.radioButton_useLocal.setChecked(False)
        # ENDIF

        rvalue = False
        if self._srcFromServer is True:
            # Use server: build the URl to download data
            if self._serverAddress.endswith('/') is False:
                self._serverAddress += '/'
            fullurl = "%s%s/exp%d/Datafiles/%s_exp%04d_scan%04d.dat" % (self._serverAddress,
                    self._instrument.lower(), exp, self._instrument.upper(), exp, scan)
            print "URL: ", fullurl

            cachedir = str(self.ui.lineEdit_cache.text()).strip()
            if os.path.exists(cachedir) is False:
                invalidcache = cachedir
                cachedir = os.getcwd()
                self.ui.lineEdit_cache.setText(cachedir)
                self._logWarning("Cache directory %s is not valid. "
                                 "Using current workspace directory %s as cache." % (invalidcache, cachedir) )

            filename = '%s_exp%04d_scan%04d.dat' % (self._instrument.upper(), exp, scan)
            srcFileName = os.path.join(cachedir, filename)
            status, errmsg = downloadFile(fullurl, srcFileName)
            if status is False:
                self._logError(errmsg)
                srcFileName = None
            else:
                rvalue = True

        elif self._srcAtLocal is True:
            # Data from local
            srcFileName = os.path.join(self._localSrcDataDir, "%s/Exp%d_Scan%04d.dat" % (self._instrument, exp, scan))
            if os.path.exists(srcFileName) is True:
                rvalue = True

        else:
            raise NotImplementedError("Logic error.  Neither downloaded from server.\
                Nor from local drive")

        return (rvalue,srcFileName)


    def _uiGetBinningParams(self, itab):
        """ Get binning parameters

        Return:
         - xmin, binsize, xmax
        """
        # Get value
        if itab == 2:
            xmin = str(self.ui.lineEdit_xmin.text())
            xmax = str(self.ui.lineEdit_xmax.text())
            binsize = str(self.ui.lineEdit_binsize.text())
        elif itab == 3:
            xmin = str(self.ui.lineEdit_mergeMinX.text())
            xmax = str(self.ui.lineEdit_mergeMaxX.text())
            binsize = str(self.ui.lineEdit_mergeBinSize.text())
        elif itab == 4:
            xmin = str(self.ui.lineEdit_min2Theta.text())
            xmax = str(self.ui.lineEdit_max2Theta.text())
            binsize = str(self.ui.lineEdit_binsize2Theta.text())
        else:
            raise NotImplementedError("Binning parameters are not used for %d-th tab."%(itab))

        # Parse values
        try:
            xmin = float(xmin)
            xmax = float(xmax)
        except ValueError:
            xmin = None
            xmax = None
        else:
            if xmin >= xmax:
                raise NotImplementedError("set minimum X = %.5f is larger than \
                    maximum X = %.5f" % (xmin, xmax))

        try:
            binsize = float(binsize)
        except ValueError:
            raise NotImplementedError("Error:  bins size '%s' is not a float number." % (binsize))

        # Fix for merging as xmin and xmax must be same for all scans
        if itab == 3 and xmin is None:
            xmin = 5.
            xmax = 150.

        return (xmin, binsize, xmax)


    def _uiGetExcludedDetectors(self):
        """ Get excluded detectors from input line edit

        Return :: list of detector IDs to exclude from reduction
        """
        excludedetidlist = []

        if self.ui.checkBox_useDetExcludeFile.isChecked():
            detids_str = str(self.ui.lineEdit_detExcluded.text()).strip()
            status, excludedetidlist = self._getIntArray(detids_str)
            if status is False:
                self._logError("Extra scans are not a list of integers: %s." % (
                    str(self.ui.lineEdit_extraScans.text())))
                excludedetidlist = []
            # ENDIF
        # ENDIF

        return excludedetidlist



    def _uiGetExpScanNumber(self):
        """ Get experiment number and scan number from widgets for merged
        """
        expnostr = self.ui.lineEdit_expNo.text()
        scannostr = self.ui.lineEdit_scanNo.text()
        try:
            expno = int(expnostr)
            scanno = int(scannostr)
        except ValueError:
            raise NotImplementedError("Either Exp No '%s' or Scan No '%s \
                is not set up right as integer." % (expnostr, scannostr))

        return (expno, scanno)


    def _uiGetExpScanTabMultiScans(self):
        """ Get exp number and scans from tab 3
        """
        try:
            expno = int(self.ui.lineEdit_expNo.text())
            startscan = int(self.ui.lineEdit_scanStart.text())
            endscan = int(self.ui.lineEdit_scanEnd.text())
        except ValueError as e:
            raise RuntimeError("For merging scans, Exp No, Starting scan number and \
                    end scan number must be given: %s" % (str(e)))

        # scans = [startscan, endscan] + [others] - [excluded]
        status, extrascanlist = self._getIntArray(str(self.ui.lineEdit_extraScans.text()))
        if status is False:
            raise RuntimeError(extrascanlsit)

        status, excludedlist = self._getIntArray(str(self.ui.lineEdit_exclScans.text()))
        self._logDebug("Excluded list: %s" %(str(excludedlist)))
        if status is False:
            self._logError(excludedlist)
            return

        scanslist = range(startscan, endscan+1)
        scanslist.extend(extrascanlist)
        scanslist = list(set(scanslist))
        for scan in excludedlist:
            scanslist.remove(scan)

        return (expno, sorted(scanslist))


    def _uiIsBinParamsChange(self, itab, binparams):
        """ Check whether current bin parameters are same
        as given value
        """
        xmin,binsize,xmax = self._uiGetBinningParams(itab)
        newbinparams = [xmin, binsize, xmax]

        # check binning
        same = True
        for i in xrange(3):
            par_0 = binparams[i]
            par_1 = newbinparams[i]

            try:
                if abs(float(par_0)-float(par_1)) > 1.0E-6:
                    same = False
            except TypeError:
                if par_0 is not None or par_1 is not None:
                    same = False

            if same is False:
                break
        # ENDFOR

        change = not same
        if change is True:
            print "[D...............B]",
            print "%s vs %s "  % (str(xmin), str(self._tabBinParamDict[itab][0])),
            print "%s vs %s "  % (str(xmax), str(self._tabBinParamDict[itab][2])),
            print "%s vs %s "  % (str(binsize), str(self._tabBinParamDict[itab][1]))
        else:
            print "[DB] Rebin = False"

        return change


    def _uiReduceData(self, itab, unit, expno=None, scanno=None):
        """ Rebin and plot by reading GUI widgets' value

        Arguments:
         - itab : index of the tab.  Only 2m 3 and 4 are allowed
         - unit : string for target unit
        """
        # Experiment number and Scan number
        if isinstance(expno, int) and isinstance(scanno, int):
            # Call from tab-3 multiple scan
            pass
        else:
            try:
                expno, scanno = self._uiGetExpScanNumber()
            except NotImplementedError as e:
                self._logError(str(e))
                return
        # ENDIF

        # Get binning parameter
        xmin, binsize, xmax = self._uiGetBinningParams(itab)

        # Get wavelength
        try:
            if itab == 3:
                wavelength = float(self._myControl.getWavelength(expno, scanno))
            else:
                wavelength = float(str(self.ui.lineEdit_wavelength.text()))
        except TypeError:
            if unit != '2theta':
                raise NotImplementedError('Wavelength must be specified for unit %s.'%(unit))

        # Get scale factor
        try:
            scalefactor = self._getFloat(self.ui.lineEdit_normalizeMonitor)
        except EmptyError:
            scalefactor = None
        except ValueError as valueerror:
            raise ValueError("Unable to get normalization factor due to %s."%(str(valueerror)))

        # Rebin
        try:
            # rebinned = self._myControl.rebin(expno, scanno, unit, wavelength, xmin, binsize, xmax)
            excludeddetlist = self._uiGetExcludedDetectors()
            self._myControl.reduceSpicePDData(expno, scanno,
                                              unit, xmin, xmax, binsize, wavelength, excludeddetlist, scalefactor)

            # Record binning
            self._tabBinParamDict[itab] = [xmin, binsize, xmax]
        except NotImplementedError as e:
            self._logError(str(e))
            return (False, expno, scanno)

        return (True, expno, scanno)


    def _uiReducePlotNoramlized(self, unit):
        """ Support Reduce2Theta, ReduceDspacing and ReduceQ
        """
        itab = 2
        canvas = self.ui.graphicsView_reducedData
        expno, scanno = self._uiGetExpScanNumber()

        change = self._uiIsBinParamsChange(itab, self._tabBinParamDict[itab])
        # check whether line record
        if unit == self._currUnit and \
                self._tabLineDict[itab].count((expno, scanno)) > 0 and change is False:
            # there is no need to plot again as line exists
            return

        # reduce
        r = self._uiReduceData(2, unit)
        good = r[0]
        expno = r[1]
        scanno = r[2]

        # failed to reduce
        if good is False:
            self._logError("Failed to reduce Exp %d Scan %d" % (expno, scanno))
            return

        # clear canvas???
        if unit != self._currUnit:
            clearcanvas = True
        elif self.ui.checkBox_clearPrevious.isChecked() is False:
            # NOTE: naming of the widget is VERY confusing.  Should be changed to keepPrevious
            clearcanvas = True
        else:
            clearcanvas = False

        # reset record dictionary if unit is different from present
        if clearcanvas is True:
            self._tabLineDict[itab] = []

        self._currUnit = unit
        self._tabLineDict[itab].append((expno, scanno))

        xlabel = self._getXLabelFromUnit(unit)
        label = "Exp %s Scan %s"%(str(expno), str(scanno))
        self._plotReducedData(expno, scanno, canvas, xlabel, label=label, clearcanvas=clearcanvas)

        return



    def _logDebug(self, dbinfo):
        """ Log debug information
        """
        print dbinfo


    def _logError(self, errinfo):
        """ Log error information
        """
        QtGui.QMessageBox.information(self, "Click!", errinfo)

    def _logNotice(self, loginfo):
        """ Log error information
        """
        msg = '[Notice] %s' % loginfo
        print msg
        # QtGui.QMessageBox.information(self, "Click!", msg)


    def _logWarning(self, warning_info):
        """ Log error information
        """
        msg = "[Warning]: %s" % (warning_info)
        QtGui.QMessageBox.information(self, "OK!", msg)

        return


    def _getFloat(self, lineedit):
        """ Get integer from line edit
        Exception: ValueError if empty or no input
        """
        valuestr = str(lineedit.text()).strip()
        if len(valuestr) == 0:
            raise EmptyError("Input is empty. It cannot be converted to float.")

        try:
            value = float(valuestr)
        except ValueError as e:
            raise e

        return value


    def _getInteger(self, lineedit):
        """ Get integer from line edit
        """
        valuestr = str(lineedit.text()).strip()
        if len(valuestr) == 0:
            raise EmptyError("Input is empty. It cannot be converted to integer.")

        try:
            value = int(valuestr)
        except ValueError as e:
            raise e

        return value


    def _getIntArray(self, intliststring):
        """ Validate whether the string can be divided into integer strings.
        Allowed: a, b, c-d, e, f

        Return :: 2-tuple (status, list/error message)
        """
        intliststring = str(intliststring)
        if intliststring == "":
            return (True, [])

        # Split by ","
        termlevel0s = intliststring.split(",")
        intlist = []

        # For each term
        errmsg = ""
        returnstatus = True

        for level0term in termlevel0s:
            level0term = level0term.strip()

            # split upon dash -
            numdashes = level0term.count("-")
            if numdashes == 0:
                # one integer
                valuestr = level0term
                try:
                    intvalue = int(valuestr)
                    if str(intvalue) != valuestr:
                        returnstatus = False
                        errmsg =  "Contains non-integer string %s." % (valuestr)
                except ValueError:
                    returnstatus = False
                    errmsg = "String %s is not an integer." % (valuestr)
                else:
                    intlist.append(intvalue)

            elif numdashes == 1:
                # Integer range
                twoterms = level0term.split("-")
                templist = []
                for i in xrange(2):
                    valuestr = twoterms[i]
                    try:
                        intvalue = int(valuestr)
                        if str(intvalue) != valuestr:
                            returnstatus = False
                            errmsg = "Contains non-integer string %s." % (valuestr)
                    except ValueError:
                        returnstatus = False
                        errmsg = "String %s is not an integer." % (valuestr)
                    else:
                        templist.append(intvalue)

                    # break loop
                    if returnstatus is False:
                        break
                # ENDFOR
                intlist.extend(range(templist[0], templist[1]+1))

            else:
                # Undefined siutation
                returnstatus = False
                errmsg = "Term %s contains more than 1 dash." % (level0terms)
            # ENDIFELSE

            # break loop if something is wrong
            if returnstatus is False:
                break
        # ENDFOR

        # Return with false
        if returnstatus is False:
            return (False, errmsg)

        return (True, intlist)


    def _getXLabelFromUnit(self, unit):
        """ Get X-label from unit
        """
        if unit == '2theta':
            xlabel = r'$2\theta$ (Degrees)'
        elif unit == 'dSpacing':
            xlabel = r"d $(\AA)$"
        elif unit == 'Momentum Transfer (Q)':
            xlabel = r"Q $(\AA^{-1})$"
        else:
            xlabel = 'Wacky Unknown'

        return xlabel
