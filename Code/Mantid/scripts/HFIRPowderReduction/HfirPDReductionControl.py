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
    #sys.path.append('/home/wzz/Mantid/Code/debug/bin')
    sys.path.append('/Users/wzz/Mantid/Code/debug/bin')
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
        
    def loadDataFile(self, expno, scanlist):
        """       
        Return :: datafilename (None for failed)
        """

    #---------------------------------------------------------------------------
        
    def mergeReduceSpiceData(self, expscanfilelist, unit, xmin, xmax, binsize, 
            wavelength):
        """ Merge and reduce SPICE data files
        Arguements:
         - expscanfilelist: list of 3 tuples: expnumber, scannumber and file name
        """
        # data structure initialization
        datamdwslist = []
        monitormdwslist = []

        # reduce individual data 
        scanlist = []
        for tuple3 in expscanfilelist:
            # get input tuple
            try:
                exp   = int(tuple3[0])
                scan  = int(tuple3[1])
                fname = tuple3[2]
                if os.path.exists(fname) is False:
                    raise NotImplementedError("Spice file %s cannot be found. " % (fname))
            except Exception as e:
                raise NotImplementedError("Invalid exp-scan-file list tuple. \
                        Reason: %s." % (str(e)))

            # reduce data
            rebingood = self.reduceSpicePDData(exp, scan, fname, unit, xmin, xmax, 
                    binsize, wavelength)

            if rebingood is True:
                wsmanager = self.getWorkspace(exp, scan, True)
                datamdwslist.append(wsmanager.datamdws)
                monitormdwslist.append(wsmanager.monitormdws)
            
                scanlist.append(scan)
        # ENDFOR

        # merge and rebin
        if len(datamdwslist) > 1: 
            mg_datamdws = datamdwslist[0] +  datamdwslist[1]
            mg_monitormdws = monitormdwslist[0] + monitormdwslist[1]
        else:
            mg_datamdws = datamdwslist[0] 
            mg_monitormdws = monitormdwslist[0] 
        for iw in xrange(2, len(datamdwslist)):
            mg_datamdws += datamdwslist[iw] 
            mg_monitormdws += monitormdwslist[iw]

        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        exp = int(expscanfilelist[0][0])
        outwsname = "Merged_Exp%d_Scan%s_%s" % (exp,
                str(expscanfilelist[0][1]), str(expscanfilelist[-1][1]))
        api.ConvertCWPDMDToSpectra(InputWorkspace=mg_datamdws,
                InputMonitorWorkspace=mg_monitormdws,
                OutputWorkspace=outwsname,
                BinningParams=binpar,
                UnitOutput=unit, 
                NeutronWaveLength=wavelength)
        moutws = AnalysisDataService.retrieve(outwsname)
        if moutws is None:
            raise NotImplementedError("Merge failed.")

        key = (exp, str(scanlist))
        self._myMergedWSDict[key] = moutws
        
        return key


    def rebin(self, exp, scan, unit, wavelength, xmin, binsize, xmax):
        """ Rebin the data MD workspace and monitor MD workspace for new bin parameter and/or 
        units
        Return - Boolean as successful or not
        """
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


    def reduceSpicePDData(self, exp, scan, datafilename, unit, xmin, xmax, binsize, wavelength):
        """ Reduce SPICE powder diffraction data. 
        Return - Boolean as reduction is successful or not
        """
        # base workspace name
        # print "base workspace name: ", datafilename
        try:
            basewsname = os.path.basename(datafilename).split(".")[0]
        except AttributeError as e:
            raise NotImplementedError("Unable to parse data file name due to %s." % (str(e)))

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
