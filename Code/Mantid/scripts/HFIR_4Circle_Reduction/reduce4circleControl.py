################################################################################
#
# Controlling class 
#
################################################################################
import os
import sys
import urllib2

try:
    import mantid
except ImportError:
    # In case Mantid is not in the python path
    homedir = os.path.expanduser('~')
    mantiddir = os.path.join(os.path.join(homedir, 'Mantid/Code/debug/bin'))
    print 'Mantid Dir = %s' % mantiddir
    if os.path.exists(mantiddir) is False:
        raise RuntimeError('Mantid bin directory %s cannot be found.' % (mantiddir))

    # import again
    sys.path.append(mantiddir)
    import mantid
import mantid.simpleapi as api

         
DebugMode = True


class PeakInfo(object):
    """ Class containing a peak's information for GUI
    """
    def __init__(self):
        """ Init
        """
        # Flag for data storage mode
        self._cacheDataOnly = False
        # Data server's URL
        self._myServerURL = '' 
        # Local data cache directory
        self._localCacheDir = ''
        # Experiment number to process
        self._myExpNumber = -1
        # Flag to delete cache dir 
        self._rmdirFlag = False

        return


    def setPeak(self, peakws, rowindex):
        """ Set peak information from a peak workspace
        """


        raise NotImplementedError("ASAP")



class CWSCDReductionControl(object):
    """ Controlling class for reactor-based single crystal diffraction reduction
    """
    def __init__(self, instrumentname=None):
        """ init
        """
        if isinstance(instrumentname, str):
            self._instrumentName = instrumentname
        elif instrumentname is None:
            self._instrumentName = ''
        else:
            raise RuntimeError('Instrument name %s of type %s is not allowed.' % (str(instrumentname),
                str(type(instrumentname))))

        # Experiment number
        self._expNumber = 0
        self._dataDir = "/tmp"

        # Container for loaded workspaces 
        self._spiceTableDict = {}

        return


    def addPeakToCalUB(self, peakws, ipeak, matrixws):
        """ Add a peak to calculate ub matrix
        """
        # Get HKL
        try:
            h = float(int(matrixws.run().getProperty('_h').value))
            k = float(int(matrixws.run().getProperty('_k').value))
            l = float(int(matrixws.run().getProperty('_l').value))
        except Exception:
            return (False, "Unable to locate _h, _k or _l in input matrix workspace.")

        thepeak = peakws.getPeak(ipeak)
        thepeak.setHKL(h, k, l)
        

        return


    def calculateUBMatrix(self, peakws, a, b, c, alpha, beta, gamma):
        """ Calculate UB matrix
        """
        api.CalculateUMatrix(PeaksWorkspace=peakws,
                a=a,b=b,c=c,alpha=alpha,beta=beta,gamma=gamma)

        ubmatrix = peakws.sample().getOrientedLattice().getUB()
        print ubmatrix

        return


    def downloadSpiceFile(self, scanno):
        """ Download a scan/pt data from internet
        """
        # Generate the URL for SPICE data file
        spicerunfileurl = self._myServerURL + 'exp%d/Datafiles/'%(self._expNumber) + \
                "HB3A_exp%04d_scan%04d.dat" % (self._expNumber, scanno)

        spicefilename = '%s_exp%04d_scan%04d.dat'%(self._instrumentName, self._expNumber, 
                scanno)
        spicefilename = os.path.join(self._dataDir, spicefilename)

        # Download
        try:
            api.DownloadFile(Address=spicerunfileurl, Filename=spicefilename)
        except Exception as e:
            return (False, str(e))

        # Check file exist?
        if os.path.exists(spicefilename) is False:
            return (False, "Unable to locate downloaded file %s."%(spicefilename))

        return (True, spicefilename)


    def downloadSpiceDetXMLFile(self, scanno, ptno):
        """
        """
        # TODO - Doc
        # Generate the URL for XML file
        xmlfilename = '%s_exp%d_scan%04d_%04d.xml' % (self._instrumentName, 
                self._expNumber, scanno, ptno)
        xmlfileurl = '%sexp%d/Datafiles/%s' % (self._myServerURL, self._expNumber,
                xmlfilename)
        targetxmlfilename = os.path.join(self._dataDir, xmlfilename)

        # Download
        try:
            api.DownloadFile(Address=xmlfileurl, 
                             Filename=targetxmlfilename)
        except Exception as e:
            return (False, 'Unable to download Detector XML file %s dur to %s.' % (
                xmlfilename, str(e)))

        # Check file exist?
        if os.path.exists(targetxmlfilename) is False:
            return (False, "Unable to locate downloaded file %s."%(targetxmlfilename))

        return (True, targetxmlfilename)


    def downloadSelectedDataSet(self, scanlist):
        """ Download data set
        """
        for scanno in scanlist:
            # Form SPICE file
            status, retobj = self.downloadSpiceFile(scanno)

            # Reject if SPICE file cannot download
            if status is False:
                print retobj
                continue
            else:
                spicefilename = retobj

            # Load SPICE file
            spicetable = self.loadSpiceFile(scanno, spicefilename)
            ptnolist = self._getPtListFromTable(spicetable)
            for ptno in ptnolist:
                status, retobj = self.downloadSpiceDetXMLFile(scanno, ptno)
                if status is False:
                    print retobj
            # ENDFOR
        # ENDFOR (scanno)

        return


    def downloadAllDataSet(self):
        """ Download all data set
        """
        # FIXME 
        for scanno in xrange(1, sys.maxint):
            status, retobj = self.downloadSpiceFile(scanno)
            if status is False:
                break



        return


    def existDataFile(self, scanno, ptno):
        """ Check whether data file for a scan or pt number exists
        """
        # Check spice file 
        spicefilename = '%s_exp%04d_scan%04d.dat'%(self._instrumentName, self._expNumber, 
                scanno)
        spicefilename = os.path.join(self._dataDir, spicefilename)
        if os.path.exists(spicefilename) is False:
            return (False, "Spice data file %s cannot be found."%(spicefilename))

        # Check xml file
        xmlfilename = '%s_exp%d_scan%04d_%04d.xml'%(self._instrumentName, self._expNumber,
                scanno, ptno)
        xmlflename = os.path.join(self._dataDir, xmlfilename)
        if os.path.exists(xmlfilename) is False:
            return (False, "Pt. XML file %s cannot be found."%(xmlfilename))

        return (True, "")


    def findPeak(self, scanno, ptno):
        """ Find peak in sample Q space

        Return :: tuple as (boolean, object) such as (false, error message) and (true, PeakInfo object)
        """
        # Check existence
        runnumber = self._checkExist(scanno, ptno)
        if runnumber is None:
            return (False, "Unable to locate scan %s/pt %s" % (str(scanno), str(ptno)))

        # 
        mdwksp = slef._getMDWorkspace(runnumber)
        
        peakws = api.FindPeaksMD(InputWorkspace=mdwksp)

        # Get number of peaks
        numpeaks = peakws.rowCount()
        if numpeaks == 0:
            return (False, "Unable to find peak in scan %d/pt %d" % (scanno, ptno))
        elif numpeaks >= 2:
            raise NotImplementedError("It is not implemented for finding more than 1 peak.")

        # Ony case: number of peaks is equal to 1 
        self._storePeakWorkspace(peakws) 
        peakinfo = PeakInfo(peakws, 0) 

        return (True, peakinfo)

    def indexPeaks(self, srcpeakws, targetpeakws):
        """ Index peaks in a peak workspace by UB matrix

        Arguments: 
         - srcpeakws :: peak workspace containing UB matrix
         - targetpeakws :: peak workspace to get peaks indexed

        Return :: (Boolean, Object)
          - Boolean = True,  Object = ???
          - Boolean = False, Object = error message (str)
        """
        # Check input
        if isinstance(peakws, PeakWorkspace) is False:
            return (False, "Input workspace %s is not a PeakWorkspace" % (str(peakws)))

        # Get UB matrix 
        if DebugMode is True:
            ubmatrix = peakws.sample().getOrientedLattice().getUB()
            print "[DB] UB matrix is %s\n" % (str(ubmatrix))

        # Copy sample/ub matrix to target 
        api.CopySample(InputWorkspace=srcpeakws, 
                OutputWorkspace=str(targetpeakws), 
                CopyName=0, CopyMaterial=0, CopyEnvironment=0, 
                CopyShape=0,CopyLattice=1,CopyOrientationOnly=0)


        numindexed = CalculatePeaksHKL(PeaksWorkspace=targetpeakws, Overwrite=True)

        print "[INFO] There are %d peaks that are indexed." % (numindexed)

        return (True, "")


    def loadSpiceFile(self, scanno, spicefilename=None):
        """ 
        """
        # TODO - DOC!
        # Form stardard names
        if spicefilename is None:
            spicefilename = os.path.join(self._dataDir, 
                    'HB3A_exp%04d_scan%04d.dat'%(self._expNumber, scanno)) 
        outwsname = 'Table_Exp%d_Scan%04d'%(self._expNumber, scanno)

        # Load SPICE
        try:
            spicetablews, tempinfows = api.LoadSpiceAscii(Filename=spicefilename, 
                    OutputWorkspace=outwsname, RunInfoWorkspace='TempInfo')
        except RuntimeError as e:
            return (False, 'Unable to load SPICE data %s due to %s' % (spicefilename, str(e)))

        # Store
        self._spiceTableDict[scanno] = spicetablews

        return spicetablews


    def loadSpiceXMLPtFile(self, scanno, ptno):
        """ 
        """
        # TODO - Doc
        xlmfilename = '/home/wzz/Projects/MantidTests/Tickets/HB3A_UBMatrix/Datafiles/HB3A_exp355_scan%04d_%04d.xml'%(scanno, pt)
        LoadSpiceXML2DDet(Filename=xlmfilename, OutputWorkspace='s%04d_%04d'%(scanno, pt), DetectorGeometry='256,256', 
            SpiceTableWorkspace='Table_355_%04d'%(scanno), PtNumber=pt)
        # Rebin in momentum space: UB matrix only
        Rebin(InputWorkspace='s%04d_%04d'%(scanno, pt), OutputWorkspace='s%04d_%04d'%(scanno, pt), Params='6,0.01,6.5')
        # Set Goniometer
        SetGoniometer(Workspace='s%04d_%04d'%(scanno, pt), Axis0='_omega,0,1,0,-1', Axis1='_chi,0,0,1,-1', Axis2='_phi,0,1,0,-1')


    def importDataToMantid(self, scanno, ptno):
        """
        """
        # Check existence of SPICE workspace
        if self._spiceTableDict.has_key(scanno) is False:
            self.loadSpiceFile(scanno)

        # Check existence of matrix workspace 
        if self._xmlMatrixWSDict.has_key((scanno, ptno)) is False:
            self.loadSpiceXMLPtFile(scanno, ptno)


    def groupWS(self):
        """
        """
        inputws = ''
        for pt in [11]:
            
            inputws += 's%04d_%04d'%(scanno, pt)
            if pt != numrows:
                inputws += ','
        # ENDFOR
        
        GroupWorkspaces(InputWorkspaces=inputws, OutputWorkspace='Group_exp0355_scan%04d'%(scanno))



    def setServerURL(self, serverurl):
        """ Set server's URL
        """
        self._myServerURL = str(serverurl)
        if self._myServerURL.endswith('/') is False:
            self._myServerURL + '/'

        urlgood = False
        try:
            urllib2.urlopen(self._myServerURL)
        except urllib2.HTTPError, e:
            print(e.code) 
        except urllib2.URLError, e: 
            print(e.args)
        else:
            urlgood = True

        return urlgood


    def setWebAccessMode(self, mode):
        """ Set data access mode form server
        """
        if isinstance(mode, str) is False:
            raise RuntimeError('Input mode is not string')

        if mode == 'cache':
            self._cacheDataOnly = True
        elif mode == 'download':
            self._cacheDataOnly = False

        return


    def setLocalCache(self, localdir):
        """ Set local cache diretory
        """
        # TODO Doc : ..., localdir is ...

        # Get absolute path 
        if os.path.isabs(localdir) is True: 
            self._localCacheDir = localdir
        else:
           cwd = os.getcwd() 
           self._localCacheDir = os.path.join(cwd, localdir)

        # Create cache directory
        if os.path.exists(self._localCacheDir) is False:
            os.mkdir(self._localCacheDir)
            self._rmdirFlag = True

        # Synchronize with data directory
        self._dataDir = self._localCacheDir

        return os.path.exists(self._localCacheDir)


    def setExpNumber(self, expno):
        """ Set experiment number
        """
        self._expNumber = expno

        return

    """
    Private Methods
    """
    def _getPtListFromTable(self, spicetable):
        # TODO Doc
        """
        """
        numrows = spicetable.rowCount()
        ptlist = []
        for irow in xrange(numrows):
            ptno = int(spicetable.cell(irow, 0))
            ptlist.append(ptno)

        return ptlist
