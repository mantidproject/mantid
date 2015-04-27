############################################################################
#
# HFIR powder reduction control class
#
############################################################################
import sys
import os
import urllib2
import math

import numpy

import HfirUtility as hutil

# Import mantid
IMPORT_MANTID = False
try:
    import mantid
    IMPORT_MANTID = True
except ImportError as e:
    curdir = os.getcwd()
    libpath = os.path.join(curdir.split('Code')[0], 'Code/debug/bin')
    if os.path.exists(libpath) is False:
        libpath = os.path.join(curdir.split('Code')[0], 'Code/release/bin')
    sys.path.append(libpath)
    #print libpath
    #"/home/wzz/Mantid_Project/Mantid/Code/Mantid/scripts/HFIRPowderReduction"
    ##sys.path.append('/home/wzz/Mantid_Project/Mantid2/Code/release/bin')
    #sys.path.append('/home/wzz/Mantid/Code/debug/bin')
    #sys.path.append('/Users/wzz/Mantid/Code/debug/bin')
    try:
        import mantid
    except ImportError as e2:
        print "Unable to import Mantid: %s." % (str(e))
        raise e
    else:
        IMPORT_MANTID = True
finally:
    if IMPORT_MANTID is True:
        import mantid.simpleapi as api
        import mantid.kernel
        from mantid.simpleapi import AnalysisDataService
        from mantid.kernel import ConfigService


VanadiumPeakPositions = [0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768, 
        0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401]

""" Powder data reduction class
"""
class PDRManager:
    """ Powder diffraction reduction workspace manager
    """
    def __init__(self, exp, scan):
        """ Initialization
        """
        try:
            self.exp = int(exp)
            self.scan = int(scan)
        except ValueError as e:
            raise NotImplementedError("Set non-integer value as Exp and Scan to PDRManager.")

        self.unit = None
        self.datamdws = None
        self.monitormdws = None
        self.reducedws = None
        self.binsize = 1E10

        self._rawSpiceTableWS = None
        self._rawLogInfoWS = None
       
        # special
        self._processedVanWS = None

        self._wavelength = None
        

        return

    def getRawSpiceTable(self):
        """
        """
        return self._rawSpiceTableWS


    def getRawInfoMatrixWS(self):
        """
        """
        return self._rawLogInfoWS 

    def getVanadiumPeaks(self):
        """
        """
        return self._vanadiumPeakPosList[:]


    def getWavelength(self):
        """
        """
        return self._wavelength

    def setup(self, datamdws, monitormdws, reducedws=None, unit=None, binsize=None):
        """ Set up
        """
        self.datamdws = datamdws
        self.monitormdws = monitormdws
        if reducedws is not None:
            self.reducedws = reducedws
        if unit is not None:
            self.unit = unit
        try:
            self.binsize = float(binsize)
        except TypeError as e:
            print e
            pass

        return

    def setRawWorkspaces(self, spicetablews, logmatrixws):
        """ Set 2 raw SPICE workspaces
        """ 
        # Validate
        if  spicetablews.id() != 'TableWorkspace' or logmatrixws.id() != 'Workspace2D':
            raise NotImplementedError("Input workspaces for setRawWorkspaces() are not of correct types.")

        self._rawSpiceTableWS =  spicetablews
        self._rawLogInfoWS = logmatrixws

        return

    def setupMDWrokspaces(self, datamdws, monitormdws):
        """
        """
        self.datamdws = datamdws
        self.monitormdws = monitormdws

        return

    def setVanadiumPeaks(self, vanpeakposlist):
        """ Set up vanadium peaks in 2theta
        """
        # Validate input
        if isinstance(vanpeakposlist, list) is False:
            raise NotImplementedError("Input must be a list.  Now it it is %s." % (str(type(vanpeakposlist))))
        elif len(vanpeakposlist) == 0:
            raise NotImplementedError("Input must be a non-empty list.")

        vanpeakposlist = sorted(vanpeakposlist)
        if vanpeakposlist[0] < 5.:
            raise NotImplementedError("Check whether the list %s is in unit of 2theta" % (str(vanpeakposlist)))

        # Set up
        self._vanadiumPeakPosList = vanpeakposlist[:]

        return


    def setWavelength(self, wavelength):
        """ Set wavelength for this run
        """
        self._wavelength = float(wavelength)
        if wavelength <= 0:
            raise NotImplementedError("It is not physical to have negative neutron wavelength")

        return


""" HFIR powder diffraction data reduction control
"""
class HFIRPDRedControl:
    """ Class for controlling HFIR powder reduction
    """
    def __init__(self):
        """ Initialization
        """
        self._myWorkspaceDict = {}  # dictionary to manage all the workspaces reduced 
                                    # key = Exp/Scan
        self._myMergedWSDict = {}   # key = Exp/Scan list
        
        self._myWavelengthDict = {}

        self._lastWkspToMerge = []

        return


    def getIndividualDetCounts(self, exp, scan, detid, xlabel):
        """ Get individual detector counts
        """
        # Check and get data
        exp = int(exp)
        scan = int(scan)
        detid = int(detid)

        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            raise NotImplementedError("Exp %d Scan %d does not have reduced \
                    workspace." % (exp, scan))
        else:
            rmanager = self._myWorkspaceDict[(exp, scan)]

            datamdws = rmanager.datamdws
            monitormdws = rmanager.monitormdws
            
            if datamdws is None or monitormdws is None:
                raise NotImplementedError('Reduction manager has no MDEventWorkspaces setup.')
        # END-IF-ELSE

        # Get raw counts
        # FIXME : use **args 
        if xlabel is None:
            tempoutws = \
                    api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws, 
                                                    MonitorWorkspace=monitormdws, 
                                                    Mode='Detector', 
                                                    DetectorID = detid)
        else:
            tempoutws = \
                    api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws, 
                                                    MonitorWorkspace=monitormdws, 
                                                    Mode='Detector', 
                                                    DetectorID = detid, 
                                                    XLabel=xlabel)

        vecx = tempoutws.readX(0)[:]
        vecy = tempoutws.readY(0)[:]

        return (vecx, vecy)


    def getRawDetectorCounts(self, exp, scan, ptnolist=None):
        """ Return raw detector counts as a list of 3-tuples
        """
        # Check and get data
        exp = int(exp)
        scan = int(scan)

        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            raise NotImplementedError("Exp %d Scan %d does not have reduced \
                    workspace." % (exp, scan))
        else: 
            rmanager = self._myWorkspaceDict[(exp, scan)]
            datamdws = rmanager.datamdws
            monitormdws = rmanager.monitormdws
            
            if datamdws is None or monitormdws is None:
                raise NotImplementedError('Reduction manager has no MDEventWorkspaces setup.')
        # END-IF-ELSE

        # get the complete list of Pt. number
        if ptnolist is None:
            ptnolist = self._getRunNumberList(datamdws=rmanager.datamdws)

        rlist = []
        # Loop over all Pt. number
        for pt in ptnolist:
            # get data
            tempoutws = api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws, 
                                                        MonitorWorkspace=monitormdws, 
                                                        Mode='Pt.', 
                                                        Pt = pt)

            vecx = tempoutws.readX(0)[:]
            vecy = tempoutws.readY(0)[:]

            rlist.append((pt, vecx, vecy))
        # ENDFOR

        return rlist


    def getSampleLogNames(self, expno, scanno):
        """ Get the list of sample logs' names if they are 
        of float data type
        """
        # check
        if self._myWorkspaceDict.has_key((expno, scanno)) is False:
            raise NotImplementedError("Exp %d Scan %d does not have reduced \
                    workspace." % (exp, scan))

        # get data
        rmanager = self._myWorkspaceDict[(expno, scanno)]
        datamdws = rmanager.datamdws

        info0 = datamdws.getExperimentInfo(0)
        run = info0.run()
        plist = run.getProperties()
        lognamelist = []
        for p in plist:
            if p.__class__.__name__.lower().count('float') == 1:
                lognamelist.append(p.name)

        return lognamelist


    def getSampleLogValue(self, expno, scanno, samplelogname, xlabel):
        """ Get vecx and vecy for sample log 
        """
        # Check and get data
        exp = int(expno)
        scan = int(scanno)

        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            raise NotImplementedError("Exp %d Scan %d does not have reduced \
                    workspace." % (exp, scan))
        else: 
            rmanager = self._myWorkspaceDict[(exp, scan)]
            datamdws = rmanager.datamdws
            monitormdws = rmanager.monitormdws
            
            if datamdws is None or monitormdws is None:
                raise NotImplementedError('Reduction manager has no MDEventWorkspaces setup.')
        # END-IF-ELSE

        # get the complete list of Pt. number 
        ptnolist = self._getRunNumberList(datamdws=rmanager.datamdws)

        rlist = []
        # get data
        tempoutws = api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws, 
                                                    MonitorWorkspace=monitormdws, 
                                                    Mode='Sample Log',
                                                    SampleLogName=samplelogname)

        vecx = tempoutws.readX(0)[:]
        vecy = tempoutws.readY(0)[:]

        return (vecx, vecy)



    def getVectorToPlot(self, exp, scan):
        """ Get vec x and vec y of the reduced workspace to plot
        """
        # get on hold of reduced workspace
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        reducedws = wsmanager.reducedws
        if reducedws is None:
            raise NotImplementedError("Exp %d Scan %d does not have reduced workspace." % (exp, scan))

        # convert to point data if necessary
        if len(reducedws.readX(0)) != len(reducedws.readY(0)):
            wsname = reducedws.name() + "_pd"
            api.ConvertToPointData(InputWorkspace=reducedws, OutputWorkspace=wsname)
            outws = AnalysisDataService.retrieve(wsname)
        else:
            outws = reducedws

        # get vectors
        return outws.readX(0), outws.readY(0)
       

    def getVectorProcessVanToPlot(self, exp, scan):
        """ Get vec x and y for the processed vanadium spectrum
        """
        # get on hold of processed vanadium data workspace
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        procVanWs = wsmanager._processedVanWS
        if procVanWs is None:
            raise NotImplementedError("Exp %d Scan %d does not have processed vanadium workspace." % (exp, scan))

        # convert to point data if necessary
        if len(procVanWs.readX(0)) != len(procVanWs.readY(0)):
            wsname = procVanWs.name() + "_pd"
            api.ConvertToPointData(InputWorkspace=procVanWs, OutputWorkspace=wsname)
            outws = AnalysisDataService.retrieve(wsname)
        else:
            outws = procVanWs

        # get vectors
        return outws.readX(0), outws.readY(0)

    def getMergedVector(self, mkey):
        """
        """
        if self._myMergedWSDict.has_key(mkey) is True:
            ws = self._myMergedWSDict[mkey]
        
            # convert to point data if necessary
            if len(ws.readX(0)) != len(ws.readY(0)):
                wsname = ws.name() + "_pd"
                api.ConvertToPointData(InputWorkspace=ws, OutputWorkspace=wsname)
                ws = AnalysisDataService.retrieve(wsname)
            
            vecx = ws.readX(0)
            vecy = ws.readY(0)
        else:
            raise NotImplementedError("No merged workspace for key = %s." % (str(mkey)))

        return (vecx, vecy)

        
    def getVanadiumPeaksPos(self, exp, scan):
        """ Convert vanadium peaks from d-spacing to 2theta 
        Arguments
         - exp
         - scan
    
        Return :: list of peak positions in 2-theta (Degrees)
        """
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        if wsmanager.datamdws is None:
            self._logError("Unable to rebin the data for exp=%d, scan=%d because either data MD workspace and \
                monitor MD workspace is not present."  % (exp, scan))
            return False

        wavelength = wsmanager.getWavelength()

        # Convert the vanadium peaks' position from dSpacing to 2theta
        vanpeakpos2theta = []
        for peakpos in VanadiumPeakPositions:
            lambda_over_2d =  wavelength/2./peakpos
            if abs(lambda_over_2d) <= 1.:
                twotheta = math.asin(lambda_over_2d)*2.*180/math.pi 
                vanpeakpos2theta.append(twotheta)
            else:
                print "Vanadium peak %f is out of d-Spacing range." % (peakpos)

        vanpeakpos2theta = sorted(vanpeakpos2theta)
        wsmanager.setVanadiumPeaks(vanpeakpos2theta) 

        return vanpeakpos2theta

    def getWavelength(self, exp, scan):
        """ Get wavelength
        """
        exp = int(exp)
        scan = int(scan)
        return self._myWavelengthDict[(exp, scan)]
    
    def getWkspToMerge(self):
        """ Get the individual workspaces that are used for merging in the last
        merging-run activities
        """
        wslist = []
        for wsmanager in self._lastWkspToMerge:
            outws = wsmanager.reducedws
            wslist.append(outws)
        # ENDFOR (wsmanager)
        
        return wslist


    def getWorkspace(self, exp, scan, raiseexception):
        """
        """
        # get on hold of data
        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            if raiseexception is True: 
                raise NotImplementedError("Exp %d Scan %d has not been processed. " % (exp, scan))
            else:
                return None
        # ENDIF

        return self._myWorkspaceDict[(exp, scan)]


    def hasDataLoaded(self, exp, scan):
        """ Check whether an experiment data set (defined by exp No. and scan No.)
        has been loaded or not.
        """
        if self._myWorkspaceDict.has_key((exp, scan)):
            return True
        else:
            return False


    def hasReducedWS(self, exp, scan):
        """ Check whether an Exp/Scan has a reduced workspace
        """
        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            print self._myWorkspaceDict.keys()
            return False

        if self._myWorkspaceDict[(exp, scan)].reducedws is None:
            return False

        return True

        
    def loadSpicePDData(self, expno, scanno, datafilename):
        """ Load SPICE powder diffraction data to MDEventsWorkspaces
        """
        # Create base workspace name
        try:
            basewsname = os.path.basename(datafilename).split(".")[0]
        except AttributeError as e:
            raise NotImplementedError("Unable to parse data file name due to %s." % (str(e)))

        # load SPICE
        tablewsname = basewsname + "_RawTable"
        infowsname  = basewsname + "ExpInfo"
        api.LoadSpiceAscii(Filename=datafilename, 
                OutputWorkspace=tablewsname, RunInfoWorkspace=infowsname)

        tablews = AnalysisDataService.retrieve(tablewsname)
        infows  = AnalysisDataService.retrieve(infowsname)

        # Create a reduction manager and add workspaces to it
        wsmanager = PDRManager(expno, scanno)
        wsmanager.setRawWorkspaces(tablews, infows)
        self._myWorkspaceDict[ (int(expno), int(scanno) )] = wsmanager

        return


    def mergeReduceSpiceData(self, expno, scannolist, unit, xmin, xmax, binsize):
        """ Merge and reduce SPICE data files
        Arguements:
         - expscanfilelist: list of 3 tuples: expnumber, scannumber and file name
        """
        # Collect data MD workspaces and monitor MD workspaces
        datamdwslist = []
        monitormdwslist = []
        self._lastWkspToMerge = []

        print "[Checkpoint 0] Scans = ", str(scannolist)
        for scanno in sorted(scannolist): 
            try: 
                wsmanager = self.getWorkspace(expno, scanno, True) 
                datamdwslist.append(wsmanager.datamdws) 
                monitormdwslist.append(wsmanager.monitormdws)
                self._lastWkspToMerge.append(wsmanager)
            except Exception as e:
                print '[Error] Unable to retrieve MDWorkspaces for Exp %d Scan %d due to %s.' % (
                    expno, scanno, str(e))
                scannolist.remove(scanno)
        # ENDFOR
        
        print "[Checkpoing 1] Scans = ", str(scannolist)

        # Merge and binning
        if len(datamdwslist) > 1: 
            mg_datamdws = datamdwslist[0] +  datamdwslist[1]
            mg_monitormdws = monitormdwslist[0] + monitormdwslist[1]
        else:
            mg_datamdws = datamdwslist[0] 
            mg_monitormdws = monitormdwslist[0] 
        for iw in xrange(2, len(datamdwslist)):
            mg_datamdws += datamdwslist[iw] 
            mg_monitormdws += monitormdwslist[iw]

        # Set up binning parameters
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        # set up output workspace's name
        scannolist = sorted(scannolist)
        outwsname = "Merged_Exp%d_Scan%s_%s" % (expno, scannolist[0], scannolist[-1])

        # Merge
        wavelength = self.getWavelength(expno, scanno)
        api.ConvertCWPDMDToSpectra(InputWorkspace=mg_datamdws,
                                   InputMonitorWorkspace=mg_monitormdws,
                                   OutputWorkspace=outwsname,
                                   BinningParams=binpar,
                                   UnitOutput=unit, 
                                   NeutronWaveLength=wavelength)
        moutws = AnalysisDataService.retrieve(outwsname)
        if moutws is None:
            raise NotImplementedError("Merge failed.")

        key = (expno, str(scannolist))
        self._myMergedWSDict[key] = moutws
        
        return key


    def parseDetEffCorrFile(self, instrument, vancorrfname):
        """ Parse detector efficiency correction file='HB2A

        Return :: 2-tuple (table workspace and or 
        """
        if instrument.upper() == 'HB2A':
            vancorrdict, errmsg = hutil.parseDetEffCorrFile(vancorrfname)
            if len(vancorrdict) > 0: 
                detefftablews = self._generateTableWS(vancorrdict)
            else:
                detefftablews = None
        else:
            detefftablews = None
            errmsg = "Instrument %s is not supported for parsing vanadium (detector efficiency) correction."

        return (detefftablews, errmsg)


    def parseExcludedDetFile(self, instrument, excldetfname):
        """ Parse excluded detectors file

        Return :: 2 -tuple (list/None, error message)
        """
        if instrument.upper() == 'HB2A':
            excldetlist, errmsg = hutil.parseDetExclusionFile(excldetfname)
        else:
            raise NotImplementedError('Instrument %s is not supported for parsing excluded detectors file.'%(instrument))

        return excldetlist, errmsg


    def parseSpiceData(self, expno, scanno, detefftablews=None):
        """ Load SPICE data to MDWorkspaces
        """
        # Get reduction manager
        try: 
            wsmanager = self._myWorkspaceDict[ (int(expno), int(scanno) )]
        except KeyError:
            raise NotImplementedError("Exp %d Scan %d has not been loaded yet." % (int(expno),
                int(scanno)))

        # Convert to MDWorkspace
        tablews = wsmanager.getRawSpiceTable()
        infows  = wsmanager.getRawInfoMatrixWS()

        basewsname = tablews.name().split('_RawTable')[0]
        datamdwsname = basewsname + "_DataMD"
        monitorwsname = basewsname + "_MonitorMD"
        api.ConvertSpiceDataToRealSpace(InputWorkspace=tablews, 
                                        RunInfoWorkspace=infows, 
                                        OutputWorkspace=datamdwsname, 
                                        OutputMonitorWorkspace=monitorwsname,
                                        DetectorEfficiencyTableWorkspace=detefftablews)

        datamdws = AnalysisDataService.retrieve(datamdwsname)
        monitormdws = AnalysisDataService.retrieve(monitorwsname)

        if datamdws is None or monitormdws is None:
            raise NotImplementedError("Failed to convert SPICE data to MDEventWorkspaces \
                    for experiment %d and scan %d." % (expno, scanno))

        # Manager:
        wsmanager.setupMDWrokspaces(datamdws, monitormdws)
        self._myWorkspaceDict[(expno, scanno)] = wsmanager
        
        return True

        
    def rebin(self, exp, scan, unit, wavelength, xmin, binsize, xmax):
        """ Rebin the data MD workspace and monitor MD workspace for new bin parameter and/or 
        units
        Return - Boolean as successful or not
        """
        raise NotImplementedError('This method should be replaced by reduceSpicePD...')
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        if wsmanager.datamdws is None or wsmanager.monitormdws is None:
            self._logError("Unable to rebin the data for exp=%d, scan=%d because either data MD workspace and \
                monitor MD workspace is not present."  % (exp, scan))
            return False

        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        reducedwsname = wsmanager.datamdws.name() + "_" + unit
        api.ConvertCWPDMDToSpectra(InputWorkspace=wsmanager.datamdws, 
                                   InputMonitorWorkspace=wsmanager.monitormdws, 
                                   OutputWorkspace=reducedwsname, 
                                   UnitOutput=unit, 
                                   BinningParams = binpar, 
                                   NeutronWaveLength=wavelength)
        outws = AnalysisDataService.retrieve(reducedwsname)
        if outws is not None: 
            wsmanager.reducedws = outws 
            wsmanager.unit = unit
        else:
            raise NotImplementedError("Failed to convert unit to %s." % (unit))
        
        return True


    def reduceSpicePDData(self, exp, scan, unit, xmin, xmax, binsize, wavelength=None, excludeddetlist=[]):
        """ Reduce SPICE powder diffraction data. 
        Return - Boolean as reduction is successful or not
        """
        # Get reduction manager
        try: 
            wsmanager = self._myWorkspaceDict[(int(exp), int(scan))]
        except KeyError:
            raise NotImplementedError("SPICE data for Exp %d Scan %d has not been loaded." % (
                int(exp), int(scan)))

        datamdws = wsmanager.datamdws
        monitormdws = wsmanager.monitormdws

        # binning from MD to single spectrum ws
        # set up binning parameters
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        basewsname = datamdws.name().split("_DataMD")[0]
        outwsname = basewsname + "_Reduced"
        print "[DB]", numpy.array(excludeddetlist)
        api.ConvertCWPDMDToSpectra(InputWorkspace=datamdws,
                InputMonitorWorkspace=monitormdws,
                OutputWorkspace=outwsname,
                BinningParams=binpar,
                UnitOutput = unit, 
                NeutronWaveLength=wavelength,
                ExcludedDetectorIDs=numpy.array(excludeddetlist))

        print "[DB] Reduction is finished.  Data is in workspace %s. " % (outwsname)

        # Set up class variable for min/max and 
        outws = AnalysisDataService.retrieve(outwsname)
        if outws is None:
            raise NotImplementedError("Failed to bin the MDEventWorkspaces to MatrixWorkspace.")

        # Manager:
        wsmanager = PDRManager(exp, scan)
        wsmanager.setup(datamdws, monitormdws, outws, unit, binsize)
        wsmanager.setWavelength(wavelength)

        self._myWorkspaceDict[(exp, scan)] = wsmanager
        
        return True


    def retrieveCorrectionData(self, instrument, exp, scan, localdatadir):
        """ Retrieve including dowloading and/or local locating 
        powder diffraction's correction files

        Arguments:
         - instrument :: name of powder diffractometer in upper case
         - exp :: integer as epxeriment number
         - scan :: integer as scan number

        Return :: 2-tuple (True, list of returned file names) or (False, error reason)
        """
        if instrument.upper() == 'HB2A':
            # For HFIR HB2A only
            
            try: 
                wsmanager = self._myWorkspaceDict[(exp, scan)]
            except KeyError as e:
                raise e

            # Get parameter m1 and colltrans
            m1 = self._getValueFromTable(wsmanager.getRawSpiceTable(), 'm1')
            colltrans = self._getValueFromTable(wsmanager.getRawSpiceTable(), 'colltrans')

            # detector efficiency file
            try: 
                detefffname, deteffurl, wavelength = hutil.makeHB2ADetEfficiencyFileName(exp, m1, colltrans)
            except NotImplementedError as e:
                raise e
            if detefffname is not None: 
                localdetefffname = os.path.join(localdatadir, detefffname) 
                print "[DB] Detector efficiency file name: %s From %s" % (detefffname, deteffurl) 
                if os.path.exists(localdetefffname) is False: 
                    downloadFile(deteffurl, localdetefffname) 
                else: 
                    print "[Info] Detector efficiency file %s exists in directory %s." % (detefffname, localdatadir)
            else:
                localdetefffname = None
            # ENDIF

            # excluded detectors file
            excldetfname, exclurl = hutil.makeExcludedDetectorFileName(exp)
            localexcldetfname = os.path.join(localdatadir, excldetfname)
            print "[DB] Excluded det file name: %s From %s" % (excldetfname, exclurl)
            if os.path.exists(localexcldetfname) is False:
                downloadstatus, errmsg = downloadFile(exclurl, localexcldetfname)
                if downloadstatus is False:
                    localexcldetfname = None
                    print "[Error] %s" % (errmsg)
            else:
                print "[Info] Detector exclusion file %s exists in directory %s." % (excldetfname, localdatadir)

            # Set to ws manager
            wsmanager.setWavelength(wavelength)
            # wsmanager.setDetEfficencyFile()
            # wsmanager.setExcludedDetFile()

        else:
            # Other instruments
            raise NotImplementedError("Instrument %s is not supported to retrieve correction file." % (instrument))


        return (True, [wavelength, localdetefffname, localexcldetfname])



    def savePDFile(self, exp, scan, filetype, sfilename):
        """ Save a reduced workspace to gsas/fullprof/topaz data file
        """
        # get workspace
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        if wsmanager.reducedws is None:
            raise NotImplementedError("Unable to rebin the data for exp=%d, scan=%d because either data MD workspace and \
                monitor MD workspace is not present."  % (exp, scan))
        else:
            wksp = wsmanager.reducedws
   
        # save
        filetypes = filetypes.lower()
        if "gsas" in filetype:
            api.SaveGSS(InputWorkspace=wksp, Filename=sfilename, \
                SplitFiles=False, Append=False,\
                MultiplyByBinWidth=normalized, Bank=1, Format="SLOG",\
                 ExtendedHeader=True)
                 
        if "fullprof" in filetype:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=1, 
                Filename=sfilename)

        if "topas" in self._outTypes:
            api.SaveFocusedXYE(InputWorkspace=wksp, StartAtBankNumber=info["bank"],\
                Filename=filename+".xye", Format="TOPAS")
                
        return 


    def setWavelength(self, exp, scan, wavelength):
        """ Set wavelength for a specific scan
        """
        exp = int(exp)
        scan = int(scan)
        if wavelength == None: 
            self._myWavelengthDict[(exp, scan)] = None
        else: 
            self._myWavelengthDict[(exp, scan)] = float(wavelength)

        return


    def smoothVanadiumSpectrum(self, expno, scanno, smoothparams_str):
        """
        """ 
        outws = api.FFTSmooth(InputWorkspace=vanRun, 
                              OutputWorkspace=vanRun, 
                              Filter="Butterworth",
                              Params=smoothparams_str,
                              IgnoreXBins=True,
                              AllSpectra=True)

        return True


    def stripVanadiumPeaks(self, exp, scan, binparams, vanpeakposlist=None):
        """ Strip vanadium peaks 

        Arguments: 
         - binparams :: string as the list of xmin, binsize, xmax or just binsize
         - vanpeakposlist :: list of peak positions.  If none, then using default

        Return ::
        """
        # Get reduced workspace
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        wksp = wsmanager.reducedws
        if wksp is None:
            raise NotImplementedError("Unable to rebin the data for exp=%d, scan=%d because either data MD workspace and \
                monitor MD workspace is not present."  % (exp, scan))

        # Convert unit to Time-of-flight by rebinning
        xaxis_unit = wksp.getAxis(0).getUnit().unitID()
        if xaxis_unit != 'Degrees': 
            wksp = api.ConvertCWPDToSpectra(InputWorkspace=wksp, 
                                            OutputWorkspace=wksp.name(), 
                                            Params=binparams)

        # Vanadium peaks positions
        if vanpeakposlist is None or len(vanpeakposlist) == 0:
            vanpeakposlist = wsmanager.getVanadiumPeaks()
            if vanpeakposlist is None:
                raise NotImplementedError('No vanadium peaks has been set up.')
        # ENDIF

        
        outwsname = wksp.name()+"_rmVan"
        wksp = api.StripPeaks(InputWorkspace=wksp, 
                              OutputWorkspace=outwsname, 
                              PeakPositions=numpy.array(vanpeakposlist))

        # Store
        wsmanager._processedVanWS = wksp

        return True


    def _generateTableWS(self, vancorrdict):
        """ Create table workspace
        """
        tablews = api.CreateEmptyTableWorkspace(OutputWorkspace="tempcorrtable")
        tablews.addColumn('int', 'DetID')
        tablews.addColumn('double', 'Correction')

        for detid in sorted(vancorrdict.keys()):
            tablews.addRow( [detid, vancorrdict[detid]] )

        return tablews


    def _getRunNumberList(self, datamdws):
        """ Get list of run number (i.e., Pt) from an MDEventWorkspace

        Return :: list of MDEventWrokspace
        """
        ptnolist = []

        numexpinfo = datamdws.getNumExperimentInfo()
        for i in xrange(numexpinfo):
            expinfo = datamdws.getExperimentInfo(i)
            runid = expinfo.run().getProperty('run_number').value
            if runid >= 0:
                ptnolist.append(runid)
        # ENDFOR

        return sorted(ptnolist)


    def _getValueFromTable(self, tablews, colname, rowindex=0):
        """ Get value from a table workspace
        """
        colnames = tablews.getColumnNames()
        try: 
            colindex = colnames.index(colname)
            rvalue = tablews.cell(rowindex, colindex)
        except ValueError:
            rvalue = None

        return rvalue

""" External Methods """
def downloadFile(url, localfilepath):
    """
    Test: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'

    Arguments:
     - localfilepath :: local data file name with full path.
    """
    # open URL
    try: 
        response = urllib2.urlopen(url) 
        wbuf = response.read()
    except urllib2.HTTPError as e:
        # Unable to download file
        if str(e).count('HTTP Error 404') == 1:
            return (False, str(e))
        else: 
            raise NotImplementedError("Unable to download file from %s\n\tCause: %s." % (url, str(e)))
    # ENDIFELSE

    if wbuf.count('not found') > 0:
        return (False, "File cannot be found at %s." % (url))

    ofile = open(localfilepath, 'w')
    ofile.write(wbuf)
    ofile.close()

    return (True, "")


