################################################################################
#
# Main class for HFIR powder reduction GUI
#
################################################################################

#pylint: disable=invalid-name
import numpy
import sys
import os
import urllib2

from Ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

from matplotlib.pyplot import setp

# FIXME - Remove after debugging
NOMANTID = True

try:
    import mantid.simpleapi as api
    import mantid.kernel
    from mantid.simpleapi import AnalysisDataService
    from mantid.kernel import ConfigService
    IMPORT_MANTID = True
except ImportError as e:
    if NOMANTID is False: 
        print "Unable to import Mantid: %s." % (str(e))
        raise e
    else:
        print "NO MANTID IS USED FOR DEBUGGING PURPOSE."


#----- default configuration ---------------
DEFAULT_SERVER = 'http://neutron.ornl.gov/user_data'
DEFAULT_INSTRUMENT = 'hb2a'
DEFAULT_WAVELENGTH = 2.4100

#-------------------------------------------


class MainWindow(QtGui.QMainWindow):
    """ Class of Main Window (top)

    Copy to ui.setupUI
    # Version 3.0 + Import for Ui_MainWindow.py
        from MplFigureCanvas import Qt4MplCanvas

        # Replace 'self.graphicsView = QtGui.QtGraphicsView' with the following
        self.graphicsView = Qt4MplCanvas(self.centralwidget)
        self.mainplot = self.graphicsView.getPlot()

    """
    def __init__(self, parent=None):
        """ Intialization and set up
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # UI Window (from Qt Designer)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Define gui-event handling 
        self.connect(self.ui.pushButton_loadData, QtCore.SIGNAL('clicked()'), 
                self.doLoadData)

        self.connect(self.ui.pushButton_unit2theta, QtCore.SIGNAL('clicked()'),
                self.doPlot2Theta)

        self.connect(self.ui.pushButton_unitD, QtCore.SIGNAL('clicked()'),
                self.doPlotDspacing)

        self.connect(self.ui.pushButton_unitQ, QtCore.SIGNAL('clicked()'),
                self.doPlotQ)

        self.connect(self.ui.pushButton_saveData, QtCore.SIGNAL('clicked()'),
                self.doSaveData)

        self.connect(self.ui.pushButton_browseCache, QtCore.SIGNAL('clicked()'),
                self.doBrowseCache)

        # Define signal-event handling


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

        # Get initial setup
        self._initSetup()

        return


    def _initSetup(self):
        """ Initial setup
        """
        # FIXME - This part will be implemented soon as default configuration is made
        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())
        #if IMPORT_MANTID is True:
        #    config = ConfigService.Instance()
        #    self._instrument = config["default.instrument"]
        #else:
        #    self._instrument = DEFAULT_INSTRUMENT

        # UI widgets setup
        self.ui.comboBox_outputFormat.addItems(['Fullprof', 'GSAS', 'Fullprof+GSAS'])


        # Set up data source
        self._serverAddress = DEFAULT_SERVER 
        self._srcFromServer = True
        self._localSrcDataDir = None
        self._srcAtLocal = False

        self._currUnit = '2theta'

        # Workspaces
        self._outws = None
        self._prevoutws = None


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


    def doLoadData(self):
        """ Load data 
        """
        # Get information
        try:
            expno = int(self.ui.lineEdit_expNo.text())
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return

        self._logDebug("Attending to load Exp %d Scan %d." % (expno, scanno))

        # Form 
        datafilename = self._loadDataFile(exp=expno, scan=scanno)

        # Get other information
        xmin, xmax, binsize = self._getBinningParams()
        if binsize is None:
            self._logError("Bin size must be specified.")

        unit = self._currUnit

        execstatus = self._reduceSpicePDData(datafilename, unit, xmin, xmax, binsize)

        return


    def doPlot2Theta(self):
        """ Rebin the data and plot in 2theta
        """
        self._plotReducedData('2theta')

        return

    def doPlotDspacing(self):
        """ Rebin the data and plot in d-spacing
        """
        self._plotReducedData('dSpacing')

        return

    def doPlotQ(self):
        """ Rebin the data and plot in momentum transfer Q
        """
        self._plotReducedData('Momentum Transfer (Q)')

        return


    def doSaveData(self):
        """ Save data
        """
        # check whether it is fine to save 
        if self._outws is None:
            self._logError("No reduced diffraction data to save.")
            #return

        # file type
        filetype = str(self.ui.comboBox_outputFormat.currentText())

        # get line edit for save data location
        savedatadir = str(self.ui.lineEdit_outputFileName.text()).strip()
        if savedatadir != None and os.path.exists(savedatadir) is True:
            homedir = savedatadir
        else:
            homedir = os.getcwd()

        # launch a dialog to get data
        filter = "All files (*.*);;Fullprof (*.dat);;GSAS (*.gsa)"
        sfilename = str(QtGui.QFileDialog.getSaveFileName(self, 'Save File', homedir, filter))

        print "Get file name: ", sfilename
   
        # save
        # FIXME - ASAP
        if filetype.lower().count("fullprof") == 1:
            print "going to save for Fullprof"

        if filetype.lower().count("gsas") == 1:
            print "going to save GSAS"

        return


    #--------------------------------------------------------
    #
    #--------------------------------------------------------

    def _getBinningParams(self):
        """ Get binning parameters
        """
        xminstr = str(self.ui.lineEdit_xmin.text()).strip()
        if len(xminstr) == 0:
            xmin = None
        else:
            xmin = float(xminstr)

        xmaxstr = str(self.ui.lineEdit_xmax.text()).strip()
        if len(xmaxstr) == 0:
            xmax = None
        else:
            xmax = float(xmaxstr)

        binsizestr = str(self.ui.lineEdit_binsize.text()).strip()
        if len(binsizestr) == 0:
            binsize = None
        else:
            binsize = float(xminstr)

        return xmin, xmax, binsize

    
    def _loadDataFile(self, exp, scan):
        """ Load data file according to its exp and scan
        """
        # Get on hold of raw data file
        if self._srcFromServer is True:
            # Use server: build the URl to download data
            if self._serverAddress.endswith('/') is False:
                self._serverAddress += '/'
            fullurl = "%s%s/exp%d/Datafiles/%s_exp%04d_scan%04d.dat" % (self._serverAddress,
                    self._instrument.lower(), exp, self._instrument.upper(), exp, scan)
            print "URL: ", fullurl
    
            cachedir = str(self.ui.lineEdit_cache.text()).strip()
            if os.path.exists(cachedir) is False:
                self._logError("Cache directory is not valid.")
                return
    
            filename = '%s_exp%04d_scan%04d.dat' % (self._instrument.upper(), exp, scan)
            self._srcFileName = os.path.join(cachedir, filename)
            status, errmsg = self._downloadFile(fullurl, self._srcFileName)
            if status is False:
                self._logError(errmsg)
                self._srcFileName = None
                return

        elif self._srcAtLocal is True:
            # Data from local
            self._srcFileName = os.path.join(self._localSrcDataDir, "%s/Exp%d_Scan%04d.dat" % (self._instrument, exp, scan))

        else:
            raise NotImplementedError("XXXXXX")

        return self._srcFileName


    def _reduceSpicePDData(self, datafilename, unit, xmin, xmax, binsize):
        """ Reduce SPICE powder diffraction data
        """
        # cache the previous one
        self._prevoutws = self._outws
        self._outws = None

        # Rebin
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        # base workspace name
        basewsname = os.path.basename(datafilename).split(".")[0]

        # load SPICE
        tablewsname = basewsname + "_RawTable"
        infowsname  = basewsname + "ExpInfo"
        api.LoadSpiceAscii(Filename=datafilename, 
                OutputWorkspace=tablewsname, RunInfoWorkspace=infowsname)

        # Build MDWorkspace
        datamdwsname = basewsname + "_DataMD"
        monitorwsname = basewsname + "_MonitorMD"
        api.ConvertSpiceDataToRealSpace(InputWorkspace=tablewsname,
                RunInfoWorkspace=infowsname,
                OutputWorkspace=datamdwsname,
                OutputMontiorWorkspace=monitorwsname)

        self._datamdws = AnalysisDataService.retrieve(datamdwsname)
        self._monitormdws = AnalysisDataService.retrieve(monitorwsname)

        # Rebin
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        outwsname = basewsname + "_Reduced_" + unit
        api.ConvertCWPDMDToSpectra(InputWorkspace=datamdwsname,
                InputMonitorWorkspace=monitorwsname,
                OutputWorkspace=outwsname,
                BinningParams=binpar,
                UnitOutput = unit, 
                NeutronWaveLength=wavelength)

        self._outws = AnalysisDataService.retrieve(outwsname)

        return 


    def _plotReducedData(self, targetunit):
        """ Plot reduced data
        """
        # whether the data is load?
        if self._inPlotState is False:
            self._logWarning("No data to plot!")

        targetunit = '2theta'
        if self._currUnit != targetunit:
            self._currUnit = targetunit
            self._rebin(targetunit)
            self._plotBinnedData()



        # read the xmin, xmax and bin size
        xmin = float(self.ui.lineEdit_xmin.text())
        xmax = float(self.ui.lineEdit_xmax.text())
        binsize = float(self.ui.lineEdit_xmin.text())





    def _rebin(self, unit, xmin, binsize, xmax):
        """ 
        """
        # TODO - ASAP

        return


    def _excludeDetectors(self, detids):
        """
        """
        # TODO 

        return


    def _excludePt(self, pts):
        """
        """ 
        # TODO

        return






    def _logDebug(self, dbinfo):
        """ Log debug information
        """
        print dbinfo


    def _logError(self, errinfo):
        """ Log error information
        """



    def _downloadFile(self, url, localfilepath):
        """
        Test: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
        """
        # open URL
        response = urllib2.urlopen(url)
        wbuf = response.read()

        if wbuf.count('not found') > 0:
            return (False, "File cannot be found at %s." % (url))


        ofile = open(localfilepath, 'w')
        ofile.write(wbuf)
        ofile.close()

        return (True, "")

