# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-instance-attributes
import mantid
import mantid.simpleapi as api
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm
import os


class CollectHB3AExperimentInfo(PythonAlgorithm):
    """Python algorithm to export sample logs to spread sheet file
    for VULCAN
    """

    def __init__(self):
        """Init"""
        PythonAlgorithm.__init__(self)

        self._myPixelInfoTableWS = None
        self._myScanPtFileTableWS = None
        self._expNumber = -1
        self._scanList = None
        self._tol2Theta = None
        self._dataDir = None
        self._ptListList = None
        self._doGenerateVirtualInstrument = False
        self._numPixelsDetector = -1

        # Define class variable Scan-Pt. dictionary: Key scan number, Value list of pt numbers
        self._scanPtDict = {}
        self._spiceTableDict = {}
        self._detStartID = {}
        self._2thetaScanPtDict = {}
        self._scanPt2ThetaDict = {}
        self._monitorCountsDict = dict()  # key = 2-tuple (int, int) as scan number and pt number.
        self._expDurationDict = dict()  # key = 2-tuple (int, int) as scan number and pt number.

        self._currStartDetID = -999999999

        return

    def category(self):
        """Category"""
        return "Diffraction\\ConstantWavelength"

    def name(self):
        """Algorithm name"""
        return "CollectHB3AExperimentInfo"

    def summary(self):
        return "Collect HB3A experiment information for data reduction by ConvertCWSDToMomentum."

    def PyInit(self):
        """Declare properties"""
        # Input scan files
        # Format of the input should be
        self.declareProperty("ExperimentNumber", -1, "Integer for experiment number.")

        scanlistprop = mantid.kernel.IntArrayProperty("ScanList", [])
        self.declareProperty(scanlistprop, "List of scans of the experiment to be loaded.  It cannot be left blank")

        pd = "List of Pts to be loaded for scans specified by 'ScanList'. \
                Each scan's Pt. must be started with -1 as a flag.  \
                If no Pt. given, then all Pt. are all loaded."
        self.declareProperty(mantid.kernel.IntArrayProperty("PtLists", []), pd)

        self.declareProperty(FileProperty(name="DataDirectory", defaultValue="", action=FileAction.OptionalDirectory))

        self.declareProperty("GetFileFromServer", False, "Obtain data file directly from neutrons.ornl.gov.")

        self.declareProperty("Detector2ThetaTolerance", 0.01, "Tolerance of 2 detector's 2theta to consider as being at same position.")

        tableprop = mantid.api.ITableWorkspaceProperty("OutputWorkspace", "", mantid.kernel.Direction.Output)
        self.declareProperty(tableprop, "TableWorkspace for experiment number, scan, file name and starting detector IDs.")

        tableprop2 = mantid.api.ITableWorkspaceProperty("DetectorTableWorkspace", "", mantid.kernel.Direction.Output)
        self.declareProperty(tableprop2, "TableWorkspace for detector Id and information.")

        self.declareProperty(
            "GenerateVirtualInstrument",
            True,
            "If True, then the geometry of all the detectors will be written " "to DetectorTableWorkspace",
        )

        default_num_dets = 256 * 256
        self.declareProperty(
            "DetectorNumberPixels",
            default_num_dets,
            "Number of pixels on the detector. \
                             It is only required if GenerateVirtualInstrument is set to False.",
        )

        return

    def PyExec(self):
        """Main executor"""
        # Read inputs
        self._getProperties()

        # Create output workspaces
        self._createOutputWorkspaces()

        # Get scan list
        self._getExpScanPtDict()

        # Get 2theta-(scan-pt) dictionary
        self._getDetectorPositionScanPtDict()

        # Determine the pixels' positions
        self._collectPixelsPositions()

        # Set up ScanPtFileTable
        self.log().warning("Scan numbers are %s." % str(list(self._scanPtDict.keys())))
        self.log().warning("Keys for scanPt2ThetaDict: %s." % str(list(self._scanPt2ThetaDict.keys())))

        for scan_number in sorted(self._scanPtDict.keys()):
            self.log().warning("scan %d has Pt. as %s." % (scan_number, str(self._scanPtDict[scan_number])))

            start_det_id = None
            for pt_number in sorted(self._scanPtDict[scan_number]):
                # get start det id.  all Pt. belonged to same scan will use the same starting detector ID
                if start_det_id is None:
                    # get 2theta for starting det-ID
                    assert (scan_number, pt_number) in self._scanPt2ThetaDict, (
                        "Scan %d Pt %d cannot be "
                        "found in scan-pt-2theta dict, "
                        "whose keys are %s."
                        "" % (scan_number, pt_number, str(list(self._scanPt2ThetaDict.keys())))
                    )
                    two_theta = self._scanPt2ThetaDict[(scan_number, pt_number)]

                    assert two_theta in self._detStartID
                    start_det_id = self._detStartID[two_theta]

                # get detector counts file name and monitor counts
                data_file_name = "HB3A_exp%d_scan%04d_%04d.xml" % (self._expNumber, scan_number, pt_number)
                monitor_counts = self._monitorCountsDict[(scan_number, pt_number)]
                duration = self._expDurationDict[(scan_number, pt_number)]
                self._myScanPtFileTableWS.addRow(
                    [int(scan_number), int(pt_number), str(data_file_name), int(start_det_id), int(monitor_counts), float(duration)]
                )

        # Output
        self.setProperty("OutputWorkspace", self._myScanPtFileTableWS)
        self.setProperty("DetectorTableWorkspace", self._myPixelInfoTableWS)

        return

    def _createOutputWorkspaces(self):
        """ """
        self._myPixelInfoTableWS = api.CreateEmptyTableWorkspace(OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        self._myPixelInfoTableWS.addColumn("int", "DetectorID")
        self._myPixelInfoTableWS.addColumn("double", "X")
        self._myPixelInfoTableWS.addColumn("double", "Y")
        self._myPixelInfoTableWS.addColumn("double", "Z")
        self._myPixelInfoTableWS.addColumn("int", "OriginalDetID")

        self._myScanPtFileTableWS = api.CreateEmptyTableWorkspace(OutputWorkspace=self.getPropertyValue("DetectorTableWorkspace"))
        self._myScanPtFileTableWS.addColumn("int", "Scan")
        self._myScanPtFileTableWS.addColumn("int", "Pt")
        self._myScanPtFileTableWS.addColumn("str", "Filename")
        self._myScanPtFileTableWS.addColumn("int", "StartDetID")
        self._myScanPtFileTableWS.addColumn("int", "MonitorCounts")
        self._myScanPtFileTableWS.addColumn("float", "Duration")

        return

    def _getExpScanPtDict(self):
        """Get the scan-Pt. dictionary"""
        for iscan in range(len(self._scanList)):
            # Loop over scan number
            scan = self._scanList[iscan]

            # Load data file
            spicefilename = os.path.join(self._dataDir, "HB3A_exp%04d_scan%04d.dat" % (self._expNumber, scan))
            spicetablews = self._loadSpiceFile(spicefilename)
            self._spiceTableDict[scan] = spicetablews
            if spicetablews is None:
                self.glog.warning("Unable to access Exp %d Scan %d's SPICE file %s." % (self._expNumber, scan, spicefilename))

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
        """Get properties from user input"""
        self._expNumber = self.getProperty("ExperimentNumber").value
        self._scanList = self.getProperty("ScanList").value
        rawptlist = self.getProperty("PtLists").value
        self._tol2Theta = self.getProperty("Detector2ThetaTolerance").value
        self._dataDir = self.getProperty("DataDirectory").value
        self._doGenerateVirtualInstrument = self.getProperty("GenerateVirtualInstrument").value
        self._numPixelsDetector = self.getProperty("DetectorNumberPixels").value

        # process Pt number
        self._ptListList = []
        for i in range(len(rawptlist)):
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
        if len(self._ptListList) != len(self._scanList):
            raise RuntimeError("Number of sub Pt. list is not equal to number of scans.")

        if self._doGenerateVirtualInstrument is False:
            self._numPixelsDetector = int(self._numPixelsDetector)
            if self._numPixelsDetector < 0:
                raise RuntimeError(
                    "In case that virtual instrument is not created, number of pixels on "
                    "detector must be given. %d is not a valid number." % self._numPixelsDetector
                )
            elif self._numPixelsDetector < 256 * 256:
                self.log().warning("User defined number of detectors is less than 256 * 256. ")
        # END-IF

        return

    def _getDetectorPositionScanPtDict(self):
        """Get detector position (2theta) - Scan and pt number dictionary
        Dictionary: key  : 2theta value
                    value: list of 2-tuple as (scan, pt)
        """
        # Scan every row of every SPICE table to get a dictionary with key as 2theta value
        for scannumber in self._spiceTableDict:
            spicetable = self._spiceTableDict[scannumber]
            requiredptnumbers = self._scanPtDict[scannumber]

            # check column names
            colnames = spicetable.getColumnNames()
            try:
                iColPtNumber = colnames.index("Pt.")
                iCol2Theta = colnames.index("2theta")
                iColMonitor = colnames.index("monitor")
                iColTime = colnames.index("time")
            except IndexError as e:
                raise IndexError("Either Pt. or 2theta is not found in columns: %d" % (str(e)))

            for irow in range(spicetable.rowCount()):
                ptnumber = spicetable.cell(irow, iColPtNumber)
                if ptnumber in requiredptnumbers:
                    twotheta = spicetable.cell(irow, iCol2Theta)
                    monitor = spicetable.cell(irow, iColMonitor)
                    exp_time = spicetable.cell(irow, iColTime)
                    self._monitorCountsDict[(scannumber, ptnumber)] = monitor
                    self._expDurationDict[(scannumber, ptnumber)] = exp_time
                    if (twotheta in self._2thetaScanPtDict) is False:
                        self._2thetaScanPtDict[twotheta] = []
                    self._2thetaScanPtDict[twotheta].append((scannumber, ptnumber))
            # END-FOR
        # END-FOR

        self.log().notice("[DB] Number of 2theta entries = %d." % (len(list(self._2thetaScanPtDict.keys()))))

        # Combine 2theta values within tolerance
        twothetalist = sorted(self._2thetaScanPtDict.keys())

        twotheta_prev = twothetalist[0]
        for itt in range(1, len(twothetalist)):
            twotheta_curr = twothetalist[itt]
            if twotheta_curr - twotheta_prev < self._tol2Theta:
                # two keys (2theta) are close enough, combine then
                self._2thetaScanPtDict[twotheta_prev].extend(self._2thetaScanPtDict[twotheta_curr][:])
                del self._2thetaScanPtDict[twotheta_curr]
            else:
                # advanced to current 2theta and no more operation is required
                twotheta_prev = twotheta_curr
            # ENDIFELSE
        # ENDFOR

        return

    def _collectPixelsPositions(self):
        """Get a list for pixels' information"""
        # Reset the current starting detector ID
        self._currStartDetID = 0

        distinct_2theta_list = sorted(self._2thetaScanPtDict.keys())
        num_distinct_2theta = len(distinct_2theta_list)

        self.log().warning("Number of distinct 2theta is %d. They are %s." % (num_distinct_2theta, str(distinct_2theta_list)))

        for index, two_theta in enumerate(distinct_2theta_list):
            if len(self._2thetaScanPtDict[two_theta]) == 0:
                raise RuntimeError("Logic error to have empty list.")
            else:
                self.log().warning(
                    "[DB] For %d-th 2theta = %.5f. Number of Pts. is %d. "
                    "They are %s." % (index, two_theta, len(self._2thetaScanPtDict[two_theta]), str(self._2thetaScanPtDict[two_theta]))
                )

            # Get scan/pt and set dictionary
            self.log().debug("Processing detector @ 2theta = %.5f, " % (two_theta))
            for scannumber, ptnumber in self._2thetaScanPtDict[two_theta]:
                # scannumber, ptnumber = self._2thetaScanPtDict[two_theta][0]
                self.log().debug("self._2thetaScanPtDict: %s" % (self._2thetaScanPtDict[two_theta]))

                self._scanPt2ThetaDict[(scannumber, ptnumber)] = two_theta
                self._detStartID[two_theta] = self._currStartDetID

                if self._doGenerateVirtualInstrument is True:
                    # Load detector counts file (.xml)
                    dataws = self._loadHB3ADetCountFile(scannumber, ptnumber)

                    # write each detector's position and ID to table workspace
                    maxdetid = -1
                    for iws in range(dataws.getNumberHistograms()):
                        detector = dataws.getDetector(iws)
                        detpos = detector.getPos()
                        newdetid = self._currStartDetID + detector.getID()
                        if detector.getID() > maxdetid:
                            maxdetid = detector.getID()
                        self._myPixelInfoTableWS.addRow([newdetid, detpos.X(), detpos.Y(), detpos.Z(), detector.getID()])
                    # ENDFOR (iws)

                else:
                    # No need to generate virtual instrument information.
                    maxdetid = self._numPixelsDetector
                # END-IF-ELSE

                # Update start ID
                self._currStartDetID += maxdetid

            # END-FOR
        # ENDFOR

        return

    def _getAllPtFromTable(self, spicetablews):
        """Get all Pt. from a table"""
        ptlist = []

        numrows = spicetablews.rowCount()
        for irow in range(numrows):
            pt = spicetablews.cell(irow, iColPt)
            ptlist.append(pt)

        return ptlist

    def _loadSpiceFile(self, spicefilename):
        """Load SPICE file"""
        outwsname = os.path.basename(spicefilename).split(".")[0]
        self.log().notice("Loading SPICE file %s." % (spicefilename))
        spicetablews, spicematrixws = api.LoadSpiceAscii(Filename=spicefilename, OutputWorkspace=outwsname)
        self.log().debug("SPICE table workspace has %d rows." % (spicetablews.rowCount()))
        self.log().debug("SPICE matrix workspace %s is ignored." % (str(spicematrixws)))

        return spicetablews

    def _loadHB3ADetCountFile(self, scannumber, ptnumber):
        """Load Spice XML file"""
        xmlfilename = os.path.join(self._dataDir, "HB3A_exp%d_scan%04d_%04d.xml" % (self._expNumber, scannumber, ptnumber))
        outwsname = os.path.basename(xmlfilename).split(".")[0]

        self.log().notice("[DB] Load SPICE file %s to %s." % (xmlfilename, outwsname))
        dataws = api.LoadSpiceXML2DDet(Filename=xmlfilename, LoadInstrument=True, OutputWorkspace=outwsname, DetectorGeometry="256,256")

        return dataws


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CollectHB3AExperimentInfo)
