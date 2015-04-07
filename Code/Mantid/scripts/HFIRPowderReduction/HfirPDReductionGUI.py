################################################################################
#
# Main class for HFIR powder reduction GUI
#
################################################################################

#pylint: disable=invalid-name
import numpy
import sys
import os

from Ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

from matplotlib.pyplot import setp

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
        self.value = value     
        
    def __str__(self): 
        return repr(self.value)



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

        # tab 'Normalized'
        self.connect(self.ui.pushButton_loadData, QtCore.SIGNAL('clicked()'), 
                self.doLoadData)
        self.connect(self.ui.pushButton_prevScan, QtCore.SIGNAL('clicked()'),
                self.doLoadPrevScan)
        self.connect(self.ui.pushButton_nextScan, QtCore.SIGNAL('clicked()'),
                self.doLoadNextScan)
        self.connect(self.ui.pushButton_unit2theta, QtCore.SIGNAL('clicked()'),
                self.doPlot2Theta)
        self.connect(self.ui.pushButton_unitD, QtCore.SIGNAL('clicked()'),
                self.doPlotDspacing)
        self.connect(self.ui.pushButton_unitQ, QtCore.SIGNAL('clicked()'),
                self.doPlotQ)
        self.connect(self.ui.pushButton_saveData, QtCore.SIGNAL('clicked()'),
                self.doSaveData)

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

        # tab 'Raw Detectors'
        self.connect(self.ui.pushButton_plotRaw, QtCore.SIGNAL('clicked()'),
                self.doPlotRawDet)
        self.connect(self.ui.pushButton_ptUp, QtCore.SIGNAL('clicked()'),
                self.doPlotPrevPtRaw)
        self.connect(self.ui.pushButton_ptDown, QtCore.SIGNAL('clicked()'),
                self.doPlotNextPtRaw)

        # tab 'Individual Detectors'
        self.connect(self.ui.pushButton_plotIndvDet, QtCore.SIGNAL('clicked()'),
                self.doPlotIndvDet)
        self.connect(self.ui.pushButton_plotPrevDet, QtCore.SIGNAL('clicked()'),
                self.doPlotPrevDetRaw)
        self.connect(self.ui.pushButton_plotNextDet, QtCore.SIGNAL('clicked()'),
                self.doPlotNextDetRaw)
                
        # main           
        self.connect(self.ui.comboBox_wavelength, QtCore.SIGNAL('currentIndexChanged(int)'),
                self.doUpdateWavelength)
        self.connect(self.ui.actionQuit, QtCore.SIGNAL('triggered()'),
                self.doExist)

        # tab 'Vanadium'
        self.connect(self.ui.pushButton_stripVanPeaks, QtCore.SIGNAL('clicked()'),
                self.doStripVandiumPeaks)
        self.connect(self.ui.pushButton_saveVanRun, QtCore.SIGNAL('clicked()'),
                self.doSaveVanRun)
        self.connect(self.ui.pushButton_rebinD, QtCore.SIGNAL('clicked()'),
                self.doRebinD)

        # tab 'Multiple Scans'
        self.connect(self.ui.pushButton_mergeScans, QtCore.SIGNAL('clicked()'),
                self.doMergeScans)
        self.connect(self.ui.pushButton_view2D, QtCore.SIGNAL('clicked()'), 
                self.doMergeScanView2D)
        self.connect(self.ui.pushButton_viewMerge, QtCore.SIGNAL('clicked()'),
                self.doMergeScanView1D)

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

        validator8 = QtGui.QDoubleValidator(self.ui.lineEdit_minD)
        validator8.setBottom(0.)
        self.ui.lineEdit_minD.setValidator(validator8)

        validator9 = QtGui.QDoubleValidator(self.ui.lineEdit_maxD)
        validator9.setBottom(0.)
        self.ui.lineEdit_maxD.setValidator(validator9)

        validator10 = QtGui.QDoubleValidator(self.ui.lineEdit_binsizeD)
        validator10.setBottom(0.)
        self.ui.lineEdit_binsizeD.setValidator(validator10)

        validator11 = QtGui.QIntValidator(self.ui.lineEdit_scanStart)
        validator11.setBottom(1)
        self.ui.lineEdit_scanStart.setValidator(validator11)

        validator12 = QtGui.QIntValidator(self.ui.lineEdit_scanEnd)
        validator12.setBottom(1)
        self.ui.lineEdit_scanEnd.setValidator(validator12)

        # TODO - Add valdiators

        # Get initial setup
        self._initSetup()

        return


    def _initSetup(self):
        """ Initial setup
        """
        # FIXME - This part will be implemented soon as default configuration is made
        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())

        # UI widgets setup
        self.ui.comboBox_outputFormat.addItems(['Fullprof', 'GSAS', 'Fullprof+GSAS'])

        # FIXME : Need to disable some widgets... consider to refactor the code
        self.ui.radioButton_useServer.setChecked(True)
        self.ui.radioButton_useLocal.setChecked(False)
        
        self.ui.comboBox_wavelength.setCurrentIndex(0)
        self.ui.lineEdit_wavelength.setText('2.41')

        self.ui.pushButton_unit2theta.setText(r'$2\theta$')

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
        
    def doBrowseLocalDataSrc(self):
        """ Browse local data storage
        """
        print "Browse local data storage location."
        
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
        
        print "Check source data server!"
        
        return

    def doLoadData(self):
        """ Load and reduce data 
        """
        # Get information
        try: 
            expno, scanno = self._uiGetExpScanNumber()
        except Exception as e:
            self._logError("Error to get Exp and Scan due to %s." % (str(e)))
            return

        self._logDebug("Attending to load Exp %d Scan %d." % (expno, scanno))

        # Form data file name and download data
        status, datafilename = self._uiLoadDataFile(exp=expno, scan=scanno)
        if status is False:
            self._logError("Unable to download or locate local data file for Exp %d \
                Scan %d." % (expno, scanno))

        # Get other information
        try:
            xmin, binsize, xmax = self._uiGetBinningParams()
        except Exception as e:
            self._logError(str(e))
            return

        unit = self._currUnit
        wavelength = float(self.ui.lineEdit_wavelength.text())

        # Reduce data     
        execstatus = self._myControl.reduceSpicePDData(expno, scanno, datafilename, unit, xmin, 
            xmax, binsize, wavelength)
        print "[DB] reduction status = ", execstatus

        # Plot data
        
        clearcanvas = self.ui.checkBox_clearPrevious.isChecked() 
        
        xlabel = self._getXLabelFromUnit(unit)
        if execstatus is True:
            self._plotReducedData(expno, scanno, self.ui.graphicsView_reducedData, xlabel,
                label="Exp %d Scan %d Bin = %.5f" % (expno, scanno, binsize), clearcanvas=clearcanvas)
            
        return execstatus


    def doLoadPrevScan(self):
        """
        """
        # Advance scan number by 1
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

        # call load data
        execstatus = self.doLoadData()

        return execstatus


    def doLoadNextScan(self):
        """
        """
        # Advance scan number by 1
        try:
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return
        else:
            scanno = scanno + 1
            if scanno < 1:
                self._logWarning("Scan number is 1 already.  Cannot have previous scan")
                return
            self.ui.lineEdit_scanNo.setText(str(scanno))

        # call load data
        execstatus = self.doLoadData()
        if execstatus is False:
            scanno = scanno - 1
            self.ui.lineEdit_scanNo.setText(str(scanno))

        return execstatus


    def doExist(self):
        """ Exist the application
        """
        clearcache = self.ui.checkBox_delCache.isChecked()

        if clearcache is True:
            # TODO - Clear cache
            print "Clear Cache! Implement ASAP!"

        self.close()

        return
        
        
    def doMergeScans(self):
        """ Merge several scans 
        for tab 'merge'
        """
        # get inputs for scans
        try:
            expno = int(self.ui.lineEdit_expNo.text())
            startscan = int(self.ui.lineEdit_scanStart.text())
            endscan = int(self.ui.lineEdit_scanEnd.text())
        except ValueError as e:
            self._logError("For merging scans, both starting scan number and \
                end scan number must be given.")
            return
        
        excludedlist = self._getIntArray(str(self.ui.lineEdit_exclScans.text()))
        self._logDebug("Excluded list: %s" %(str(excludedlist)))
        if isinstance(excludedlist, str):
            self._logError(excludedlist)
            return
            
        scanslist = range(startscan, endscan+1)
        for scan in excludedlist:
            scanslist.remove(scan)
        
        # process input exp number and scan list
        expscanfilelist = []
        for scanno in scanslist:
            retv, datafilename = self._uiLoadDataFile(expno, scanno)
            if retv is True:
                expscanfilelist.append( (expno, scanno, datafilename) )
            else:
                self._logError("Unable to load Exp %s Scan %s." % (expno, scanno))
        # ENDFOR

        try: 
            unit = self._currUnit
            xmin, binsize, xmax = self._uiGetBinningParams()
            wavelength = float(self.ui.lineEdit_wavelength.text())
        except Exception as e:
            raise e
        mindex = self._myControl.mergeReduceSpiceData(expscanfilelist, unit, xmin, xmax, binsize, wavelength)
        
        label = "Exp %d, Scan %s." % (expno, str(scanslist))
        self._plotMergedReducedData(mindex, label)
        self._lastMergeIndex = mindex
        self._lastMergeLabel = label
        
        return
        
    def doMergeScanView1D(self):
        """ Change the merged run's view to 1D plot
        """
        # Highlight the button's color
        self.ui.pushButton_view2D.setStyleSheet('QPushButton {color: black;}')
        self.ui.pushButton_viewMerge.setStyleSheet('QPushButton {color: red;}')        
        
        # Clear image
        self.ui.graphicsView_mergeRun.clearCanvas()
        
        # Plot
        self._plotMergedReducedData(self._lastMergeIndex, self._lastMergeLabel)
        

    def doMergeScanView2D(self):
        """ Change the merged run's view to 2D plot
        """
        # Highlight button color and change the color of another one
        #self.ui.pushButton_view2D.setStyleSheet('QPushButton {background-color: #A3C1DA; color: red;}')
        #self.ui.pushButton_viewMerge.setStyleSheet('QPushButton {background-color: #A3C1DA; color: black;}')
        self.ui.pushButton_view2D.setStyleSheet('QPushButton {color: red;}')
        self.ui.pushButton_viewMerge.setStyleSheet('QPushButton {color: black;}')
        # Convert the workspaces to 2D vector
        vecylist = []
        yticklabels = []
        xmin = None
        xmax = None
        for ws in self._myControl.getWkspToMerge(): 
            # put y values to list for constructing 2D array
            vecy = ws.readY(0)
            vecylist.append(vecy)
            yticklabels.append(ws.name())
            
            # set up range of x
            if xmin is None:
                vecx = ws.readX(0)
                xmin = vecx[0]
                xmax = vecx[-1]
            
        # ENDFOR
        dim2array = numpy.array(vecylist)        

        print "2D vector: \n",  dim2array
        print "x range: %f, %f" % (xmin, xmax)
        print "y labels: ", yticklabels
         
        # Plot
        holdprev=False
        self.ui.graphicsView_mergeRun.clearAllLines()
        self.ui.graphicsView_mergeRun.addPlot2D(dim2array, xmin=xmin, xmax=xmax, ymin=0, \
            ymax=len(vecylist), holdprev=holdprev, yticklabels=yticklabels)

        return


    def doPlot2Theta(self):
        """ Rebin the data and plot in 2theta
        """
        self._uiRebinPlot('2theta')
        self._currUnit = '2theta'
        
        return

    def doPlotDspacing(self):
        """ Rebin the data and plot in d-spacing
        """
        # new unit and information
        newunit = "dSpacing"
        
        self._uiRebinPlot(newunit)
        self._currUnit = newunit
        
        return


    def doPlotIndvDet(self):
        """ Plot individual detector
        """
        # get exp and scan numbers and check whether the data has been loaded
        try:
            expno = self.getInteger(self.ui.lineEdit_expNo)
            scanno = self.getInteger(self.ui.lineEdit_scanNo)
        except EmptyError as e:
            self._logError(str(e))
            return

        # get detector ID and x-label option
        try:
            detid = self.getInteger(self.ui.lineEdit_detID)
        except EmptyError:
            self._logError("Detector ID must be specified for plotting individual detector.")
            return

        xlabel = str(self.ui.comboBox_indvDetXLabel.currentText())

        # plot
        try: 
            self._plotIndividualDetCounts(expno, scanno, detid, xlabel)
        except NotImplementedError as e:
            self._logError(str(e))

        return


    def doPlotQ(self):
        """ Rebin the data and plot in momentum transfer Q
        """
        newunit = 'Momentum Transfer (Q)'
        self._uiRebinPlot(unit =newunit)
        self._currUnit = newunit
        
        return


    def doPlotRawDet(self):
        """ Plot current raw detector signals
        """
        # get experiment number and scan number for data file
        try: 
            expno = self.getInteger(self.ui.lineEdit_expNo)
            scanno = self.getInteger(self.ui.lineEdit_scanNo)
        except EmptyError as e:
            self._logError(str(e))
            return

        # plot options
        doOverPlot = bool(self.ui.checkBox_overpltRawDet.isChecked())
        plotmode = str(self.ui.comboBox_rawDetMode.currentText())
        try:
            ptNo = self.getInteger(self.ui.lineEdit_ptNo)
        except EmptyError:
            ptNo = None

        # plot
        self._plotRawDetSignal(expno, scanno, plotmode, ptNo, doOverPlot)



    def doPlotPrevDetRaw(self):
        """ Plot previous raw detector
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            detno = self._detNo + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._detNo = detno
            self.ui.lineEdit_detNo.setText(str(self._detNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doPlotNextDetRaw(self):
        """ Plot next raw detector signals
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            detno = self._detNo + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._detNo = detno
            self.ui.lineEdit_detNo.setText(str(self._detNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doPlotPrevPtRaw(self):
        """ Plot previous raw detector
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            ptno = self._ptNo - 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._ptNo = ptno
            self.ui.lineEdit_ptNo.setText(str(self._ptNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doPlotNextPtRaw(self):
        """ Plot next raw detector signals
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            ptno = self._ptN + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._ptNo = ptno
            self.ui.lineEdit_ptNo.setText(str(self._ptNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doRebinD(self):
        """ Rebin MDEventWorkspaces in d-Spacing. for pushButton_rebinD
        in vanadium peak strip tab
        """
        self._uiRebinPlot(unit='dSpacing', xmin=self.ui.lineEdit_minD.text(),
            binsize=self.ui.lineEdit_binsizeD.text(), 
            xmax=self.ui.lineEdit_maxD.text(),
            canvas=self.ui.graphicsView_vanPeaks)
        return
        
        dminstr = str(self.ui.lineEdit_minD.text()).strip()  
        dmaxstr = str(self.ui.lineEdit_maxD.text()).strip()     
        dbinsizestr = str(self.ui.lineEdit_binsizeD.text()).strip()

        # dmin and dmax
        if len(dminstr) == 0 or len(dmaxstr) == 0:
            dmin = None
            dmax = None
        else:
            dmin = float(dminstr)
            dmax = float(dmaxstr)

        # bin size
        if len(dbinsizestr) == 0:
            self._logError("Bin size in d-spacing must be specified!")
            return
        else:
            binsize = float(dbinsizestr)

        # rebin
        self._rebin('dSpacing', dmin, binsize, dmax)
        
        self._plotReducedData(xlabel, 0, True)

        return


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
            filter = "All files (*.*);;Fullprof (*.dat);;GSAS (*.gsa)"
            sfilename = str(QtGui.QFileDialog.getSaveFileName(self, 'Save File', 
                homedir, filter))
        except NotImplementedError as e:
            self._logError(str(e))
        else:
            self._myControl.savePDFile(expno, scanno, filetype, sfilename)
            
        return
    
    def doSaveVanRun(self):
        """ Save the vanadium run with peaks removed
        """
        # TODO - Need to get use case from Clarina
        raise NotImplementedError("Need use case from instrument scientist")



    def doStripVandiumPeaks(self):
        """ Strip vanadium peaks
        """
        inputws = self._myReducedPDWs
        
        api.StripVanadiumPeaks(inputws, BackgroundType="Linear",
                WorkspaceIndex=0)

        self._plotVanadiumRun(xlabel, 0, True)

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
        x = event.xdata
        y = event.ydata
        button = event.button
        

        if x is not None and y is not None:

            if button == 1:
                msg = "You've clicked on a bar with coords:\n %f, %f\n and button %d" % (x, y, button)
                QtGui.QMessageBox.information(self, "Click!", msg)

            elif button == 3:
                # right click of mouse will pop up a context-menu
                self.ui.menu = QtGui.QMenu(self) 

                addAction = QtGui.QAction('Add', self) 
                addAction.triggered.connect(self.addSomething) 
                self.ui.menu.addAction(addAction)

                rmAction = QtGui.QAction('Remove', self) 
                rmAction.triggered.connect(self.rmSomething) 
                self.ui.menu.addAction(rmAction) 
                
                # add other required actions 
                self.ui.menu.popup(QtGui.QCursor.pos())

        return


    def getInteger(self, lineedit):
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


    def on_mouseMotion(self, event):
        """
        """
        prex = self._viewMerge_X
        prey = self._viewMerge_Y 
        
        curx = event.xdata
        cury = event.ydata
        if curx is None or cury  is None:
            return
        
        self._viewMerge_X = event.xdata
        self._viewMerge_Y = event.ydata
        
        if int(prey) != int(self._viewMerge_Y):
            print "Mouse is moving to ", event.xdata, event.ydata

        return


    def addSomething(self):
        """
        """
        print "Add something?"

        return


    def rmSomething(self):
        """
        """
        print "Remove something?"

        return


    #---------------------------------------------------------------------------
    # Private methods dealing with UI
    #---------------------------------------------------------------------------
    def _uiLoadDataFile(self, exp, scan):
        """ Load data file according to its exp and scan 
        Either download the data from a server or copy the data file from local 
        disk
        """
        # Get on hold of raw data file
        useserver = self.ui.radioButton_useServer.isChecked()
        uselocal = self.ui.radioButton_useLocal.isChecked()
        if (useserver and uselocal) is False:
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
                cachedir = os.getcwd()
                self.ui.lineEdit_cache.setText(cachedir)
                self._logWarning("Cache directory is not valid. \
                    Using current workspace directory %s as cache." % (cachedir) )
    
            filename = '%s_exp%04d_scan%04d.dat' % (self._instrument.upper(), exp, scan)
            self._srcFileName = os.path.join(cachedir, filename)
            status, errmsg = downloadFile(fullurl, self._srcFileName)
            if status is False:
                self._logError(errmsg)
                self._srcFileName = None
            else:
                rvalue = True

        elif self._srcAtLocal is True:
            # Data from local
            self._srcFileName = os.path.join(self._localSrcDataDir, "%s/Exp%d_Scan%04d.dat" % (self._instrument, exp, scan))
            if os.path.exists(self._srcFileName) is True:
                rvalue = True

        else:
            raise NotImplementedError("Logic error.  Neither downloaded from server.\
                Nor from local drive")

        return (rvalue,self._srcFileName)

    #--------------------------------------------------------------------------
    # Private methods to plot data
    #--------------------------------------------------------------------------

    def _plotIndividualDetCounts(self, expno, scanno, detid, xlabel):
        """ Plot a specific detector's counts along all experiment points (pt)
        """
        # load data if necessary
        if self._myControl.hasDataLoaded(expno, scanno) is False:
            rvalue, filename = self._uiLoadDataFile(expno, scanno)   
            if rvalue is False:
                self._logError("Unable to download or locate local data file for Exp %d \
                    Scan %d." % (expno, scanno))
                return
            self._myControl.loadSpicePDData(expno, scanno, filename)

        # pop out the xlabel list
        floatsamplelognamelist = self._myControl.getSampleLogNames(expno, scanno)
        self.ui.comboBox_indvDetXLabel.clear()
        self.ui.comboBox_indvDetXLabel.addItems(floatsamplelognamelist)

        
                
    def _plotRawDetSignal(self, expno, scanno, plotmode, ptno, dooverplot):
        """ Plot the counts of one detector of a certain Pt. in an experiment
        """
        # load data if necessary
        if self._myControl.hasDataLoaded(expno, scanno) is False:
            rvalue, filename = self._uiLoadDataFile(expno, scanno)   
            if rvalue is False:
                self._logError("Unable to download or locate local data file for Exp %d \
                    Scan %d." % (expno, scanno))
                return
            self._myControl.loadSpicePDData(expno, scanno, filename)

        # get vecx and vecy
        if plotmode == "All Pts.":
            # Plot all Pts.
            vecxylist = self._myControl.getRawDetectorCounts(expno, scanno)

            # Clear previous
            self.ui.graphicsView_Raw.clearAllLines()
            self.ui.graphicsView_Raw.setLineMarkerColorIndex(0)

        elif plotmode == "Single Pts.":
            # Plot plot
            if dooverplot is True:
                self.ui.graphicsView_Raw.clearAllLines()
                self.ui.graphicsView_Raw.setLineMarkerColorIndex(0)

            # Plot one pts.
            vecxylist = self._myControl.getRawDetectorCounts(expno, scanno, [ptno])

        else:
            # Raise exception
            raise NotImplementedError("Plot mode %s is not supported." % (plotmode))

        # plot
        canvas = self.ui.graphicsView_Raw
        unit = '$2\theta$'

        # plot
        xmin = 1.E10
        xmax = -1.0E10
        ymin = 1.E10
        ymax = -1.0E10
        for ptno, vecx, vecy in vecxylist:
            # FIXME - Label is left blank as there can be too many labels 
            label = str(ptno)
            marker, color = canvas.getNextLineMarkerColorCombo()
            canvas.addPlot(vecx, vecy, marker=marker, color=color, xlabel=unit, \
                    ylabel='intensity',label=label)
           
            # auto setup for image boundary
            tmpxmin = min(vecx)
            tmpxmax = max(vecx)
            if tmpxmin < xmin:
                xmin = tmpxmin
            if tmpxmax > xmax:
                xmax = tmpxmax

            tmpymax = max(vecy)
            tmpymin = min(vecy)
            if tmpymin < ymin:
                ymin = tmpymin
            if tmpymax > ymax:
                ymax = tmpymax
        # ENDFOR

        # set X-Y range
        dx = xmax-xmin
        dy = ymax-ymin
        canvas.setXYLimit(xmin-dx*0.1, xmax+dx*0.1, ymin-dy*0.1, ymax+dy*0.1)

        return


    def _plotMergedReducedData(self, mkey, label):
        """ Plot the reduced data from merged ...
        """
        # get the data
        try:
            vecx, vecy = self._myControl.getMergedVector(mkey)
        except Exception as e:
            self._logError("Unable to retrieve merged reduced data due to %s." % (str(e)))
            return

        canvas = self.ui.graphicsView_mergeRun

        # FIXME : shall be an option?
        clearcanvas = True
        if clearcanvas is True:
            canvas.clearAllLines()
            canvas.setLineMarkerColorIndex(0)

        # plot
        marker, color = canvas.getNextLineMarkerColorCombo()
        xlabel = self._getXLabelFromUnit(self._currUnit)

        canvas.addPlot(vecx, vecy, marker=marker, color=color, 
            xlabel=xlabel, ylabel='intensity',label=label)
            
        if clearcanvas is True:
            xmax = max(vecx)
            xmin = min(vecx)
            dx = xmax-xmin
            
            ymax = max(vecy)
            ymin = min(vecy)
            dy = ymax-ymin
            
            canvas.setXYLimit(xmin-dx*0.1, xmax+dx*0.1, ymin-dy*0.1, ymax+dy*0.1)

        return



    def _plotReducedData(self, exp, scan, canvas, xlabel, label=None, clearcanvas=True,
        spectrum=0):
        """ Plot reduced data for exp and scan
         self._plotReducedData(exp, scan, self.ui.canvas1, clearcanvas, xlabel=self._currUnit, 0, clearcanvas)
        """
        # FIXME - NEED TO REFACTOR TOO!
        # print "[DB] Plot reduced data!", "_inPlotState = ", str(self._inPlotState)

        # whether the data is load
        if self._myControl.hasReducedWS(exp, scan) is False:
            self._logWarning("No data to plot!")
            return
        
        # get to know whether it is required to clear the image
        if clearcanvas is True:
            canvas.clearAllLines()
            canvas.setLineMarkerColorIndex(0)
            #self.ui.graphicsView_reducedData.clearAllLines()
            #self._myLineMarkerColorIndex = 0
            
        # plot
        vecx, vecy = self._myControl.getVectorToPlot(exp, scan)
        
        
        # get the marker color for the line
        marker, color = canvas.getNextLineMarkerColorCombo()
        
        # plot
        if label is None:
            label = "Exp %d Scan %d" % (exp, scan)
            
        canvas.addPlot(vecx, vecy, marker=marker, color=color, 
            xlabel=xlabel, ylabel='intensity',label=label)
            
        if clearcanvas is True:
            xmax = max(vecx)
            xmin = min(vecx)
            dx = xmax-xmin
            
            ymax = max(vecy)
            ymin = min(vecy)
            dy = ymax-ymin
            
            canvas.setXYLimit(xmin-dx*0.1, xmax+dx*0.1, ymin-dy*0.1, ymax+dy*0.1)
            
        return


     
    def _uiCheckBinningParameters(self, curxmin=None, curxmax=None, curbinsize=None, curunit=None, targetunit=None):
        """ check the binning parameters including xmin, xmax, bin size and target unit
        
        Return: True or false
        """
        # get value
        xmin = str(self.ui.lineEdit_xmin.text())
        xmax = str(self.ui.lineEdit_xmax.text())
        binsize = str(self.ui.lineEdit_binsize.text())

        change = False        
        # check x-min
        if len(xmin) > 0:
            xmin = float(xmin)
            if ( (self._myMinX is None) or (self._myMinX is not None and abs(xmin-self._myMinX) > 1.0E-5) ):
                change = True
        else:
            xmin = None

        # check x-max
        if len(xmax) > 0: 
            xmax = float(xmax)
            if ( (self._myMaxX is None) or (self._myMaxX is not None and 
                abs(xmax-self._myMaxX) > 1.0E-5) ):
                change = True
        else:
            xmax = None
        
        # check binsize
        if len(binsize) > 0: 
            binsize = float(binsize)
            if ( (self._myBinSize is None) or (self._myBinSize is not None and 
                abs(binsize-self._myBinSize) > 1.0E-5) ):
                change = True
        else:
            binsize = None
        
        # whether the unit should be changed or bin be changed?
        if curunit != targetunit:
            change = True
            
        return (change, xmin, xmax, binsize)
        
    def _uiGetBinningParams(self, xmin_w=None, binsize_w=None, xmax_w=None):
        """ Get binning parameters
        
        Return: 
         - xmin, binsize, xmax
        """
        # get value
        if xmin_w is None:
            xmin = str(self.ui.lineEdit_xmin.text())
            xmax = str(self.ui.lineEdit_xmax.text())
            binsize = str(self.ui.lineEdit_binsize.text())
        else:
            xmin = str(xmin_w)
            xmax = str(xmax_w)
            binsize = str(binsize_w)
        
        # set data
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
        except ValueError as e:
            raise NotImplementedError("Error bins size cannot be left blank.")
            
        return (xmin, binsize, xmax)
        
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
        
    def _uiRebinPlot(self, unit, xmin=None, binsize=None, xmax=None, canvas=None):
        """ Rebin and plot by reading GUI widgets' value
        """
        # experiment number and scan number
        try:
            expno, scanno = self._uiGetExpScanNumber()
        except NotImplementedError as e:
            self._logError(str(e))
            return
        
        # binning parameters
        if binsize is None:
            try:    
                xmin, binsize, xmax = self._uiGetBinningParams()
            except NotImplementedError as e:
                self._logError(str(e))
                return
        else:
            xmin, binsize, xmax = self._uiGetBinningParams(xmin, binsize, xmax)
            
        # wavelength
        try: 
            wavelength = float(self.ui.lineEdit_wavelength.text())
        except ValueError as e:
            self._logError(str(e))
            return  
        
        # rebin
        try:
            rebinned = self._myControl.rebin(expno, scanno, unit, wavelength, xmin, binsize, xmax)
        except NotImplementedError as e:
            self._logError(str(e))
            return
        else:
            if rebinned:
                # plot if rebinned
                xlabel = self._getXLabelFromUnit(unit)
                
                # set up default canvas
                if canvas is None:
                    canvas = self.ui.graphicsView_reducedData
                    
                self._plotReducedData(expno, scanno, canvas, xlabel, 
                    label=None, clearcanvas=True)
            else:
                print "Rebinned = ", str(rebinned)

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
        print "Log(Error): %s" % (errinfo)


    def _logWarning(self, errinfo):
        """ Log error information
        """
        print "Log(Warning): %s" % (errinfo)
        

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


    def _getIntArray(self, intliststring):
        """ Validate whether the string can be divided into integer strings.
        Allowed: a, b, c-d, e, f
        """
        intliststring = str(intliststring)
        if intliststring == "":
            return []

        # Split by ","
        termlevel0s = intliststring.split(",")
        
        intlist = []

        # For each term
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
                        return "Contains non-integer string %s." % (valuestr)
                except ValueError:
                    return "String %s is not an integer." % (valuestr)
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
                            return "Contains non-integer string %s." % (valuestr)
                    except ValueError:
                        return "String %s is not an integer." % (valuestr)
                    else:
                        templist.append(intvalue)
                # ENDFOR
                intlist.extend(range(templist[0], templist[1]+1))

            else:
                # Undefined siutation
                return "Term %s contains more than 1 dash." % (level0terms)
        # ENDFOR

        return intlist
