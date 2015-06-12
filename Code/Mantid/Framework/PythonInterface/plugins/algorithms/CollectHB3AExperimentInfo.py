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
    def __init__(self):
        """ Init
        """
        PythonAlgorithm.__init__(self)

        self._myPixelInfoTableWS = None
        self._myScanPtFileTableWS = None
        self._expNumber = -1
        self._scanList = None
        self._tol2Theta = None
        self._dataDir = None
        self._ptListList = None

        # Define class variable Scan-Pt. dictionary: Key scan number, Value list of pt numbers
        self._scanPtDict = {}
        self._spiceTableDict = {}
        self._detStartID = {}
        self._2thetaScanPtDict = {}
        self._scanPt2ThetaDict = {}

        self._currStartDetID = -999999999

        return

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

        scanlistprop = mantid.kernel.IntArrayProperty("ScanList", [])
        self.declareProperty(scanlistprop, "List of scans of the experiment to be loaded.  It cannot be left blank")
        #self.declareProperty(IntArrayProperty("ScanList", [], Direction.Input),
        #        "List of scans of the experiment to be loaded.  It cannot be left blank.")

        pd = "List of Pts to be loaded for scans specified by 'ScanList'. \
                Each scan's Pt. must be started with -1 as a flag.  \
                If no Pt. given, then all Pt. are all loaded."
        self.declareProperty(mantid.kernel.IntArrayProperty("PtLists", []), pd)

        self.declareProperty(FileProperty(name="DataDirectory",defaultValue="",action=FileAction.OptionalDirectory))

        self.declareProperty("GetFileFromServer", False, "Obtain data file directly from neutrons.ornl.gov.")

        self.declareProperty("Detector2ThetaTolerance", 0.01, "Tolerance of 2 detector's 2theta to consider as being at same position.")

        tableprop = mantid.api.ITableWorkspaceProperty("OutputWorkspace", "", mantid.kernel.Direction.Output)
        self.declareProperty(tableprop, "TableWorkspace for experiment number, scan, file name and starting detector IDs.")

        tableprop2 = mantid.api.ITableWorkspaceProperty("DetectorTableWorkspace", "", mantid.kernel.Direction.Output)
        self.declareProperty(tableprop2, "TableWorkspace for detector Id and information.")


        return


    def PyExec(self):
        """ Main executor
        """
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
        for scannumber in sorted(self._scanPtDict.keys()):
            for ptnumber in sorted(self._scanPtDict[scannumber]):
                self.log().debug("Keys for scanPt2ThetaDict: %s." %(str(self._scanPt2ThetaDict.keys())))
                twotheta = self._scanPt2ThetaDict[(scannumber, ptnumber)]
                startdetid = self._detStartID[twotheta]
                datafilename = 'HB3A_exp%d_scan%04d_%04d.xml'%(self._expNumber, scannumber, ptnumber)
                self._myScanPtFileTableWS.addRow([int(scannumber), int(ptnumber), str(datafilename), int(startdetid)])

        # Output
        self.setProperty("OutputWorkspace", self._myScanPtFileTableWS)
        self.setProperty("DetectorTableWorkspace", self._myPixelInfoTableWS)

        return

    def _createOutputWorkspaces(self):
        """
        """
        self._myPixelInfoTableWS = api.CreateEmptyTableWorkspace(OutputWorkspace=self.getPropertyValue('OutputWorkspace'))
        self._myPixelInfoTableWS.addColumn("int", "DetectorID")
        self._myPixelInfoTableWS.addColumn("double", "X")
        self._myPixelInfoTableWS.addColumn("double", "Y")
        self._myPixelInfoTableWS.addColumn("double", "Z")
        self._myPixelInfoTableWS.addColumn("int", "OriginalDetID")

        self._myScanPtFileTableWS = api.CreateEmptyTableWorkspace(OutputWorkspace=self.getPropertyValue('DetectorTableWorkspace'))
        self._myScanPtFileTableWS.addColumn("int", "Scan")
        self._myScanPtFileTableWS.addColumn("int", "Pt")
        self._myScanPtFileTableWS.addColumn("str", "Filename")
        self._myScanPtFileTableWS.addColumn("int", "StartDetID")

        return

    def _getExpScanPtDict(self):
        """ Get the scan-Pt. dictionary
        """
        for iscan in xrange(len(self._scanList)):
            # Loop over scan number
            scan = self._scanList[iscan]

            # Load data file
            spicefilename = os.path.join(self._dataDir, 'HB3A_exp%04d_scan%04d.dat' % (self._expNumber, scan))
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
        """ Get properties from user input
        """
        self._expNumber = self.getProperty("ExperimentNumber").value
        self._scanList =  self.getProperty("ScanList").value
        rawptlist = self.getProperty("PtLists").value
        self._tol2Theta = self.getProperty("Detector2ThetaTolerance").value
        self._dataDir = self.getProperty("DataDirectory").value

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
        if len(self._ptListList) != len(self._scanList):
            raise RuntimeError("Number of sub Pt. list is not equal to number of scans.")


        return


    def _getDetectorPositionScanPtDict(self):
        """ Get detector position (2theta) - Scan and pt number dictionary
        Dictionary: key  : 2theta value
                    value: list of 2-tuple as (scan, pt)
        """
        # Scan every row of every SPICE table to get a dictionary with key as 2theta value
        for scannumber in self._spiceTableDict.keys():
            spicetable = self._spiceTableDict[scannumber]
            requiredptnumbers = self._scanPtDict[scannumber]

            # check column names
            colnames = spicetable.getColumnNames()
            try:
                iColPtNumber = colnames.index('Pt.')
                iCol2Theta = colnames.index('2theta')
            except IndexError as e:
                raise IndexError("Either Pt. or 2theta is not found in columns: %d"%(str(e)))

            for irow in xrange(spicetable.rowCount()):
                ptnumber = spicetable.cell(irow, iColPtNumber)
                if ptnumber in requiredptnumbers:
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
            if twotheta_curr - twotheta_prev < self._tol2Theta:
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
        # Reset the current starting detector ID
        self._currStartDetID = 0

        for twotheta in sorted(self._2thetaScanPtDict.keys()):
            if len(self._2thetaScanPtDict[twotheta]) == 0:
                raise RuntimeError("Logic error to have empty list.")
            else:
                self.log().notice("[DB] Process pixels of detector centered at 2theta = %.5f." % (twotheta))

            # Load detector counts file (.xml)
            self.log().debug("Processing detector @ 2theta = %.5f, " % (twotheta))
            scannumber, ptnumber = self._2thetaScanPtDict[twotheta][0]
            self.log().debug("self._2thetaScanPtDict: %s"%(self._2thetaScanPtDict[twotheta]))
            dataws = self._loadHB3ADetCountFile(scannumber, ptnumber)

            self._scanPt2ThetaDict[(scannumber, ptnumber)] = twotheta

            self._detStartID[twotheta] = self._currStartDetID

            maxdetid = 0
            for iws in xrange(dataws.getNumberHistograms()):
                detector = dataws.getDetector(iws)
                detpos = detector.getPos()
                newdetid = self._currStartDetID + detector.getID()
                if detector.getID() > maxdetid:
                    maxdetid = detector.getID()
                self._myPixelInfoTableWS.addRow([newdetid, detpos.X(), detpos.Y(), detpos.Z(), detector.getID()])
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
        outwsname = os.path.basename(spicefilename).split(".")[0]
        self.log().notice("Loading SPICE file %s." % (spicefilename))
        spicetablews, spicematrixws = api.LoadSpiceAscii(Filename=spicefilename, OutputWorkspace=outwsname)
        self.log().debug("SPICE table workspace has %d rows."%(spicetablews.rowCount()))
        self.log().debug("SPICE matrix workspace %s is ignored."%(str(spicematrixws)))

        return spicetablews


    def _loadHB3ADetCountFile(self, scannumber, ptnumber):
        """ Load Spice XML file
        """
        xmlfilename = os.path.join(self._dataDir, "HB3A_exp%d_scan%04d_%04d.xml"%(self._expNumber, scannumber, ptnumber))
        outwsname = os.path.basename(xmlfilename).split('.')[0]

        self.log().notice("[DB] Load SPICE file %s to %s." % (xmlfilename, outwsname))
        dataws = api.LoadSpiceXML2DDet(Filename=xmlfilename, LoadInstrument=True,
                    OutputWorkspace=outwsname, DetectorGeometry='256,256')

        return dataws



# Register algorithm with Mantid
AlgorithmFactory.subscribe(CollectHB3AExperimentInfo)
