#pylint: disable=invalid-name
import numpy
import sys
import os

from Ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
#from PyQt4.QtCore import *
#from PyQt4.QtGui import *
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

from matplotlib.pyplot import setp

try:
    import mantid.simpleapi as api
    import mantid.kernel
    from mantid.simpleapi import AnalysisDataService
    from mantid.kernel import ConfigService
    IMPORT_MANTID = True
except ImportError as e:
    print "Unable to import Mantid: %s." % (str(e))
    IMPORT_MANTID = False


#----- default configuration ---------------
DEFAULT_SERVER = 'http://neutron.ornl.gov/'
DEFAULT_INSTRUMENT = 'HB2A'
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

        # UI widgets setup
        self.ui.comboBox_outputFormat.addItems(['Fullprof', 'GSAS', 'Fullprof+GSAS'])

        # Mantid configuration
        if IMPORT_MANTID is True:
            config = ConfigService.Instance()
            self._instrument = config["default.instrument"]
        else:
            self._instrument = DEFAULT_INSTRUMENT

        # Set up data source
        self._serverAddress = DEFAULT_SERVER + "_" + self._instrument
        self._srcFromServer = True
        self._localSrcDataDir = None
        self._srcAtLocal = False

        self._currUnit = '2theta'


    #-- Event Handling ----------------------------------------------------


    def doLoadData(self):
        """ Load data 
        """
        # Get information
        expno = int(self.ui.lineEdit_expNo.text())
        scanno = int(self.ui.lineEdit_scanNo.text())

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
        # Figure out file name
        if self._srcFromServer is True:
            # Use server
            fullurl = self._serverAddress + "/Exp%d_Scan%04d.dat" % (exp, scan)
            cachedfile = urllib2.download()
            self._srcFileName = "Terrible"

        elif self._srcAtLocal is True:
            # Data from local
            self._srcFileName = os.path.join(self._localSrcDataDir, "%s/Exp%d_Scan%04d.dat" % (self._instrument, exp, scan))

        else:
            raise NotImplementedError("XXXXXX")

        return self._srcFileName


    def _reduceSpicePDData(self, datafilename, unit, xmin, xmax, binsize):
        """ Reduce SPICE powder diffraction data
        """
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
