################################################################################
#
# Controlling class
#
# == Data download and storage ==
# - Local data storage (local-mode)
# - Download from internet to cache (download-mode)
#
################################################################################
import os
import sys
import urllib2

import numpy

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
from mantid.api import AnalysisDataService

DebugMode = True

DET_X_SIZE = 256
DET_Y_SIZE = 256


class PeakInfo(object):
    """ Class containing a peak's information for GUI
    In order to manage some operations for a peak
    It does not contain peak workspace but will hold
    """
    def __init__(self, parent):
        """ Init
        """
        assert isinstance(parent, CWSCDReductionControl)

        # Define class variable
        self._myParent = parent

        self._myPeakWSKey = None
        self._myPeakIndex = None
        self._myPeak = None

        return

    def get_peak_workspace(self):
        """
        Get peak workspace related
        :return:
        """
        exp_number, scan_number, pt_number = self._myPeakWSKey
        return self._myParent.get_ub_peak_ws(exp_number, scan_number, pt_number)[1]

    def get_raw_data_ws(self):
        """

        :return:
        """
        exp_number, scan_number, pt_number = self._myPeakWSKey
        data_ws = self._myParent.get_raw_data_workspace(exp_number, scan_number, pt_number)
        if data_ws is None:
            print "\nRaw data workspace dict keys:", self._myParent._myRawDataWSDict.keys(), '\n'
            raise RuntimeError('Unable to find raw data workspace of Exp %d Scan %d Pt %d. ' % (
                exp_number, scan_number, pt_number))
        assert isinstance(data_ws, mantid.dataobjects._dataobjects.Workspace2D)

        return data_ws

    def set_from_run_info(self, exp_number, scan_number, pt_number):
        """ Set from run information with parent
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return:
        """
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)

        status, peak_ws = self._myParent.get_ub_peak_ws(exp_number, scan_number, pt_number)
        if status is True:
            self._myPeakWSKey = (exp_number, scan_number, pt_number)
            self._myPeakIndex = 0
        else:
            error_message = peak_ws
            return False, error_message

        return True, ''

    def set_from_peak_ws(self, peak_ws, peak_index):
        """
        Set from peak workspace
        :param peak_ws:
        :return:
        """
        # Check
        assert isinstance(peak_ws, mantid.dataobjects._dataobjects.PeaksWorkspace)

        # Get peak
        try:
            peak = peak_ws.getPeak(peak_index)
        except RuntimeError as run_err:
            raise RuntimeError(run_err)

        self._myPeak = peak

        return

    def getExpInfo(self):
        """

        :return:
        """
        return run_number, scan, pt

    def getHKL(self):
        """

        :return:
        """
        return self._myPeak.getHKL()

    def getQSample(self):
        """

        :return:
        """
        return self._myPeak.getQSampleFrame()


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
        # No Use/Confusing: self._expNumber = 0

        self._dataDir = None
        self._workDir = '/tmp'

        self._myServerURL = ''

        # Some set up
        self._expNumber = None

        # Container for MDEventWorkspace for each Pt.
        self._myPtMDDict = dict()
        # Container for loaded workspaces
        self._mySpiceTableDict = {}
        # Container for loaded raw pt workspace
        self._myRawDataWSDict = dict()
        # Container for PeakWorkspaces for calculating UB matrix
        self._myUBPeakWSDict = dict()

        # A dictionary to manage all loaded and processed MDEventWorkspaces
        # self._expDataDict = {}

        return

    def add_peak_info(self, exp_number, scan_number, pt_number):
        """ Add a peak info for calculating UB matrix
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return:
        """
        has_peak_ws, peak_ws = self.get_ub_peak_ws(exp_number, scan_number, pt_number)
        if has_peak_ws is False:
            return False, 'No peak workspace found for Exp %s Scan %s Pt %s' % (
                exp_number, scan_number, pt_number
            )
        if peak_ws.rowCount() > 1:
            return False, 'There are more than 1 peak in PeaksWorkspace.'

        peak_info = PeakInfo(self)
        peak_info.set_from_run_info(exp_number, scan_number, pt_number)

        return True, peak_info

    def calculate_ub_matrix(self, peak_info_list, a, b, c, alpha, beta, gamma):
        """
        Calculate UB matrix

        Set Miller index from raw data in Workspace2D.
        :param peakws:
        :param a:
        :param b:
        :param c:
        :param alpha:
        :param beta:
        :param gamma:
        :return:
        """
        # Check
        assert isinstance(peak_info_list, list)
        for peak_info in peak_info_list:
            if isinstance(peak_info, PeakInfo) is False:
                raise NotImplementedError('Input PeakList is of type %s.' % str(type(peak_info_list[0])))
            assert isinstance(peak_info, PeakInfo)

        if len(peak_info_list) < 2:
            return False, 'Too few peaks are input to calculate UB matrix.  Must be >= 2.'

        # Construct a new peak workspace by combining all single peak
        ub_peak_ws = api.CloneWorkspace(InputWorkspace=peak_info_list[0].get_peak_workspace())

        for i_peak_info in xrange(1, len(peak_info_list)):
            # Set HKL
            matrix_ws = peak_info_list[i_peak_info].get_raw_data_ws()
            h = float(int(matrix_ws.run().getProperty('_h').value))
            k = float(int(matrix_ws.run().getProperty('_k').value))
            l = float(int(matrix_ws.run().getProperty('_l').value))

            peak_ws = peak_info_list[i_peak_info].get_peak_workspace()
            peak = peak_ws.getPeak(0)
            peak.setHKL(h, k, l)

            # Combine peak workspace

            api.CombinePeaksWorkspaces(LHSWorkspace=ub_peak_ws, RHSWorkspace=peak_ws,
                                       CombineMatchingPeaks=False, OutputWorkspace=ub_peak_ws)
        # END-FOR(i_peak_info)

        # Calculate UB matrix
        api.CalculateUMatrix(PeaksWorkspace=ub_peak_ws,
                             a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)

        ub_matrix = peak_ws.sample().getOrientedLattice().getUB()

        return True, ub_matrix

    def does_raw_loaded(self, exp_no, scan_no, pt_no):
        """
        Check whether the raw Workspace2D for a Pt. exists
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :return:
        """
        return (exp_no, scan_no, pt_no) in self._myRawDataWSDict

    def does_spice_loaded(self, exp_no, scan_no):
        """ Check whether a SPICE file has been loaded
        :param exp_no:
        :param scan_no:
        :return:
        """
        return (exp_no, scan_no) in self._mySpiceTableDict

    def download_spice_file(self, exp_number, scan_number, over_write):
        """
        Download a scan/pt data from internet
        :param exp_number: experiment number
        :param scan_number:
        :return:
        """
        # Generate the URL for SPICE data file
        file_url = '%sexp%d/Datafiles/HB3A_exp%04d_scan%04d.dat' % (self._myServerURL, exp_number,
                                                                    exp_number, scan_number)
        file_name = '%s_exp%04d_scan%04d.dat' % (self._instrumentName, exp_number, scan_number)
        file_name = os.path.join(self._dataDir, file_name)
        if os.path.exists(file_name) is True and over_write is False:
            return True, file_name

        # Download
        try:
            api.DownloadFile(Address=file_url, Filename=file_name)
        except RuntimeError as run_err:
            return False, str(run_err)

        # Check file exist?
        if os.path.exists(file_name) is False:
            return False, "Unable to locate downloaded file %s." % file_name

        return True, file_name

    def download_spice_xml_file(self, scan_no, pt_no, exp_no=None, overwrite=False):
        """ Download a SPICE XML file for one measurement in a scan
        :param scan_no:
        :param pt_no:
        :param exp_no:
        :param overwrite:
        :return: tuple (boolean, local file name/error message)
        """
        # Experiment number
        if exp_no is None:
            exp_no = self._expNumber

        # Form the target file name and path
        xml_file_name = '%s_exp%d_scan%04d_%04d.xml' % (self._instrumentName, exp_no, scan_no, pt_no)
        local_xml_file_name = os.path.join(self._dataDir, xml_file_name)
        if os.path.exists(local_xml_file_name) is True and overwrite is False:
            return True, local_xml_file_name

        # Generate the URL for XML file
        xml_file_url = '%sexp%d/Datafiles/%s' % (self._myServerURL, exp_no, xml_file_name)

        # Download
        try:
            api.DownloadFile(Address=xml_file_url,
                             Filename=local_xml_file_name)
        except RuntimeError as run_err:
            return False, 'Unable to download Detector XML file %s dur to %s.' % (xml_file_name, str(run_err))

        # Check file exist?
        if os.path.exists(local_xml_file_name) is False:
            return False, "Unable to locate downloaded file %s."%(local_xml_file_name)

        return True, local_xml_file_name

    def download_data_set(self, scan_list, overwrite=False):
        """
        Download data set including (1) spice file for a scan and (2) XML files for measurements
        :param scan_list:
        :return:
        """
        # Check
        if self._expNumber is None:
            raise RuntimeError('Experiment number is not set up for controller.')

        error_message = ''

        for scan_no in scan_list:
            # Download single spice file for a run
            status, ret_obj = self.download_spice_file(exp_number=self._expNumber,
                                                       scan_number=scan_no,
                                                       over_write=overwrite)

            # Reject if SPICE file cannot download
            if status is False:
                error_message += '%s\n' % ret_obj
                continue

            # Load SPICE file to Mantid
            spice_file_name = ret_obj
            status, ret_obj = self.load_spice_scan_file(self._expNumber, scan_no, spice_file_name)
            if status is False:
                error_message = ret_obj
                return False, error_message
            else:
                spice_table = self._mySpiceTableDict[(self._expNumber, scan_no)]
                assert spice_table
            pt_no_list = self._get_pt_list_from_spice_table(spice_table)

            # Download all single-measurement file
            for pt_no in pt_no_list:
                status, ret_obj = self.download_spice_xml_file(scan_no, pt_no, overwrite=overwrite)
                if status is False:
                    error_message += '%s\n' % ret_obj
            # END-FOR
        # END-FOR (scan_no)

        return True, error_message

    def existDataFile(self, scanno, ptno):
        """
        Check whether data file for a scan or pt number exists
        :param scanno:
        :param ptno:
        :return:
        """
        # Check spice file
        spice_file_name = '%s_exp%04d_scan%04d.dat'%(self._instrumentName,
                                                   self._expNumber, scanno)
        spice_file_name = os.path.join(self._dataDir, spice_file_name)
        if os.path.exists(spice_file_name) is False:
            return False, 'Spice data file %s cannot be found.'% spice_file_name

        # Check xml file
        xmlfilename = '%s_exp%d_scan%04d_%04d.xml'%(self._instrumentName, self._expNumber,
                scanno, ptno)
        xmlfilename = os.path.join(self._dataDir, xmlfilename)
        if os.path.exists(xmlfilename) is False:
            return (False, "Pt. XML file %s cannot be found."%(xmlfilename))

        return True, ""

    def find_peak(self, exp_number, scan_number, pt_number):
        """ Find 1 peak in sample Q space for UB matrix
        :param scan_number:
        :param pt_number:
        :return:tuple as (boolean, object) such as (false, error message) and (true, PeakInfo object)

        This part will be redo as 11847_Load_HB3A_Experiment
        """
        # Check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)

        # Download or make sure data are there
        status_sp, err_msg_sp = self.download_spice_file(exp_number, scan_number, over_write=False)
        status_det, err_msg_det = self.download_spice_xml_file(scan_number, pt_number, exp_number,
                                                               overwrite=False)
        if status_sp is False or status_det is False:
            return False, 'Unable to access data (1) %s (2) %s' % (err_msg_sp, err_msg_det)

        # Collect reduction information: example
        exp_info_ws_name = get_pt_info_ws_name(exp_number, scan_number)
        virtual_instrument_info_table_name = get_virtual_instrument_table_name(exp_number, scan_number, pt_number)
        api.CollectHB3AExperimentInfo(
            ExperimentNumber=exp_number,
            ScanList=[scan_number],
            PtLists=[-1, pt_number],
            DataDirectory=self._dataDir,
            GetFileFromServer=False,
            Detector2ThetaTolerance=0.01,
            OutputWorkspace=exp_info_ws_name,
            DetectorTableWorkspace=virtual_instrument_info_table_name)

        # Load XML file to MD
        pt_md_ws_name = get_single_pt_md_name(exp_number, scan_number, pt_number)
        api.ConvertCWSDExpToMomentum(InputWorkspace=exp_info_ws_name ,
                                     DetectorTableWorkspace=virtual_instrument_info_table_name,
                                     OutputWorkspace=pt_md_ws_name,
                                     SourcePosition=[0, 0, -2],
                                     SamplePosition=[0, 0, 0],
                                     PixelDimension=[1, 1, 2, 2, 3, 3, 4, 4],
                                     Directory=self._dataDir)

        # Find peak in Q-space
        pt_peak_ws_name = get_single_pt_peak_ws_name(exp_number, scan_number, pt_number)
        api.FindPeaksMD(InputWorkspace=pt_md_ws_name, MaxPeaks=10,
                        DensityThresholdFactor=0.01, OutputWorkspace=pt_peak_ws_name)
        peak_ws = AnalysisDataService.retrieve(pt_peak_ws_name)

        num_peaks = peak_ws.getNumberPeaks()
        if num_peaks != 1:
            err_msg = 'Find %d peak from scan %d pt %d.  ' \
                      'For UB matrix calculation, 1 and only 1 peak is allowed' % (num_peaks, scan_number, pt_number)
            return False, err_msg
        else:
            self._add_ub_peak_ws(exp_number, scan_number, pt_number, peak_ws)

        return True, ''

    def get_pt_numbers(self, exp_no, scan_no, load_spice_scan=False):
        """ Get Pt numbers (as a list) for a scan in an experiment
        :param exp_no:
        :param scan_no:
        :param load_spice_scan:
        :return:
        """
        print '[DB] Get Pt. for Exp %d Scan %d' % (exp_no, scan_no)
        table_ws = self._get_spice_workspace(exp_no, scan_no)
        if table_ws is None:
            if load_spice_scan is False:
                return False, 'Spice file for Exp %d Scan %d is not loaded.' % (exp_no, scan_no)
            else:
                status, error_message = self.load_spice_scan_file(exp_no, scan_no)
                if status is True:
                    table_ws = self._get_spice_workspace(exp_no, scan_no)
                    if table_ws is None:
                        raise NotImplementedError('Logic error! Cannot happen!')
                else:
                    return False, 'Unable to load Spice file for Exp %d Scan %d due to %s.' % (
                        exp_no, scan_no, error_message)

        col_name_list = table_ws.getColumnNames()
        i_pt = col_name_list.index('Pt.')
        if i_pt < 0 or i_pt >= len(col_name_list):
            return False, 'No column with name Pt. can be found in SPICE table.'

        pt_number_list = []
        num_rows = table_ws.rowCount()
        for i in xrange(num_rows):
            pt_number = table_ws.cell(i, i_pt)
            pt_number_list.append(pt_number)

        return True, pt_number_list

    def get_raw_detector_counts(self, exp_no, scan_no, pt_no):
        """
        Get counts on raw detector
        :param scan_no:
        :param pt_no:
        :return: boolean, 2D numpy data
        """
        # Get workspace (in memory or loading)
        raw_ws = self.get_raw_data_workspace(exp_no, scan_no, pt_no)
        if raw_ws is None:
            return False, 'Raw data for Exp %d Scan %d Pt %d is not loaded.' % (exp_no, scan_no, pt_no)

        # Convert to numpy array
        array2d = numpy.ndarray(shape=(DET_X_SIZE, DET_Y_SIZE), dtype='float')
        for i in xrange(DET_X_SIZE):
            for j in xrange(DET_Y_SIZE):
                array2d[i][j] = raw_ws.readY(i * DET_X_SIZE + j)[0]

        return array2d

    def get_ub_peak_ws(self, exp_number, scan_number, pt_number):
        """

        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return:
        """
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)

        if (exp_number, scan_number, pt_number) not in self._myUBPeakWSDict:
            return False, 'Exp %d Scan %d Pt %d has no peak workspace.' % (exp_number,
                                                                           scan_number,
                                                                           pt_number)

        return True, self._myUBPeakWSDict[(exp_number, scan_number, pt_number)]

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

    def load_spice_scan_file(self, exp_no, scan_no=None, spice_file_name=None):
        """
        Load a SPICE scan file to table workspace and run information matrix workspace.
        :param scan_no:
        :param spice_file_name:
        :return: status (boolean), error message (string)
        """
        # Check whether the workspace has been loaded
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        out_ws_name = get_spice_table_name(exp_no, scan_no)
        if (exp_no, scan_no) in self._mySpiceTableDict:
            return True, out_ws_name

        # Form standard name for a SPICE file if name is not given
        if spice_file_name is None:
            spice_file_name = os.path.join(self._dataDir, get_spice_file_name(exp_no, scan_no))

        # Load SPICE file
        try:
            spice_table_ws, info_matrix_ws = api.LoadSpiceAscii(Filename=spice_file_name,
                                                                OutputWorkspace=out_ws_name,
                                                                RunInfoWorkspace='TempInfo')
        except RuntimeError as run_err:
            return False, 'Unable to load SPICE data %s due to %s' % (spice_file_name, str(run_err))

        # Store
        self._add_spice_workspace(exp_no, scan_no, spice_table_ws)

        return True, out_ws_name

    def load_spice_xml_file(self, exp_no, scan_no, pt_no, xml_file_name=None):
        """
        Load SPICE's XML file to
        :param scan_no:
        :param pt_no:
        :return:
        """
        # Form XMIL file as ~/../HB3A_exp355_scan%04d_%04d.xml'%(scan_no, pt)
        if xml_file_name is None:
            xml_file_name = os.path.join(self._dataDir,
                                         'HB3A_exp%d_scan%04d_%04d.xml' % (exp_no, scan_no, pt_no))

        # Get spice table
        spice_table_ws = self._get_spice_workspace(exp_no, scan_no)
        assert isinstance(spice_table_ws, mantid.dataobjects._dataobjects.TableWorkspace)
        spice_table_name = spice_table_ws.name()

        # Load SPICE Pt. file
        # spice_table_name = 'Table_Exp%d_Scan%04d' % (exp_no, scan_no)
        pt_ws_name = get_raw_data_workspace_name(exp_no, scan_no, pt_no)
        try:
            ret_obj = api.LoadSpiceXML2DDet(Filename=xml_file_name,
                                            OutputWorkspace=pt_ws_name,
                                            DetectorGeometry='256,256',
                                            SpiceTableWorkspace=spice_table_name,
                                            PtNumber=pt_no)
        except RuntimeError as run_err:
            return False, str(run_err)

        pt_ws = ret_obj

        # Add data storage
        self._add_raw_workspace(exp_no, scan_no, pt_no, pt_ws)

        return True, pt_ws_name

    def groupWS(self):
        """

        :return:
        """
        inputws = ''
        for pt in [11]:
            
            inputws += 's%04d_%04d'%(scanno, pt)
            if pt != numrows:
                inputws += ','
        # ENDFOR
        
        GroupWorkspaces(InputWorkspaces=inputws, OutputWorkspace='Group_exp0355_scan%04d'%(scanno))

    def set_server_url(self, server_url):
        """
        Set URL for server to download the data
        :param server_url:
        :return:
        """
        # Server URL must end with '/'
        self._myServerURL = str(server_url)
        if self._myServerURL.endswith('/') is False:
            self._myServerURL += '/'

        # Test URL valid or not
        is_url_good = False
        error_message = None
        try:
            result = urllib2.urlopen(self._myServerURL)
        except urllib2.HTTPError, err:
            error_message = str(err.code)
        except urllib2.URLError, err:
            error_message = str(err.args)
        else:
            is_url_good = True
            result.close()

        if error_message is None:
            error_message = ''
        else:
            error_message = 'Unable to open data server URL: %s due to %s.' % (server_url, error_message)

        return is_url_good, error_message

    def setWebAccessMode(self, mode):
        """
        Set data access mode form server
        :param mode:
        :return:
        """
        if isinstance(mode, str) is False:
            raise RuntimeError('Input mode is not string')

        if mode == 'cache':
            self._cacheDataOnly = True
        elif mode == 'download':
            self._cacheDataOnly = False

        return

    def set_local_data_dir(self, local_dir):
        """
        Set local data storage
        :param local_dir:
        :return:
        """
        # Get absolute path
        if os.path.isabs(local_dir) is False:
            # Input is relative path to current working directory
            cwd = os.getcwd()
            local_dir = os.path.join(cwd, local_dir)

        # Create cache directory if necessary
        if os.path.exists(local_dir) is False:
            try:
                os.mkdir(local_dir)
            except OSError as os_err:
                return False, str(os_err)

        # Check whether the target is writable
        if os.access(local_dir, os.W_OK) is False:
            return False, 'Specified local data directory %s is not writable.' % local_dir

        # Successful
        self._dataDir = local_dir

        return True, ''

    def set_working_directory(self, work_dir):
        """
        Set up the directory for working result
        :return: (boolean, string)
        """
        if os.path.exists(work_dir) is False:
            try:
                os.mkdir(work_dir)
            except OSError as os_err:
                return False, 'Unable to create working directory %s due to %s.' % (work_dir, str(os_err))
        elif os.access(work_dir, os.W_OK) is False:
            return False, 'User specified working directory %s is not writable.' % work_dir

        self._workDir = work_dir

        return True, ''

    def set_instrument_name(self, instrument_name):
        """
        Set instrument name
        :param instrument_name:
        :return:
        """
        # Check
        if isinstance(instrument_name, str) is False:
            return False, 'Input instrument name is not a string but of type %s.' % str(type(instrument_name))
        if len(instrument_name) == 0:
            return False, 'Input instrument name is an empty string.'

        self._instrumentName = instrument_name

        return True, ''

    def set_exp_number(self, exp_number):
        """ Add experiment number
        :param exp_number:
        :return:
        """
        assert isinstance(exp_number, int)
        self._expNumber = exp_number

        return True

    def _add_raw_workspace(self, exp_no, scan_no, pt_no, raw_ws):
        """ Add raw Pt.'s workspace
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :param raw_ws: workspace or name of the workspace
        :return: None
        """
        # Check
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(pt_no, int)

        if isinstance(raw_ws, str):
            # Given by name
            matrix_ws = AnalysisDataService.retrieve(raw_ws)
        else:
            matrix_ws = raw_ws
        assert isinstance(matrix_ws, mantid.dataobjects._dataobjects.Workspace2D)

        self._myRawDataWSDict[(exp_no, scan_no, pt_no)] = matrix_ws

        return

    def _add_md_workspace(self, exp_no, scan_no, pt_no, md_ws):
        """
         Add MD workspace to storage
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :param md_ws:
        :return:
        """
        # Check input
        print '[DB] Type of md_ws is %s.' % str(type(md_ws))
        assert isinstance(md_ws, mantid.dataobjects._dataobjects.MDEventWorkspace)

        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(pt_no)

        self._myPtMDDict[(exp_no, scan_no, pt_no)] = md_ws

        return

    def _add_ub_peak_ws(self, exp_number, scan_number, pt_number, peak_ws):
        """
        Add peak workspace for UB matrix
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :param peak_ws:
        :return:
        """
        # Check
        assert isinstance(peak_ws, mantid.dataobjects._dataobjects.PeaksWorkspace)

        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)

        # Add
        self._myUBPeakWSDict[(exp_number, scan_number, pt_number)] = peak_ws

        return

    def _add_spice_workspace(self, exp_no, scan_no, spice_table_ws):
        """
        """
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(spice_table_ws, mantid.dataobjects._dataobjects.TableWorkspace)
        self._mySpiceTableDict[(exp_no, scan_no)] = spice_table_ws

        return

    def _get_spice_workspace(self, exp_no, scan_no):
        """ Get SPICE's scan table workspace
        :param exp_no:
        :param scan_no:
        :return: Table workspace or None
        """
        try:
            ws = self._mySpiceTableDict[(exp_no, scan_no)]
        except KeyError:
            print '[DB] Keys to SPICE TABLE: %s' % str(self._mySpiceTableDict.keys())
            return None

        return ws

    def get_raw_data_workspace(self, exp_no, scan_no, pt_no):
        """ Get raw workspace
        """
        try:
            ws = self._myRawDataWSDict[(exp_no, scan_no, pt_no)]
            assert isinstance(ws, mantid.dataobjects._dataobjects.Workspace2D)
        except KeyError:
            return None

        return ws

    def _get_pt_list_from_spice_table(self, spice_table_ws):
        """
        Get list of Pt. from a SPICE table workspace
        :param spice_table_ws: SPICE table workspace
        :return: list of Pt.
        """
        numrows = spice_table_ws.rowCount()
        ptlist = []
        for irow in xrange(numrows):
            ptno = int(spice_table_ws.cell(irow, 0))
            ptlist.append(ptno)

        return ptlist


def get_spice_file_name(exp_number, scan_number):
    """
    Get standard HB3A SPICE file name from experiment number and scan number
    :param exp_number:
    :param scan_num:
    :return:
    """
    file_name = 'HB3A_exp%04d_scan%04d.dat' % (exp_number, scan_number)

    return file_name


def get_spice_table_name(exp_number, scan_number):
    """ Form the name of the table workspace for SPICE
    :param exp_number:
    :param scan_number:
    :return:
    """
    table_name = 'HB3A_%03d_%04d_SpiceTable' % (exp_number, scan_number)

    return table_name


def get_raw_data_workspace_name(exp_number, scan_number, pt_number):
    """ Form the name of the matrix workspace to which raw pt. XML file is loaded
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'HB3A_%03d_%04d_%04d_Raw' % (exp_number, scan_number, pt_number)

    return ws_name


def get_pt_info_ws_name(exp_number, scan_number):
    """
    Information table workspace'name from CollectHB3AInfo
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'ScanPtInfo_Exp%d_Scan%d'  % (exp_number, scan_number)

    return ws_name


def get_single_pt_peak_ws_name(exp_number, scan_number, pt_number):
    """
    Form the name of the peak workspace
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'Peak_Exp%d_Scan%d_Pt%d' % (exp_number, scan_number, pt_number)

    return ws_name


def get_single_pt_md_name(exp_number, scan_number, pt_number):
    """ Form the name of the MDEvnetWorkspace for a single Pt. measurement
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'HB3A_Exp%d_Scan%d_Pt%d_MD' % (exp_number, scan_number, pt_number)

    return ws_name


def get_virtual_instrument_table_name(exp_number, scan_number, pt_number):
    """
    Generate the name of the table workspace containing the virtual instrument information
    :param exp_number:
    :param scan_number:
    :param pt_number:
    :return:
    """
    ws_name = 'VirtualInstrument_Exp%d_Scan%d_Pt%d_Table' % (exp_number, scan_number, pt_number)

    return ws_name
