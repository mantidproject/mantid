#pylint: disable=invalid-name
################################################################################
#
# MainWindow application for reducing HFIR 4-circle 
#
################################################################################
import sys
import os
import numpy

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *


try:
    import mantid
except ImportError:
    sys.path.append('/home/wzz/Mantid/Code/debug/bin/')
    import mantid
finally:
    import mantid.simpleapi as api
    import mantid.kernel
    from mantid.simpleapi import AnalysisDataService
    from mantid.kernel import ConfigService


try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

from ui_MainWindow import Ui_MainWindow #import line for the UI python class

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

        # Mantid configuration
        config = ConfigService.Instance()
        self._instrument = config["default.instrument"]

        # Event handlings
        self.connect(self.ui.pushButton_load, QtCore.SIGNAL('clicked()'), self.doLoad)

        self.connect(self.ui.pushButton_testURLs, QtCore.SIGNAL('clicked()'),
                self.doTestURL)

        self.connect(self.ui.pushButton_plotScan, QtCore.SIGNAL('clicked()'),
                self.doPlotScanPt)

        self.connect(self.ui.pushButton_prevScan, QtCore.SIGNAL('clicked()'),
                self.doPlotPrevScanPt)

        self.connect(self.ui.pushButton_nextScan, QtCore.SIGNAL('clicked()'),
                self.doPlotNextScanPt)

        self.connect(self.ui.pushButton_browseLocalData, QtCore.SIGNAL('clicked()'),
                self.doBrowseLocalSrcDataDir)

        self.connect(self.ui.pushButton_browseSaveDir, QtCore.SIGNAL('clicked()'),
                self.doBrowseSaveDir)

        # Validator


        # Declaration of class variable
        self._runID = None
        self._expID = None
        self._currPt = None
        self._xmlwsbasename = None
        
        # Some configuration
        self._homeSrcDir = os.getcwd()
        self._homeSaveDir = os.getcwd()

        return

    #---------------------------------------------------------------------------
    # Event handling methods
    #---------------------------------------------------------------------------
    def doBrowseLocalSrcDataDir(self):
        """ Browse local source dir
        """
        srcdatadir = str(QtGui.QFileDialog.getExistingDirectory(self,'Get Directory',self._homeSrcDir))
        self._homeSrcDir = srcdatadir

        self.ui.lineEdit_localSrcDir.setText(srcdatadir)

        return


    def doBrowseSaveDir(self):
        """ Browse the local directory to save the data
        """
        targetdatadir = str(QtGui.QFileDialog.getExistingDirectory(self, 'Get Directory', self._homeSaveDir))
        self._homeSaveDir = targetdatadir

        self.ui.lineEdit_dirSave.setText(targetdatadir)

        return


    def doLoad(self):
        """ Download and optinally load the data
        """
        # get experiment and run 
        expid = int(self.ui.lineEdit_exp.text())
        runid = int(self.ui.lineEdit_run.text())

        workdir = str(self.ui.lineEdit_dirSave.text())

        # load mode
        uselocalfile = self.ui.checkBox_dataLocal.status()
        if uselocalfile == 0:
            uselocalfile = False
        else:
            uselocalfile = True

        # determine operation mode
        if uselocalfile is True:
            source = str(self.ui.lineEdit_localSrcDir.text())
            mode = ['Copy', 'Reduce']
        else:
            source = str(self.ui.lineEdit_url.text())
            modestr = str(self.ui.comboBox_mode.currentText())
            mode = ['Download']
            if modestr.count('Reduce') == 1:
                mode.append('Reduce')

        self._loadData(source, workdir, expid, runid, mode)

        return


    def doPlotScanPt(self):
        """ Plot the Pt. 
        """
        # get measurement pt and the file number
        pt = int(self.ui.lineEdit_ptPlot.text())

        xmlwsname = self._xmlwsbasename + "_%d" % (pt)
        if self._xmlwkspdict.has_key(xmlwsname) is False:
            self._logError('Pt %d does not does not exist.' % (pt))
        xmlws = self._xmlwkspdict[xmlwsname]
        self._currPt = xmlws

        self._plotRawXMLWksp(self._currPt)

        return

        
    def doPlotPrevScanPt(self):
        """ Plot the Pt. 
        """
        # get measurement pt and the file number
        curindex = self._wkspNameList.index(self._currPt)
        prevwsname = self._wkspNameList[curindex-1]
        self._currPt = prevwsname

        self._plotRawXMLWksp(self._currPt)

        return


    def doPlotNextScanPt(self):
        """ Plot the Pt. 
        """
        # get measurement pt and the file number
        nextindex = self._wkspNameList.index(self._currPt) + 1
        if nextindex == len(self._wkspNameList):
            nextindex = 0
        nextwsname = self._wkspNameList[nextindex]
        self._currPt = nextwsname

        self._plotRawXMLWksp(self._currPt)

        return


    def doTestURL(self):
        """ Test whether the root URL provided specified is good
        """
        url = str(self.ui.lineEdit_url.text())

        if isGoodURL is True:
            self.popOneButtonDialog("URL is good")
        else:
            self.popOneButtonDialog("Unable to access %s.  Check internet access." % (url))

        raise NotImplementedError("ASAP")


    #---------------------------------------------------------------------------
    # Private event handling methods
    #---------------------------------------------------------------------------
    def _loadData(self, source, targetdir, expid, runid, mode):
        """ Copy/download data to a directory and reduce them as an option
        Arguments:
         - source
         - targetdir
         - mode: 
        """
        basefilename =  "HB3A_exp%d_scan%0d.txt" % (expid, runid)
        localfilename = os.path.join(targetdir, basefilename)

        # load SPICE's run file
        if 'Download' in mode:
            # download from internet
            # generate the URL from 
            if source.endswith('/') is False:
                source = source+'/'
            spicerunfileurl = source + "HB3A_exp%d_scan%0d.txt" % (expid, runid)

            # download
            try:
                api.DownloadFile(Address=spicerunfileurl, Filename=localfilename)
            except Exception as e:
                return (False, str(e))

            # check file exist?
            if os.path.doesExist(localfilename) is False:
                return (False, "NO DOWNLOADED FILE")
            
        else:
            # copy from local disk
            # check whether the source and target directory are same
            source = os.path.absolutePath(source)
            targetdir = os.path.abosolutePath(targetdir)

            # copy file
            if source != targetdir:
                sourcefilename = os.path.join(source, basefilename)
                os.copyFile(sourcefilename, localfilename)

            # check file exist?
            if os.path.doesExist(localfilename) is False:
                return (False, "NO COPIED FILE")

        # ENDIFELSE

        # process SPICE's scan data file
        if 'Reduce' in mode:
            # load scan/run spice file
            spicetablews, infows = api.LoadSpiceAscii(Filename=localfilename, OutputWorkspace=spicetablewsname, 
                    RunInfoWorkspace=infowsname)

            # get Pt. data 
            ptlist = self._getPtList(spicetablews)

            self._xmlwkspdict = {} 
            for pt in ptlist:
                # generate xml file name
                basename = 'HB3A_exp%d_scan%04d_%04d.xml' % (expid, runid, pt)
                xmlfilename = os.path.join(targetdir, basename)
                if os.path.doesExist(xmlfilename) is False:
                    self._logError("File %s does not exist for exp %d scan %d pt %d" % (xmlfilename, expid, runid, pt))

                # load
                xmlwkspname = 'HB3A_e%d_s%d_m%d_raw' % (expid, runid, pt)
                xmlwksp = api.LoadSpiceXMLData(Filename=xmlfilename, OutputWorkspace=xmlwkspname)
                # FIXME - emit an signal?: for tree structure and log

                self._xmlwkspdict[pt] = xmlwksp
            # ENDFOR
        # ENDIF

        return
        

    def _plotRawXMLWksp(self, xmlws):
        """ Plot raw workspace from XML file for a measurement/pt.
        """
        # get data
        numspec = xmlws.getNumberHistograms()
        vecylist = []
        for iws in xrange(len(numspec)): 
            vecy = xmlws.readY(0)
            vecylist.append(vecy)
        # ENDFOR(iws)

        # plot 
        self._plot2DData(vecylist)

        return


