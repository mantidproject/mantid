############################################################################
#
# HFIR powder reduction control class
#
############################################################################
import sys
import os
import urllib2

# Import mantid
IMPORT_MANTID = False
try:
    import mantid
    IMPORT_MANTID = True
except ImportError as e:
    sys.path.append('/home/wzz/Mantid_Project/Mantid2/Code/release/bin')
    sys.path.append('/Users/wzz/Mantid/Code/debug/bin')
    try:
        import mantid
    except ImportError as e2:
        if NOMANTID is False: 
            print "Unable to import Mantid: %s." % (str(e))
            raise e
        else:
            print "NO MANTID IS USED FOR DEBUGGING PURPOSE."
            print sys.path
            IMPORT_MANTID = False
    else:
        IMPORT_MANTID = True
        NOMANTID = True
finally:
    if IMPORT_MANTID is True:
        import mantid.simpleapi as api
        import mantid.kernel
        from mantid.simpleapi import AnalysisDataService
        from mantid.kernel import ConfigService

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

        return

    def setup(self, datamdws, monitormdws, reducedws, unit, binsize):
        """ Set up
        """
        self.datamdws = datamdws
        self.monitormdws = monitormdws
        self.reducedws = reducedws
        self.unit = unit
        try: 
            self.binsize = float(binsize)
        except ValueError as e:
            print e
            pass

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

        return


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


    def hasReducedWS(self, exp, scan):
        """ Check whether an Exp/Scan has a reduced workspace
        """
        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            print self._myWorkspaceDict.keys()
            return False

        if self._myWorkspaceDict[(exp, scan)].reducedws is None:
            return False

        return True


    def rebin(self, exp, scan, unit, wavelength, xmin, binsize, xmax):
        """ Rebin the data MD workspace and monitor MD workspace for new bin parameter and/or 
        units
        """
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        if wsmanager.datamdws is None or wsmanager.monitormdws is None:
            self._logError("Unable to rebin the data for exp=%d, scan=%d because either data MD workspace and \
                monitor MD workspace is not present."  % (exp, scan))
            return

        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        reducedwsname = datamdws.name() + "_" + unit
        api.ConvertCWPDMDToSpectra(InputWorkspace=datamdws,
            InputMonitorWorkspace=monitormdws,
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
        
        return 


    def reduceSpicePDData(self, exp, scan, datafilename, unit, xmin, xmax, binsize, wavelength):
        """ Reduce SPICE powder diffraction data. 
        """
        # base workspace name
        # print "base workspace name: ", datafilename
        basewsname = os.path.basename(datafilename).split(".")[0]

        # load SPICE
        tablewsname = basewsname + "_RawTable"
        infowsname  = basewsname + "ExpInfo"
        api.LoadSpiceAscii(Filename=datafilename, 
                OutputWorkspace=tablewsname, RunInfoWorkspace=infowsname)

        # convert to MDWorkspace
        datamdwsname = basewsname + "_DataMD"
        monitorwsname = basewsname + "_MonitorMD"
        api.ConvertSpiceDataToRealSpace(InputWorkspace=tablewsname,
                RunInfoWorkspace=infowsname,
                OutputWorkspace=datamdwsname,
                OutputMonitorWorkspace=monitorwsname)

        datamdws = AnalysisDataService.retrieve(datamdwsname)
        monitormdws = AnalysisDataService.retrieve(monitorwsname)

        # binning from MD to single spectrum ws
        
        # set up binning parameters
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        outwsname = basewsname + "_Reduced"
        api.ConvertCWPDMDToSpectra(InputWorkspace=datamdwsname,
                InputMonitorWorkspace=monitorwsname,
                OutputWorkspace=outwsname,
                BinningParams=binpar,
                UnitOutput = unit, 
                NeutronWaveLength=wavelength)

        print "[DB] Reduction is finished.  Data is in workspace %s. " % (datamdwsname)

        # Set up class variable for min/max and 
        outws = AnalysisDataService.retrieve(outwsname)
        if outws is None:
            raise NotImplementedError("Failed to bin the MDEventWorkspaces to MatrixWorkspace.")

        # Manager:
        wsmanager = PDRManager(exp, scan)
        wsmanager.setup(datamdws, monitormdws, outws, unit, binsize)

        self._myWorkspaceDict[(exp, scan)] = wsmanager
        
        return True
       

""" External Methods """
def downloadFile(url, localfilepath):
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
