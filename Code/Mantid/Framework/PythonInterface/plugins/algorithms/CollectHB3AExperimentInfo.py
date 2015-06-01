#pylint: disable=no-init,invalid-name,too-many-instance-attributes
import mantid
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os

iColPtNumber = 0
iCol2Theta = 10


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

        scanlistprop = mantid.kernel.IntArrayProperty("ScanList", [])
        self.declareProperty(scanlistprop, "List of scans of the experiment to be loaded.  It cannot be left blank")
        #self.declareProperty(IntArrayProperty("ScanList", [], Direction.Input),
        #        "List of scans of the experiment to be loaded.  It cannot be left blank.")

        self.declareProperty(mantid.kernel.IntArrayProperty("PtLists", []),
                "List of Pts to be loaded for scans specified by 'ScanList'. Each scan's Pt. must be started with -1 as a flag.  If no Pt. given, then all Pt. are all loaded.")

        self.declareProperty(FileProperty(name="DataDirectory",defaultValue="",action=FileAction.Directory))

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

        # Get scan list
        self._getExpScanPtDict()

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
        for iscan in xrange(len(self._scanList)):
            # Loop over scan number
            scan = self._scanList[iscan]

            # Load data file
            spicefilename = os.path.join(self._dataDir, 'HB3A_exp%04d_scan%04d.dat' % (self._expNumber, scan))
            spicetablews = self._loadSpiceFile(spicefilename)
            self._spiceTableDict[scan] = spicetablews
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

        # Define class variable Scan-Pt. dictionary: Key scan number, Value list of pt numbers
        self._scanPtDict = {}

        self._spiceTableDict = {}

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

            # check column names
            colnames = spicetable.getColumnNames()
            if colnames[iColPtNumber].lower().startswith('pt') is False or \
                    colnames[iCol2Theta].lower().startswith('2theta') is False:
                        raise NotImplementedError("Colulmn %d is not Pt. but %s; OR Column %d is  not 2theta, but %s." % (
                            iColPtNumber, colnames[iColPtNumber], iCol2Theta, colnames[iCol2Theta]))

            for irow in xrange(spicetable.rowCount()):
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
        for twotheta in sorted(self._2thetaScanPtDict.keys()):
            if len(self._2thetaScanPtDict[twotheta]) == 0:
                raise RuntimeError("Logic error to have empty list.")
            else:
                self.log().notice("[DB] Process pixels of detector centered at 2theta = %.5f." % (twotheta))
            
            # Load detector counts file (.xml)
            scannumber, ptnumber = self._2thetaScanPtDict[twotheta] 
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
        outwsname = os.path.basename(spicefilename).split(".")[0]
        self.log().notice("Loading SPICE file %s." % (spicefilename))
        spicetablews, spicematrixws = api.LoadSpiceAscii(Filename=spicefilename, OutputWorkspace=outwsname)
        self.log().notice("[DB] SPICE table workspace has %d rows."%(spicetablews.rowCount()))

        return spicetablews



# Register algorithm with Mantid
AlgorithmFactory.subscribe(CollectHB3AExperimentInfo)
