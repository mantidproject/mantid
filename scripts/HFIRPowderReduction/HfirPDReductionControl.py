#pylint: disable=too-many-lines,relative-import,invalid-name,too-many-instance-attributes,too-many-arguments
############################################################################
#
# HFIR powder reduction control class
# Key Words: FUTURE
#
############################################################################
import sys
import os
import urllib2
import math
import numpy

import HfirUtility as hutil

import mantid
import mantid.simpleapi as api
from mantid.simpleapi import AnalysisDataService

VanadiumPeakPositions = [0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,
                         0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401]


""" default configuration """
DEFAULT_SERVER = 'http://neutron.ornl.gov/user_data/'
DEFAULT_INSTRUMENT = 'hb2a'
DEFAULT_WAVELENGTH = 2.4100


class PDRManager(object):
    """ Powder diffraction reduction workspace manager
    """
    def __init__(self, exp, scan):
        """ Initialization
        """
        try:
            self.exp = int(exp)
            self.scan = int(scan)
        except ValueError:
            raise NotImplementedError("Set non-integer value as Exp and Scan to PDRManager.")

        self.unit = None
        self.datamdws = None
        self.monitormdws = None
        self.reducedws = None
        self.binsize = 1E10

        self._rawSpiceTableWS = None
        self._rawLogInfoWS = None

        # vanadium only
        self._processedVanWS = None
        self._processedVanWSTemp = None
        self._processVanNote = ""
        self._applySmoothVan = False
        self._vanadiumPeakPosList = []

        self._wavelength = None

        #register startup
        mantid.UsageService.registerFeatureUsage("Interface","HfirPowderReduction",False)

        return

    def applySmoothVanadium(self, smoothaccept):
        """ Apply the smoothing effect of to vanadium data
        """
        if isinstance(smoothaccept, bool) is False:
            raise NotImplementedError('Input for applySmoothVanadium() is not boolean!')

        self._applySmoothVan = smoothaccept

        return

    def getAverageMonitorCounts(self):
        """ Return the average monitor counts
        """
        # Check
        if self._rawSpiceTableWS is None:
            raise NotImplementedError('Raw SPICE TableWorkspace is None for scan %d, exp %d' % (
                self.exp, self.scan
            ))

        # Get the column index for monitor counts
        colnames = self._rawSpiceTableWS.getColumnNames()
        try:
            imonitor = colnames.index("monitor")
        except ValueError:
            raise RuntimeError("monitor is not a column name in SPICE table workspace.")

        # Sum and average
        numpts = self._rawSpiceTableWS.rowCount()
        totalcounts = 0
        for irow in xrange(numpts):
            moncounts = self._rawSpiceTableWS.cell(irow, imonitor)
            totalcounts += moncounts

        return float(totalcounts)/float(numpts)


    def getProcessedVanadiumWS(self):
        """
        """
        return self._processedVanWS

    def getProcessedVanadiumWSTemp(self):
        """
        """
        return self._processedVanWSTemp


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

    def isSmoothApplied(self):
        """
        """
        return self._applySmoothVan

    def setup(self, datamdws, monitormdws, reducedws=None, unit=None, binsize=None):
        """ Set up MDEventWorkspaces and reduction parameters
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

        return

    def set_raw_workspaces(self, spice_table_ws, log_matrix_ws):
        """ Set 2 raw SPICE workspaces
        """
        # Validate
        if  spice_table_ws.id() != 'TableWorkspace' or log_matrix_ws.id() != 'Workspace2D':
            raise NotImplementedError("Input workspaces for setRawWorkspaces() are not of correct types.")

        self._rawSpiceTableWS = spice_table_ws
        self._rawLogInfoWS = log_matrix_ws

        return

    def setupMDWrokspaces(self, datamdws, monitormdws):
        """
        """
        self.datamdws = datamdws
        self.monitormdws = monitormdws

        return

    def setProcessedVanadiumData(self, wksp):
        """ Set tempory processed vanadium data
        Arguments:
         - vanws :: workspace
         - note  :: string as note
        """
        self._processedVanWS = wksp

        return

    def setProcessedVanadiumDataTemp(self, vanws, note):
        """ Set tempory processed vanadium data
        Arguments:
         - vanws :: workspace
         - note  :: string as note
        """
        self._processedVanWSTemp = vanws
        self._processVanNote = note

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

#pylint: disable=too-many-public-methods
class HFIRPDRedControl(object):
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

        self._serverAddress = DEFAULT_SERVER
        self._instrument = DEFAULT_INSTRUMENT

        # spice file manager/lookup table
        self._spiceFileManager = dict()

        return

    def applySmoothVanadium(self, expno, scanno, applysmooth):
        """ Apply smoothed vanadium
        """
        if self._myWorkspaceDict.has_key((expno, scanno)) is False:
            raise NotImplementedError("Exp %d Scan %d does not have reduced \
                    workspace." % (expno, scanno))
        else:
            rmanager = self._myWorkspaceDict[(expno, scanno)]
            rmanager.applySmoothVanadium(applysmooth)

        return

    def getIndividualDetCounts(self, exp, scan, detid, xlabel, normalized=True):
        """ Get individual detector counts
        :param exp:
        :param scan:
        :param detid:
        :param xlabel:
        :param normalized:
        :return:
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
        # FUTURE: use **args
        if xlabel is None:
            tempoutws = \
                    api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws,
                                                    MonitorWorkspace=monitormdws,
                                                    Mode='Detector',
                                                    DetectorID = detid,
                                                    NormalizeByMonitorCounts=normalized)
        else:
            print "Plot detector %d's counts vs. sample log %s."%(detid, xlabel)
            tempoutws = \
                    api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws,
                                                    MonitorWorkspace=monitormdws,
                                                    Mode='Detector',
                                                    DetectorID = detid,
                                                    XLabel=xlabel,
                                                    NormalizeByMonitorCounts=normalized)

        vecx = tempoutws.readX(0)[:]
        vecy = tempoutws.readY(0)[:]

        return vecx, vecy

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
        for ptno in ptnolist:
            # get data
            tempoutws = api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws,
                                                        MonitorWorkspace=monitormdws,
                                                        Mode='Pt.',
                                                        Pt = ptno)

            vecx = tempoutws.readX(0)[:]
            vecy = tempoutws.readY(0)[:]

            rlist.append((ptno, vecx, vecy))
        # ENDFOR

        return rlist


    def getSampleLogNames(self, expno, scanno):
        """ Get the list of sample logs' names if they are
        of float data type
        """
        # check
        if self._myWorkspaceDict.has_key((expno, scanno)) is False:
            raise NotImplementedError("Exp %d Scan %d does not have reduced \
                    workspace." % (expno, scanno))

        # get data
        rmanager = self._myWorkspaceDict[(expno, scanno)]
        datamdws = rmanager.datamdws

        info0 = datamdws.getExperimentInfo(0)
        run = info0.run()
        plist = run.getProperties()
        lognamelist = []
        for prop in plist:
            if prop.__class__.__name__.lower().count('float') == 1:
                lognamelist.append(prop.name)

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
        # ptnolist = self._getRunNumberList(datamdws=rmanager.datamdws)

        # get data
        print "[DB] Plot sample log: XLabel = %s" % (xlabel)
        tempoutws = api.GetSpiceDataRawCountsFromMD(InputWorkspace=datamdws,
                                                    MonitorWorkspace=monitormdws,
                                                    Mode='Sample Log',
                                                    SampleLogName=samplelogname,
                                                    XLabel=xlabel)

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


    def getVectorProcessVanToPlot(self, exp, scan, tempdata=False):
        """ Get vec x and y for the processed vanadium spectrum
        """
        # get on hold of processed vanadium data workspace
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)

        if tempdata is True:
            procVanWs = wsmanager.getProcessedVanadiumWSTemp()
        else:
            procVanWs = wsmanager.getProcessedVanadiumWS()
            #procVanWs = wsmanager._processedVanWS

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
        """ Get vector X and Y from merged scans
        """
        if self._myMergedWSDict.has_key(mkey) is True:
            wksp = self._myMergedWSDict[mkey]

            # convert to point data if necessary
            if len(wksp.readX(0)) != len(wksp.readY(0)):
                wsname = wksp.name() + "_pd"
                api.ConvertToPointData(InputWorkspace=wksp, OutputWorkspace=wsname)
                wksp = AnalysisDataService.retrieve(wsname)

            vecx = wksp.readX(0)
            vecy = wksp.readY(0)
        else:
            raise NotImplementedError("No merged workspace for key = %s." % (str(mkey)))

        return (vecx, vecy)

    def get_vanadium_peak_positions(self, exp, scan):
        """ Convert vanadium peaks from d-spacing to 2theta
        Arguments
         - exp
         - scan

        Return :: list of peak positions in 2-theta (Degrees)
        """
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        if wsmanager.datamdws is None:
            err_msg = 'Unable to rebin the data for exp=%d, scan=%d because ' \
                      'either data MD workspace and monitor MD workspace is not present.' % (exp, scan)
            return False, err_msg

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

        return True, vanpeakpos2theta

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

    def get_log_name_list(self, exp_number, scan_number):
        """
        Get the list of log names, including sample logs and spice table column names
        :return: 2 tuple
        """
        # check input
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
   
        # FIXME/TODO - what are the functions of get_spice_table_ws...
        spice_table_ws = self.get_spice_table_ws(exp_number, scan_number, throw=True)
        spice_matrix_ws = self.get_spice_info_ws(exp_number, scan_number, throw=True)

        # get column names
        spice_col_names = spice_table_ws.getColumnNames()
        spice_log_names = list()
        for sample_log in spice_matrix_ws.getRun().getProperties():
            spice_log_names.append(sample_log.name)
        # END-FOR

        # FIXME/TODO/NOW: Expand from sample code below - Enable testing scan
        # LoadSpiceAscii(Filename='/home/wzz/Projects/workspaces/Mantid/HB2A/HB2A_exp0496_scan0055.dat', 
        # OutputWorkspace='HB2A_exp0496_scan0055_RawTable', RunInfoWorkspace='HB2A_exp0496_scan0055ExpInfo')
        # table_ws = mtd['HB2A_exp0496_scan0055_RawTable']
        # matrix_ws = mtd['HB2A_exp0496_scan0055ExpInfo']

        return spice_log_names, spice_col_names

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

    def get_spice_table_ws(self, exp_number, scan_number):
        """
        Get SPICE table workspace
        Args:
            exp_number:
            scan_number:

        Returns:

        """
        return self._myWorkspaceDict[(exp_number, scan_number)].get_spice_table()

    def do_load_spice_file(self, exp_number, scan_number):
        """
        Load SPICE powder diffraction data to MDEventsWorkspaces
        Args:
            exp_number:
            scan_number:

        Returns:

        """
        # check input's validity
        assert isinstance(exp_number, int), 'Experiment number %s must be an integer but not of %s.' \
                                            '' % (str(exp_number), type(exp_number))
        assert isinstance(scan_number, int), 'Scan number %s must be an integer but not %s.' \
                                             '' % (str(exp_number), type(exp_number))
        assert (exp_number, scan_number) in self._spiceFileManager, \
            'Exp %d Scan %d spice file has not been downloaded.' % (exp_number, scan_number)

        # get data file
        spice_file_name = self._spiceFileManager[(exp_number, scan_number)]

        # Create base workspace name
        try:
            base_ws_name = os.path.basename(spice_file_name).split(".")[0]
        except AttributeError as e:
            raise RuntimeError("Unable to parse data file name due to %s." % (str(e)))
        else:
            table_ws_name = base_ws_name + "_RawTable"
            info_ws_name  = base_ws_name + "ExpInfo"

        # load file
        api.LoadSpiceAscii(Filename=spice_file_name,
                           OutputWorkspace=table_ws_name,
                           RunInfoWorkspace=info_ws_name)

        tablews = AnalysisDataService.retrieve(table_ws_name)
        infows  = AnalysisDataService.retrieve(info_ws_name)
        if tablews is None or infows is None:
            raise NotImplementedError('Unable to retrieve either spice table workspace %s or log workspace %s' % (
                table_ws_name, info_ws_name))

        # Create a reduction manager and add workspaces to it
        ws_manager = PDRManager(exp_number, scan_number)
        ws_manager.set_raw_workspaces(tablews, infows)
        self._myWorkspaceDict[(exp_number, scan_number)] = ws_manager

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
            except KeyError as ne:
                print '[Error] Unable to retrieve MDWorkspaces for Exp %d Scan %d due to %s.' % (
                    expno, scanno, str(ne))
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
        wavelength = self.getWavelength(expno, scannolist[0])
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
        """ Load SPICE data to MDWorkspaces from raw table workspace
        """
        # Get reduction manager
        try:
            wsmanager = self._myWorkspaceDict[ (int(expno), int(scanno) )]
        except KeyError:
            raise NotImplementedError("Exp %d Scan %d has not been loaded yet." % (int(expno),int(scanno)))

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

    def reset_to_normalized(self, exp, scan_list, min_x, max_x, bin_size):
        """ Reset the scaled up data to normalized data
        :param exp:
        :param scan_list:
        :param min_x:
        :param max_x:
        :param bin_size:
        :return:
        """
        try:
            exp = int(exp)
        except ValueError as e:
            return False, str(e)

        for scan in scan_list:
            try:
                scan = int(scan)
                wsmanager = self._myWorkspaceDict[(exp, scan)]
            except ValueError:
                # type error, return with false
                return False, 'Scan number %s is not integer.'%(str(scan))
            except KeyError:
                # data has not been reduced yet. Reduce dat
                self.reduceSpicePDData(exp, scan, unit='2theta',
                                       xmin=min_x, xmax=max_x, binsize=bin_size)
                wsmanager = self._myWorkspaceDict[(exp, scan)]
            # END_TRY_EXCEPT

            # Reduce data if it is not reduced
            if wsmanager.reducedws is None:
                self.reduceSpicePDData(exp, scan, unit='2theta', xmin=min_x, xmax=max_x, binsize=bin_size)

            monitorcounts = wsmanager.getAverageMonitorCounts()
            print '[DB] Exp %d Scan %d: average monitor counts = %.5f' % (exp, scan, monitorcounts)
            # FUTURE: implement method ws_manager.reset_to_normalized() instead
            wsmanager.reducedws = wsmanager.reducedws / monitorcounts
        # END_FOR(scan)

        return True, ''

    def scale_to_raw_monitor_counts(self, exp, scan_list, min_x, max_x, bin_size):
        """ Scale up the reduced powder spectrum to its average monitor counts
        :param exp:
        :param scan_list:
        :param min_x:
        :param max_x:
        :param bin_size:
        :return:
        """
        try:
            exp = int(exp)
        except ValueError as e:
            return False, str(e)

        for scan in scan_list:
            try:
                scan = int(scan)
                wsmanager = self._myWorkspaceDict[(exp, scan)]
            except ValueError:
                # type error, return with false
                return False, 'Scan number %s is not integer.'%(str(scan))
            except KeyError:
                # data has not been reduced yet. Reduce dat
                self.reduceSpicePDData(exp, scan, unit='2theta',
                                       xmin=min_x, xmax=max_x, binsize=bin_size)
                wsmanager = self._myWorkspaceDict[(exp, scan)]
            # END_TRY_EXCEPT

            # Reduce data if it is not reduced
            if wsmanager.reducedws is None:
                self.reduceSpicePDData(exp, scan, unit='2theta', xmin=min_x, xmax=max_x, binsize=bin_size)

            monitorcounts = wsmanager.getAverageMonitorCounts()
            print '[DB] Exp %d Scan %d: average monitor counts = %.5f' % (exp, scan, monitorcounts)
            wsmanager.reducedws = wsmanager.reducedws * monitorcounts
        # END_FOR(scan)

        return True, ''

    def reduceSpicePDData(self, exp, scan, unit, xmin, xmax, binsize, wavelength=None,
                          excludeddetlist=None,scalefactor=None):
        """ Reduce SPICE powder diffraction data from MDEventWorkspaces
        Return - Boolean as reduction is successful or not
        """
        # Get reduction manager
        try:
            ws_manager = self._myWorkspaceDict[(int(exp), int(scan))]
        except KeyError:
            raise NotImplementedError("SPICE data for Exp %d Scan %d has not been loaded." % (
                int(exp), int(scan)))

        datamdws = ws_manager.datamdws
        monitormdws = ws_manager.monitormdws

        # binning from MD to single spectrum ws
        # set up binning parameters
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        # scale-factor
        if scalefactor is None:
            scalefactor = 1.
        else:
            scalefactor = float(scalefactor)
            print "[DB] Scale factor is %f." % (scalefactor)

        # Excluded detectors
        if excludeddetlist is None:
            excludeddetlist = []
        else:
            print "[DB] Excluded detectors: %s"%(excludeddetlist), "Convert to numpy array", \
                numpy.array(excludeddetlist)

        basewsname = datamdws.name().split("_DataMD")[0]
        outwsname = basewsname + "_Reduced"
        api.ConvertCWPDMDToSpectra(InputWorkspace=datamdws,
                                   InputMonitorWorkspace=monitormdws,
                                   OutputWorkspace=outwsname,
                                   BinningParams=binpar,
                                   UnitOutput = unit,
                                   NeutronWaveLength=wavelength,
                                   ExcludedDetectorIDs=numpy.array(excludeddetlist),
                                   ScaleFactor=scalefactor)

        print "[DB] Reduction is finished.  Data is in workspace %s. " % (outwsname)

        # Set up class variable for min/max and
        outws = AnalysisDataService.retrieve(outwsname)
        if outws is None:
            raise NotImplementedError("Failed to bin the MDEventWorkspaces to MatrixWorkspace.")

        # Manager:
        if self._myWorkspaceDict.has_key((exp, scan)) is False:
            raise NotImplementedError('Exp %d Scan %d has not been initialized.  ' % (exp, scan))
        # wsmanager = PDRManager(exp, scan)
        ws_manager = self._myWorkspaceDict[(exp, scan)]
        ws_manager.setup(datamdws, monitormdws, outws, unit, binsize)
        ws_manager.setWavelength(wavelength)

        # self._myWorkspaceDict[(exp, scan)] = wsmanager

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
                    download_file(deteffurl, localdetefffname)
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
                downloadstatus, errmsg = download_file(exclurl, localexcldetfname)
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

        return True, [wavelength, localdetefffname, localexcldetfname]

    def saveMergedScan(self, sfilename, mergeindex):
        """ Save the current merged scan
        """
        if self._myMergedWSDict.has_key(mergeindex) is True:
            wksp = self._myMergedWSDict[mergeindex]
        else:
            raise NotImplementedError('Unable to locate the merged scan workspace.')

        api.SaveFocusedXYE(InputWorkspace=wksp,
                           StartAtBankNumber=1,
                           Filename=sfilename)

        return

    def savePDFile(self, exp, scan, filetype, sfilename):
        """ Save a reduced workspace to gsas/fullprof/topaz data file
        """
        # get workspace
        wsmanager = self.getWorkspace(exp, scan, raiseexception=True)
        if wsmanager.reducedws is None:
            raise NotImplementedError("Unable to rebin the data for exp=%d, scan=%d because \
                    either data MD workspace and monitor MD workspace is not present."  % (exp, scan))
        else:
            wksp = wsmanager.reducedws

        # save
        filetype = filetype.lower()
        if "gsas" in filetype:
            if sfilename.endswith('.dat') is True:
                sfilename.replace('.dat', '.gsa')

            api.SaveGSS(InputWorkspace=wksp,
                        Filename=sfilename,
                        SplitFiles=False, Append=False,
                        MultiplyByBinWidth=False,
                        Bank=1,
                        Format="SLOG",
                        ExtendedHeader=True)
        # ENDIF

        if "fullprof" in filetype:
            if sfilename.endswith('.gsa') is True:
                sfilename.replace('.gsa', '.dat')

            api.SaveFocusedXYE(InputWorkspace=wksp,
                               StartAtBankNumber=1,
                               Filename=sfilename)
        # ENDIF

        if "topas" in filetype:
            sfilename = sfilename[:-4]+".xye"
            api.SaveFocusedXYE(InputWorkspace=wksp,
                               StartAtBankNumber=info["bank"],
                               Filename=sfilename,
                               Format="TOPAS")
        # ENDIF

        return


    def saveProcessedVanadium(self, expno, scanno, savefilename):
        """ Save processed vanadium data
        """
        # Get workspace
        wsmanager = self.getWorkspace(expno, scanno, raiseexception=True)

        if wsmanager.isSmoothApplied() is True:
            wksp = wsmanager.getProcessedVanadiumWSTemp()
        else:
            wksp = wsmanager.getProcessedVanadiumWS()

        # Save
        api.SaveFocusedXYE(InputWorkspace=wksp,
                           StartAtBankNumber=1,
                           Filename=savefilename)

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
        # Get reduced workspace
        wsmanager = self.getWorkspace(expno, scanno, raiseexception=True)
        vanRun = wsmanager.getProcessedVanadiumWS()
        outws = vanRun.name()+"_smooth"

        outws = api.FFTSmooth(InputWorkspace=vanRun,
                              OutputWorkspace=outws,
                              Filter="Butterworth",
                              Params=smoothparams_str,
                              IgnoreXBins=True,
                              AllSpectra=True)

        if outws is not None:
            wsmanager.setProcessedVanadiumDataTemp(outws, "FFT smooth")

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
        wsmanager.setProcessedVanadiumData(wksp)

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

    def download_spice_file(self, exp_number, scan_number, cache_dir):
        """
        Download SPICE file from HB2A data server
        Args:
            exp_number:
            scan_number:
            cache_dir:

        Returns: 2-tuple: boolean, string (file name or error message)

        """
        # check
        assert os.path.exists(cache_dir), 'Cache file directory %s does not exist.' % cache_dir

        spice_url = "%s%s/exp%d/Datafiles/%s_exp%04d_scan%04d.dat" % (self._serverAddress,
                                                                      self._instrument.lower(),
                                                                      exp_number,
                                                                      self._instrument.upper(),
                                                                      exp_number,
                                                                      scan_number)
        print "[DB] SPICE file URL: ", spice_url

        spice_base_name = '%s_exp%04d_scan%04d.dat' % (self._instrument.upper(), exp_number, scan_number)
        spice_full_path = os.path.join(cache_dir, spice_base_name)
        status, errmsg = download_file(spice_url, spice_full_path)

        if status:
            self._spiceFileManager[(exp_number, scan_number)] = spice_full_path
        else:
            self._spiceFileManager[(exp_number, scan_number)] = None

        return status, errmsg

    def locate_local_spice_file(self, exp_number, scan_number, local_dir):
        """
        Locate the local-stored SPICE file
        Args:
            exp_number:
            scan_number:
            local_dir:

        Returns:

        """
        assert isinstance(local_dir, str), 'Local data file directory %s must be a string but not %s.' \
                                           '' % (str(local_dir), type(local_dir))

        # assemble the full path to the SPICE file
        spice_file_path =  os.path.join(local_dir, "%s/Exp%d_Scan%04d.dat" % (self._instrument, exp_number,
                                                                              scan_number))
        if os.path.exists(spice_file_path) is True:
            rvalue = True
            self._spiceFileManager[(exp_number, scan_number)] = spice_file_path
        else:
            rvalue = False
            self._spiceFileManager[(exp_number, scan_number)] = None

        return rvalue


#-------------------------------------------------------------------------------
# External Methods
#-------------------------------------------------------------------------------
def download_file(url, file_name):
    """
    Test: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
    Args:
        url:
        file_name:

    Returns: 2-tuple as (boolean, string)

    """
    # check input
    assert isinstance(url, str), 'URl %s must be a string but not %s.' \
                                 '' % (str(url), type(url))
    assert isinstance(file_name, str), 'File name %s must be a string but not %s.' \
                                       '' % (str(file_name), type(file_name))

    try:
        api.DownloadFile(Address=url, Filename=file_name)
    except RuntimeError as run_err:
        return False, 'Unable to download file from %s: %s' % (url, str(run_err))

    return True, ''


