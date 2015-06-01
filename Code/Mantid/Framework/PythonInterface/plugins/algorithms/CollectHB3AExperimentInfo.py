#pylint: disable=no-init,invalid-name,too-many-instance-attributes
import mantid
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os

class CollectHB3AExperimentInfo(PythonAlgorithm):
    """ Python algorithm to export sample logs to spread sheet file
    for VULCAN
    """

    def category(self):
        """ Category
        """
        return "Diffraction"

    def name(self):
        """ Algorithm name
        """
        return "CollectHB3AExperimentInfo"

    def summary(self):
        return "Collect HB3A experiment information for data reduction by ConvertCWSDToMomentum."

    def PyInit(self):
        """ Declare properties
        """
        # Input scan files
        # Format of the input should be 
        self.declareProperty("ExperimentNumber", -1, "Integer for experiment number.")

        self.declareProperty(IntArrayProperty("ScanList", [], Direction.Input),
                "List of scans of the experiment to be loaded.  It cannot be left blank.")

        self.declareProperty(InArrayProperty("PtLists", [], Direction.Input),
                "List of Pts to be loaded for scans specified by 'ScanList'. Each scan's Pt. must be started with -1 as a flag.  If no Pt. given, then all Pt. are all loaded.")

        self.declareProperty(FileProperty(name="DataDirectory",defaultValue="",action=FileAction.Directory))

        self.declareProperty("GetFileFromServer", False, "Obtain data file directly from neutrons.ornl.gov.")

        self.declarerPoperty("Detector2ThetaTolerance", 0.01, "Tolerance of 2 detector's 2theta to consider as being at same position.")


        ## Input workspace
        #self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
        #                     "Name of data workspace containing sample logs to be exported. ")

        ## Output file name
        #self.declareProperty(FileProperty("OutputFilename", "", FileAction.Save, [".txt"]),
        #                     "Name of the output sample environment log file name.")

        ## Sample log names
        #self.declareProperty(StringArrayProperty("SampleLogNames", values=[], direction=Direction.Input),
        #                     "Names of sample logs to be exported in a same file.")

        ## Header
        #self.declareProperty("WriteHeaderFile", False, "Flag to generate a sample log header file.")

        #self.declareProperty("Header", "", "String in the header file.")

        ## Time zone
        #timezones = ["UTC", "America/New_York", "Asia/Shanghai", "Australia/Sydney", "Europe/London", "GMT+0",\
        #        "Europe/Paris", "Europe/Copenhagen"]

        #description = "Sample logs recorded in NeXus files (in SNS) are in UTC time.  TimeZone " + \
        #    "can allow the algorithm to output the log with local time."
        #self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones), description)

        ## Log time tolerance
        #self.declareProperty("TimeTolerance", 0.01,
        #                     "If any 2 log entries with log times within the time tolerance, " + \
        #                     "they will be recorded in one line. Unit is second. ")

        return


    def PyExec(self):
        """ Main executor
        """
        # Read inputs
        self._getProperties()

        # Get scan list
        self._getExpScanPtLDict()

        # Get 2theta-(scan-pt) dictionary
        self._getDetectorPositionScanPtDict()

        # Determine the pixels' positions
        self._collectPixelsPositions()

        # Output
        self.setProperty("OutputWorkspace", self._myScanPtFileTableWS)
        self.setProperty("DetectorTableWorkspace", self._myPixelInfoTableWS)

        return


    def _getExpScanPtDict(self):
        """ Get the scan-Pt. dictionary
        """
        for iscan in self._scanList:
            # Loop over scan number
            scan = self._scanList[iscan]

            # Load data file
            spicefilename = 'HB3A_Exp%04d_Scan%4d.dat' % (self._expNo, scan)
            spicetablews = self._loadSpiceFile(spicefilename)
            if spicetablews is None:
                self.glog.warning("Unable to access Exp %d Scan %d's SPICE file %s." % (self._expNo, scan, spicefilename))

            # Get list of Pts.
            if len(self._ptListList[iscan]) == 0:
                # including all PC
                ptnumberlist = self._getAllPtFromTable(spicetablews)
                self._scanPtDict[scan] = ptnumberlist
            else:
                # specified from user
                self._scanPtDict[scan] = self._ptListList[iscan]

        # ENDFOR

        return


    def _getProperties(self):
        """ Get properties from user input
        """
        self._expNumber = getProperty("ExperimentNumer").value()
        self._scanList = getProperty("ScanList").value()
        rawptlist = self.getProperty("PtLists").value() 
        self._tol2Theta = getProperty("2ThetaTolerance").value()

        # process Pt number
        self._ptListList = []
        for i in xrange(len(rawptlist)):
            curpt = rawptlist[i]
            if curpt == -1:
                # begining of a scan
                sublist = []
                self._ptListList.append(sublist)
            else:
                # regular
                sublist.append(curpt)
            # ENDIF
        # ENDFOR

        # check
        if len(self._ptListList) != self._scanList:
            raise RuntimeError("Number of sub Pt. list is not equal to number of scans.")

        return


    def _getDetectorPositionScanPtDict(self):
        """ Get detector position (2theta) - Scan and pt number dictionary
        Dictionary: key  : 2theta value
                    value: list of 2-tuple as (scan, pt)
        """
        self._2thetaScanPtDict = {}

        # Scan every row of every SPICE table to get a dictionary with key as 2theta value
        for scannumber in self._spiceTableDict.keys():
            spicetable = self._spiceTableDict[scannumber]
            for irow in xrange(len(spicetable.rowCount())):
                ptnumber = spicetable.cell(irow, iColPtNumber)
                twotheta = spicetable.cell(irow, iCol2Theta)
                if self._2thetaScanPtDict.has_key(twotheta) is False:
                    self._2thetaScanPtDict[twotheta] = []
                self._2thetaScanPtDict[twotheta].append( (scannumber, ptnumber) ) # ENDFOR
        # ENDFOR

        self.log().notice("[DB] Number of 2theta entries = %d." % (len(self._2thetaScanPtDict.keys())))

        # Combine 2theta values within tolerance
        twothetalist = sorted(self._2thetaScanPtDict.keys())

        twotheta_prev = twothetalist[0]
        for itt in xrange(1, len(twothetalist)):
            twotheta_curr = twothetalist[itt]
            if twotheta_curr - twotheta_prev < self._2thetaTol:
                # two keys (2theta) are close enough, combine then 
                self._2thetaScanPtDict[twotheta_prev].extend(self._2thetaScanPtDict[twotheta_curr][:]) 
                del self._2thetaScanPtDict[twotehta] 
            else:
                # advanced to current 2theta and no more operation is required
                twotheta_prev = twotheta_curr
            # ENDIFELSE
        # ENDFOR

        return

    def _collectPixelsPositions(self):
        """ Get a list for pixels' information
        """
        for twotheta in sorted(self._2thetaScanPtDict.keys()):
            if len(self._2thetaScanPtDict[twotheta]) == 0:
                raise RuntimeError("Logic error to have empty list.")
            
            # Load detector counts file (.xml)
            scannumber, ptnumber = self._2thetaScanPtDict[twotehta] 
            dataws = self._loadHB3ADetCountFile(scannumber, ptnumber)

            self._detStartID[twotheta] = self._newDetID

            maxdetid = 0
            for iws in xrange(dataws.getNumberHistograms()):
                detector = dataws.getDetector(iws)
                detps = detector.getPos()
                newdetid = self._currStartDetID + detector.getID()
                if detector.getID() > maxdetid:
                    maxdetid = detector.getID()
                self._myPixelInfoTableWS.addRow([newdetid, detpos.X(), detpos.Y(), detpos.Z()])
            # ENDFOR (iws)

            self._currStartDetID += maxdetid
        # ENDFOR

        return


    def _getAllPtFromTable(self, spicetablews):
        """ Get all Pt. from a table
        """
        ptlist = []

        numrows = spicetablews.rowCount()
        for irow in xrange(numrows):
            pt = spicetablews.cell(irow, iColPt)
            ptlist.append(pt)

        return ptlist


    def _loadSpiceFile(self, spicefilename):
        """ Load SPICE file
        """
        spicetablews, spicematrixws = api.LoadSpiceAscii(Filename=spicefilename)

        return spicetablews



# Register algorithm with Mantid
AlgorithmFactory.subscribe(CollectHB3AExperimentInfo)
