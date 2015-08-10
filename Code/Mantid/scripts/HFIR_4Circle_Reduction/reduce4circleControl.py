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
    def __init__(self, instrument_name=None):
        """ init
        """
        if isinstance(instrument_name, str):
            self._instrumentName = instrument_name
        elif instrument_name is None:
            self._instrumentName = ''
        else:
            raise RuntimeError('Instrument name %s of type %s is not allowed.' % (str(instrument_name),
                                                                                  str(type(instrument_name))))

        # Experiment number, data storage
        self._expNumber = 0

        self._dataDir = "/tmp"
        self._localCacheDir = None

        self._myServerURL = ''

        # Container for loaded workspaces 
        self._spiceTableDict = {}
        # A dictionary to manage all loaded and processed MDEventWorkspaces
        self._expDataDict = {}

        return

    def addPeakToCalUB(self, peakws, ipeak, matrixws):
        """ Add a peak to calculate ub matrix
        :param peakws:
        :param ipeak:
        :param matrixws:
        :return:
        """
        # Get HKL
        try:
            h = float(int(matrixws.run().getProperty('_h').value))
            k = float(int(matrixws.run().getProperty('_k').value))
            l = float(int(matrixws.run().getProperty('_l').value))
        except ValueError:
            return (False, "Unable to locate _h, _k or _l in input matrix workspace.")

        thepeak = peakws.getPeak(ipeak)
        thepeak.setHKL(h, k, l)

        return

    def calculateUBMatrix(self, peakws, a, b, c, alpha, beta, gamma):
        """
        Calculate UB matrix
        :param peakws:
        :param a:
        :param b:
        :param c:
        :param alpha:
        :param beta:
        :param gamma:
        :return:
        """
        api.CalculateUMatrix(PeaksWorkspace=peakws,
                a=a,b=b,c=c,alpha=alpha,beta=beta,gamma=gamma)

        ubmatrix = peakws.sample().getOrientedLattice().getUB()
        print ubmatrix

        return

    def download_spice_file(self, scan_number):
        """
        Download a scan/pt data from internet
        :param scan_number:
        :return:
        """
        # Generate the URL for SPICE data file
        file_url = '%sexp%d/Datafiles/HB3A_exp%04d_scan%04d.dat' % (self._myServerURL, self._expNumber,
                                                                    self._expNumber, scan_number)
        file_name = '%s_exp%04d_scan%04d.dat' % (self._instrumentName, self._expNumber, scan_number)
        file_name = os.path.join(self._dataDir, file_name)

        # Download
        try:
            api.DownloadFile(Address=file_url, Filename=file_name)
        except Exception as e:
            return False, str(e)

        # Check file exist?
        if os.path.exists(file_name) is False:
            return False, "Unable to locate downloaded file %s." % file_name

        return True, file_name

    def download_spice_xml_file(self, scan_no, pt_no):
        """ Download a SPICE XML file for one measurement in a scan
        :param scan_no:
        :param pt_no:
        :return: tuple (boolean, local file name/error message)
        """
        # Generate the URL for XML file
        xml_file_name = '%s_exp%d_scan%04d_%04d.xml' % (self._instrumentName, self._expNumber, scan_no, pt_no)
        xml_file_url = '%sexp%d/Datafiles/%s' % (self._myServerURL, self._expNumber, xml_file_name)
        local_xml_file_name = os.path.join(self._dataDir, xml_file_name)

        # Download
        try:
            api.DownloadFile(Address=xml_file_url,
                             Filename=local_xml_file_name)
        except Exception as e:
            return False, 'Unable to download Detector XML file %s dur to %s.' % (xml_file_name, str(e))

        # Check file exist?
        if os.path.exists(local_xml_file_name) is False:
            return False, "Unable to locate downloaded file %s."%(local_xml_file_name)

        return True, local_xml_file_name

    def download_data_set(self, scan_list):
        """
        Download data set including (1) spice file for a scan and (2) XML files for measurements
        :param scan_list:
        :return:
        """
        error_message = ''

        for scan_no in scan_list:
            # Download single spice file for a run
            status, ret_obj = self.download_spice_file(scan_no)

            # Reject if SPICE file cannot download
            if status is False:
                error_message += '%s\n' % ret_obj
                continue

            # Load SPICE file to Mantid
            spice_file_name = ret_obj
            status, spice_table = self.load_spice_scan_file(scan_no, spice_file_name)
            if status is False:
                error_message = spice_table
                return False, error_message
            pt_no_list = self._getPtListFromTable(spice_table)

            # Download all single-measurement file
            for pt_no in pt_no_list:
                status, ret_obj = self.download_spice_xml_file(scan_no, pt_no)
                if status is False:
                    error_message += '%s\n' % ret_obj
            # END-FOR
        # END-FOR (scan_no)

        return True, error_message

    def downloadAllDataSet(self):
        """
        Download all data set
        :return:
        """
        # FIXME 
        for scanno in xrange(1, sys.maxint):
            status, retobj = self.download_spice_file(scanno)
            if status is False:
                break



        return

    def existDataFile(self, scanno, ptno):
        """
        Check whether data file for a scan or pt number exists
        :param scanno:
        :param ptno:
        :return:
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

        return True, ""

    def findPeak(self, scanno, ptno):
        """
        Find peak in sample Q space
        :param scanno:
        :param ptno:
        :return:tuple as (boolean, object) such as (false, error message) and (true, PeakInfo object)
        """
        # Check existence
        runnumber = self._expDataDict[self._expNumber][scanno][ptno].getRunNumber()
        if runnumber is None:
            return False, "Unable to locate scan %s/pt %s" % (str(scanno), str(ptno))

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

        return True, peakinfo

    def get_raw_detector_counts(self, scan_no, pt_no):
        """
        Get counts on raw detector
        :param scan_no:
        :param pt_no:
        :return: boolean, 2D numpy data
        """
        # Get workspace (in memory or loading)
        status, ret_obj = self._locate_raw_det_counts(scan_no, pt_no)
        if status is False:
            error_message = ret_obj
        else:
            status, error_message = self.load_spice_xml_file()

        # TODO - FROM HERE!
        # QUESTION: SHALL LOAD/FIND FROM MEMORY BE HIDDEN BY MYCONTROL CLASS FROM GUI?
        #

        '''
        xmlwsname = self._xmlwsbasename + "_%d" % (pt)
        if self._xmlwkspdict.has_key(xmlwsname) is False:
            self._logError('Pt %d does not does not exist.' % (pt))
        xmlws = self._xmlwkspdict[xmlwsname]
        self._currPt = xmlws
        '''

        self._plot_raw_xml_2d(self._currPt)


        numspec = xmlws.getNumberHistograms()
        vecylist = []
        for iws in xrange(len(numspec)):
            vecy = xmlws.readY(0)
            vecylist.append(vecy)
        # ENDFOR(iws)

        # plot
        self._plot2DData(vecylist)

    def indexPeaks(self, srcpeakws, targetpeakws):
        """ Index peaks in a peak workspace by UB matrix
        :param srcpeakws: peak workspace containing UB matrix
        :param targetpeakws: peak workspace to get peaks indexed
        :return: (Boolean, Object) - Boolean = True,  Object = ??? - Boolean = False, Object = error message (str)
        """
        # Check input
        if isinstance(peakws, PeakWorkspace) is False:
            return False, "Input workspace %s is not a PeakWorkspace" % (str(peakws))

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

        return True, ""

    def load_spice_scan_file(self, scan_no, spice_file_name=Non):
        """
        """
        # Form standard name for a SPICE file if name is not given
        if spice_file_name is None:
            spice_file_name = os.path.join(self._dataDir, 'HB3A_exp%04d_scan%04d.dat' % (self._expNumber, scan_no))
        out_ws_name = 'Table_Exp%d_Scan%04d' % (self._expNumber, scan_no)

        # Load SPICE file
        try:
            spice_table_ws, info_matrix_ws = api.LoadSpiceAscii(Filename=spice_file_name,
                                                                OutputWorkspace=out_ws_name,
                                                                RunInfoWorkspace='TempInfo')
        except RuntimeError as e:
            return False, 'Unable to load SPICE data %s due to %s' % (spice_file_name, str(e))

        # Store
        self._spiceTableDict[scan_no] = spice_table_ws

        return True, spice_table_ws

    def load_spice_xml_file(self, scan_no, pt_no):
        """ Load SPICE's XML file to
        """
        # Form XMIL file as ~/../HB3A_exp355_scan%04d_%04d.xml'%(scan_no, pt)
        xml_file_name = os.path.join(self._dataDir, 'HB3A_exp355_scan%04d_%04d.xml' % (scan_no, pt_no))

        # Load SPICE Pt. file
        try:
            ret_obj = api.LoadSpiceXML2DDet(Filename=xml_file_name,
                                            OutputWorkspace='s%04d_%04d'%(scan_no, pt_no),
                                            DetectorGeometry='256,256',
                                            SpiceTableWorkspace='Table_355_%04d' % scan_no,
                                            PtNumber=pt_no)
        except RuntimeError as e:
            return False, str(e)

        pt_ws = ret_obj

        return True, pt_ws

    def process_pt_wksp(self, scan_no, pt_no):
        """
        """
        # Rebin in momentum space: UB matrix only
        try:
            ret_obj = api.Rebin(InputWorkspace='s%04d_%04d'%(scan_no, pt_no),
                                OutputWorkspace='s%04d_%04d'%(scan_no, pt_no),
                                Params='6,0.01,6.5')
        except RuntimeError as e:
            return False, str(e)

        # Set Goniometer
        try:
            ret_obj = api.SetGoniometer(Workspace='s%04d_%04d'%(scan_no, pt_no),
                              Axis0='_omega,0,1,0,-1',
                              Axis1='_chi,0,0,1,-1',
                              Axis2='_phi,0,1,0,-1')
        except RuntimeError as e:
            return False, str(e)

        processed_ws = ret_obj

        return True, processed_ws

    def import_data_to_mantid(self, scan_no, pt_no):
        """
        """
        # Check existence of SPICE workspace
        if self._spiceTableDict.has_key(scan_no) is False:
            status, ret_obj = self.load_spice_scan_file(scan_no)
            if status is False:
                return False, ret_obj

        # Check existence of matrix workspace 
        if self._expDataDict[self._expNumber].has_key((scan_no, pt_no)) is False:
            self.load_spice_xml_file(scan_no, pt_no)

        return True, ''

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

    def set_server_url(self, server_url):
        """ Set data server's URL
        """
        # Server URL must end with '/'
        self._myServerURL = str(server_url)
        if self._myServerURL.endswith('/') is False:
            self._myServerURL + '/'

        # Test URL valid or not
        is_url_good = False
        try:
            r = urllib2.urlopen(self._myServerURL)
        except urllib2.HTTPError, e:
            print(e.code) 
        except urllib2.URLError, e: 
            print(e.args)
        else:
            is_url_good = True
            r.close()

        return is_url_good

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

    def set_local_cache(self, localdir):
        """ Set local cache diretory
        """
        # TODO Doc : ..., localdir is ...

        # FIXME: (Improvement) move the algorithm to work with destination dir from reduce4circleGUI.do_download_spice_data here!

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

    def set_exp_number(self, expno):
        """ Set experiment number
        """
        # TODO : need some verification on input expno
        try:
            self._expNumber = int(expno)
        except TypeError:
            return False

        return True

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
