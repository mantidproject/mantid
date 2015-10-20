#pylint: disable=C0302,C0103,R0902,R0904,R0913,W0212,W0621,R0912
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
import urllib2

import numpy

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
        self._userHKL = [0, 0, 0]

        self._myExpNumber = None
        self._myScanNumber = None
        self._myPtNumber = None

        self._myPeakWSKey = (None, None, None)
        self._myPeakIndex = None
        # IPeak instance
        self._myPeak = None

        self._myLastPeakUB = None

        return

    def get_peak_workspace(self):
        """
        Get peak workspace related
        :return:
        """
        assert isinstance(self._myPeakWSKey, tuple)
        exp_number, scan_number, pt_number = self._myPeakWSKey

        return self._myParent.get_ub_peak_ws(exp_number, scan_number, pt_number)[1]

    def get_peak_ws_hkl(self):
        """ Get HKL from PeakWorkspace
        :return:
        """
        hkl = self._myPeak.getHKL()

        return hkl.getX(), hkl.getY(), hkl.getZ()

    def get_user_hkl(self):
        """
        Get HKL set to this object by client
        :return: 3-tuple of float as (H, K, L)
        """
        hkl = self._userHKL

        return hkl[0], hkl[1], hkl[2]

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
            self._myPeak = peak_ws.getPeak(0)
        else:
            error_message = peak_ws
            return False, error_message

        self._myExpNumber = exp_number
        self._myScanNumber = scan_number
        self._myPtNumber = pt_number

        return True, ''

    def set_from_peak_ws(self, peak_ws, peak_index):
        """
        Set from peak workspace
        :param peak_ws:
        :return:
        """
        # Check
        assert isinstance(peak_ws, mantid.dataobjects.PeaksWorkspace)

        # Get peak
        try:
            peak = peak_ws.getPeak(peak_index)
        except RuntimeError as run_err:
            raise RuntimeError(run_err)

        self._myPeak = peak

        return

    def set_peak_ws_hkl_from_user(self):
        """

        :return:
        """
        # Check
        if isinstance(self._myPeak, mantid.api.IPeak) is False:
            raise RuntimeError('self._myPeakWS should be an instance of  mantid.api.IPeak. '
                               'But it is of instance of %s now.' % str(type(self._myPeak)))

        # Get hkl
        h, k, l = self._userHKL
        print '[DB] PeakInfo Get User HKL = (%f, %f, %f) to IPeak ' % (h, k, l)

        self._myPeak.setHKL(h, k, l)

        return

    def set_user_hkl(self, h, k, l):
        """
        Set HKL to this peak Info
        :return:
        """
        assert isinstance(h, float)
        assert isinstance(k, float)
        assert isinstance(l, float)

        self._userHKL[0] = h
        self._userHKL[1] = k
        self._userHKL[2] = l

        print '[DB] PeakInfo Set User HKL to (%f, %f, %f) ' % (self._userHKL[0], self._userHKL[1], self._userHKL[2])

        return

    def getExpInfo(self):
        """

        :return: 3-tuple of integer as experiment number, scan number and Pt number
        """
        return self._myExpNumber, self._myScanNumber, self._myPtNumber

    def getQSample(self):
        """

        :return: 3-tuple of floats as Qx, Qy, Qz
        """
        q_sample = self._myPeak.getQSampleFrame()
        return q_sample.getX(), q_sample.getY(), q_sample.getZ()


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
        # Container for UB  matrix
        self._myUBMatrixDict = dict()

        # Peak Info
        self._myPeakInfoDict = dict()
        # Last UB matrix calculated
        self._myLastPeakUB = None
        # Flag for data storage
        self._cacheDataOnly = False

        # A dictionary to manage all loaded and processed MDEventWorkspaces
        # self._expDataDict = {}

        return

    def add_peak_info(self, exp_number, scan_number, pt_number):
        """ Add a peak info for calculating UB matrix
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return: (boolean, PeakInfo/string)
        """
        has_peak_ws, peak_ws = self.get_ub_peak_ws(exp_number, scan_number, pt_number)
        if has_peak_ws is False:
            err_msg = 'No peak workspace found for Exp %s Scan %s Pt %s' % (
                exp_number, scan_number, pt_number)
            print '\n[DB] Fail to add peak info due to %s\n' % err_msg
            return False, err_msg

        if peak_ws.rowCount() > 1:
            err_msg = 'There are more than 1 peak in PeaksWorkspace.'
            print '\n[DB] Fail to add peak info due to %s\n' % err_msg
            return False, err_msg

        peak_info = PeakInfo(self)
        peak_info.set_from_run_info(exp_number, scan_number, pt_number)

        # Add to data management
        self._myPeakInfoDict[(exp_number, scan_number, pt_number)] = peak_info

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
        ub_peak_ws_name = 'Temp_UB_Peak'
        ub_peak_ws = api.CloneWorkspace(InputWorkspace=peak_info_list[0].get_peak_workspace(),
                                        OutputWorkspace=ub_peak_ws_name)

        for i_peak_info in xrange(1, len(peak_info_list)):
            # Set HKL as optional
            peak_ws = peak_info_list[i_peak_info].get_peak_workspace()

            # Combine peak workspace
            ub_peak_ws = api.CombinePeaksWorkspaces(LHSWorkspace=ub_peak_ws,
                                                    RHSWorkspace=peak_ws,
                                                    CombineMatchingPeaks=False,
                                                    OutputWorkspace=ub_peak_ws_name)
        # END-FOR(i_peak_info)

        # Calculate UB matrix
        try:
            api.CalculateUMatrix(PeaksWorkspace=ub_peak_ws_name,
                                 a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        except ValueError as val_err:
            return False, str(val_err)

        ub_matrix = ub_peak_ws.sample().getOrientedLattice().getUB()

        self._myLastPeakUB = ub_peak_ws

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
        # Check
        if exp_number is None:
            exp_number = self._expNumber
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)

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
            GenerateVirtualInstrument=False,
            ScanList=[scan_number],
            PtLists=[-1, pt_number],
            DataDirectory=self._dataDir,
            GetFileFromServer=False,
            Detector2ThetaTolerance=0.01,
            OutputWorkspace=exp_info_ws_name,
            DetectorTableWorkspace=virtual_instrument_info_table_name)

        # Load XML file to MD
        pt_md_ws_name = get_single_pt_md_name(exp_number, scan_number, pt_number)
        api.ConvertCWSDExpToMomentum(InputWorkspace=exp_info_ws_name,
                                     CreateVirtualInstrument=False,
                                     OutputWorkspace=pt_md_ws_name,
                                     Directory=self._dataDir)

        # Find peak in Q-space
        pt_peak_ws_name = get_single_pt_peak_ws_name(exp_number, scan_number, pt_number)
        api.FindPeaksMD(InputWorkspace=pt_md_ws_name, MaxPeaks=10,
                        DensityThresholdFactor=0.01, OutputWorkspace=pt_peak_ws_name)
        peak_ws = AnalysisDataService.retrieve(pt_peak_ws_name)
        pt_md_ws = AnalysisDataService.retrieve(pt_md_ws_name)
        self._myPtMDDict[(exp_number, scan_number, pt_number)] = pt_md_ws

        num_peaks = peak_ws.getNumberPeaks()
        if num_peaks != 1:
            err_msg = 'Find %d peak from scan %d pt %d.  ' \
                      'For UB matrix calculation, 1 and only 1 peak is allowed' % (num_peaks, scan_number, pt_number)
            return False, err_msg
        else:
            self._add_ub_peak_ws(exp_number, scan_number, pt_number, peak_ws)
            status, ret_obj = self.add_peak_info(exp_number, scan_number, pt_number)
            if status is True:
                pass
                # peak_info = ret_obj
                # peak_info.set_md_ws(pt_md_ws)
            else:
                err_msg = ret_obj
                return False, err_msg

        return True, ''

    def get_experiment(self):
        """
        Get experiment number
        :return:
        """
        return self._expNumber

    def get_pt_numbers(self, exp_no, scan_no, load_spice_scan=False):
        """ Get Pt numbers (as a list) for a scan in an experiment
        :param exp_no:
        :param scan_no:
        :param load_spice_scan:
        :return: (Boolean, Object) as (status, pt number list/error message)
        """
        # Check
        if exp_no is None:
            exp_no = self._expNumber
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)

        # Get workspace
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

    def get_sample_log_value(self, exp_number, scan_number, pt_number, log_name):
        """
        Get sample log's value
        :param exp_number:
        :param scan_number:167
        :param pt_number:
        :param log_name:
        :return: float
        """
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)
        assert isinstance(log_name, str)
        try:
            md_ws = self._myPtMDDict[(exp_number, scan_number, pt_number)]
        except KeyError as ke:
            return 'Unable to find log value %s due to %s.' % (log_name, str(ke))

        return md_ws.getExperimentInfo(0).run().getProperty(log_name).value

    def get_peak_info(self, exp_number, scan_number, pt_number):
        """
        get peak information instance
        :param exp_number: experiment number.  if it is None, then use the current exp number
        :param scan_number:
        :param pt_number:
        :return:
        """
        # Check for type
        if exp_number is None:
            exp_number = self._expNumber

        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)

        # Check for existence
        if (exp_number, scan_number, pt_number) not in self._myUBPeakWSDict:  # self._myPeakInfoDict:
            err_msg = 'Unable to find PeakInfo for Exp %d Scan %d Pt %d. ' \
                      'Existing keys are %s' % (exp_number, scan_number, pt_number,
                                                str(self._myUBPeakWSDict.keys()))
            return False, err_msg

        print '[DB] PeakInfoDictionary Keys = %s' % str(self._myPeakInfoDict.keys())

        return True, self._myPeakInfoDict[(exp_number, scan_number, pt_number)]

    def get_ub_peak_ws(self, exp_number, scan_number, pt_number):
        """
        Get peak workspace for the peak picked to calculate UB matrix
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

    def index_peak(self, ub_matrix, scan_number, pt_number):
        """ Index peaks in a Pt.
        :param ub_matrix: numpy.ndarray (3, 3)
        :param scan_number:
        :param pt_number:
        :return: boolean, object (list of HKL or error message)
        """
        # Check
        assert isinstance(ub_matrix, numpy.ndarray)
        assert ub_matrix.shape == (3, 3)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int)

        # Find out the peak workspace
        exp_num = self._expNumber
        if (exp_num, scan_number, pt_number) in self._myUBPeakWSDict is False:
            err_msg = 'No PeakWorkspace is found for exp %d scan %d pt %d' % (
                exp_num, scan_number, pt_number)
            return False, err_msg

        peak_ws = self._myUBPeakWSDict[(exp_num, scan_number, pt_number)]
        ub_1d = ub_matrix.reshape(9,)
        print '[DB] UB matrix = ', ub_1d

        # Set UB
        api.SetUB(Workspace=peak_ws, UB=ub_1d)

        # Note: IndexPeaks and CalcualtePeaksHKL do the same job
        #       while IndexPeaks has more control on the output
        num_peak_index, error = api.IndexPeaks(PeaksWorkspace=peak_ws,
                                               Tolerance=0.4,
                                               RoundHKLs=False)

        if num_peak_index == 0:
            return False, 'No peak can be indexed.'
        elif num_peak_index > 1:
            raise RuntimeError('Case for PeaksWorkspace containing more than 1 peak is not '
                               'considered. Contact developer for this issue.')
        else:
            hkl_v3d = peak_ws.getPeak(0).getHKL()
            hkl = [hkl_v3d.X(), hkl_v3d.Y(), hkl_v3d.Z()]

        return True, (hkl, error)

    def load_spice_scan_file(self, exp_no, scan_no, spice_file_name=None):
        """
        Load a SPICE scan file to table workspace and run information matrix workspace.
        :param scan_no:
        :param spice_file_name:
        :return: status (boolean), error message (string)
        """
        # Default for exp_no
        if exp_no is None:
            exp_no = self._expNumber

        # Check whether the workspace has been loaded
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        out_ws_name = get_spice_table_name(exp_no, scan_no)
        if (exp_no, scan_no) in self._mySpiceTableDict:
            return True, out_ws_name

        # Form standard name for a SPICE file if name is not given
        if spice_file_name is None:
            spice_file_name = os.path.join(self._dataDir, get_spice_file_name(exp_no, scan_no))

        # Download SPICE file if necessary
        if os.path.exists(spice_file_name) is False:
            self.download_spice_file(exp_no, scan_no, over_write=True)

        try:
            spice_table_ws, info_matrix_ws = api.LoadSpiceAscii(Filename=spice_file_name,
                                                                OutputWorkspace=out_ws_name,
                                                                RunInfoWorkspace='TempInfo')
            api.DeleteWorkspace(Workspace=info_matrix_ws)
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
        assert isinstance(spice_table_ws, mantid.dataobjects.TableWorkspace)
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

    def group_workspaces(self, exp_number, group_name):
        """

        :return:
        """
        # Find out the input workspace name
        ws_names_str = ''
        for key in self._myRawDataWSDict.keys():
            if key[0] == exp_number:
                ws_names_str += '%s,' % self._myRawDataWSDict[key].name()

        for key in self._mySpiceTableDict.keys():
            if key[0] == exp_number:
                ws_names_str += '%s,' % self._mySpiceTableDict[key].name()

        # Check
        if len(ws_names_str) == 0:
            return False, 'No workspace is found for experiment %d.' % exp_number

        # Remove last ','
        ws_names_str = ws_names_str[:-1]

        # Group
        api.GroupWorkspaces(InputWorkspaces=ws_names_str,
                            OutputWorkspace=group_name)

        return

    def merge_pts_in_scan(self, exp_no, scan_no, target_ws_name, target_frame):
        """
        Merge Pts in Scan
        All the workspaces generated as internal results will be grouped
        :param exp_no:
        :param scan_no:
        :param target_ws_name:
        :param target_frame:
        :return: (merged workspace name, workspace group name)
        """
        # Check
        if exp_no is None:
            exp_no = self._expNumber
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(target_frame, str)
        assert isinstance(target_ws_name, str)

        ub_matrix_1d = None

        # Target frame
        if target_frame.lower().startswith('hkl'):
            target_frame = 'hkl'
            ub_matrix_1d = self._myUBMatrixDict[self._expNumber].reshape(9,)
        elif target_frame.lower().startswith('q-sample'):
            target_frame = 'qsample'

        else:
            raise RuntimeError('Target frame %s is not supported.' % target_frame)

        # Process data and save
        status, pt_num_list = self.get_pt_numbers(exp_no, scan_no, True)
        if status is False:
            err_msg = pt_num_list
            return False, err_msg
        else:
            print '[DB] Number of Pts for Scan %d is %d' % (scan_no, len(pt_num_list))
            print '[DB] Data directory: %s' % self._dataDir
        max_pts = 0
        ws_names_str = ''
        ws_names_to_group = ''

        for pt in pt_num_list:
            try:
                self.download_spice_xml_file(scan_no, pt, overwrite=False)
                api.CollectHB3AExperimentInfo(ExperimentNumber=exp_no, ScanList='%d' % scan_no, PtLists='-1,%d' % pt,
                                              DataDirectory=self._dataDir,
                                              GenerateVirtualInstrument=False,
                                              OutputWorkspace='ScanPtInfo_Exp%d_Scan%d' % (exp_no, scan_no),
                                              DetectorTableWorkspace='MockDetTable')

                out_q_name = 'HB3A_Exp%d_Scan%d_Pt%d_MD' % (exp_no, scan_no, pt)
                api.ConvertCWSDExpToMomentum(InputWorkspace='ScanPtInfo_Exp406_Scan%d' % scan_no,
                                             CreateVirtualInstrument=False,
                                             OutputWorkspace=out_q_name,
                                             Directory=self._dataDir)

                ws_names_to_group += out_q_name + ','
                if target_frame == 'hkl':
                    out_hkl_name = 'HKL_Scan%d_Pt%d' % (scan_no, pt)
                    api.ConvertCWSDMDtoHKL(InputWorkspace=out_q_name,
                                           UBMatrix=ub_matrix_1d,
                                           OutputWorkspace=out_hkl_name)
                    ws_names_str += out_hkl_name + ','
                    ws_names_to_group += out_hkl_name + ','
                else:
                    ws_names_str += out_q_name + ','

            except RuntimeError as e:
                print '[Error] Reducing scan %d pt %d due to %s' % (scan_no, pt, str(e))
                continue

            else:
                max_pts = pt
        # END-FOR

        # Merge
        if target_frame == 'qsample':
            out_ws_name = target_ws_name + '_QSample'
        elif target_frame == 'hkl':
            out_ws_name = target_ws_name + '_HKL'
        else:
            raise RuntimeError('Impossible to have target frame %s' % target_frame)

        ws_names_str = ws_names_str[:-1]
        api.MergeMD(InputWorkspaces=ws_names_str, OutputWorkspace=out_ws_name, SplitInto=max_pts)

        # Group workspaces
        group_name = 'Group_Exp406_Scan%d' % scan_no
        api.GroupWorkspaces(InputWorkspaces=ws_names_to_group, OutputWorkspace=group_name)
        spice_table_name = get_spice_table_name(exp_no, scan_no)
        api.GroupWorkspaces(InputWorkspaces='%s,%s' % (group_name, spice_table_name), OutputWorkspace=group_name)

        ret_tup = out_ws_name, group_name

        return ret_tup

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

    def set_ub_matrix(self, exp_number, ub_matrix):
        """
        Set up UB matrix to _UBMatrix dictionary
        :param exp_number:
        :param ub_matrix:
        :return:
        """
        # Check
        if exp_number is None:
            exp_number = self._expNumber

        assert isinstance(exp_number, int)
        assert isinstance(ub_matrix, numpy.ndarray)
        assert ub_matrix.shape == (3, 3)

        # Set up
        self._myUBMatrixDict[exp_number] = ub_matrix

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

    def set_hkl_to_peak(self, exp_number, scan_number, pt_number):
        """
        Get HKL as _h, _k, _l from MDEventWorkspace.  It is for HB3A only
        :return:
        """
        status, peak_info = self.get_peak_info(exp_number, scan_number, pt_number)
        if status is False:
            err_msg = peak_info
            return False, err_msg

        md_ws = self._myPtMDDict[(exp_number, scan_number, pt_number)]
        assert md_ws.getNumExperimentInfo() == 1
        exp_info = md_ws.getExperimentInfo(0)

        try:
            m_h = float(exp_info.run().getProperty('_h').value)
            m_k = float(exp_info.run().getProperty('_k').value)
            m_l = float(exp_info.run().getProperty('_l').value)
        except RuntimeError as error:
            return False, 'Unable to retrieve HKL due to %s.' % (str(error))

        peak_ws = peak_info.get_peak_workspace()
        peak = peak_ws.getPeak(0)
        peak.setHKL(m_h, m_k, m_l)

        return True, (m_h, m_k, m_l)

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
        assert isinstance(matrix_ws, mantid.dataobjects.Workspace2D)

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
        assert isinstance(md_ws, mantid.dataobjects.MDEventWorkspace)

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
        assert isinstance(peak_ws, mantid.dataobjects.PeaksWorkspace)

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
        assert isinstance(spice_table_ws, mantid.dataobjects.TableWorkspace)
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
            assert isinstance(ws, mantid.dataobjects.Workspace2D)
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
    ws_name = 'HB3A_exp%d_scan%04d_%04d' % (exp_number, scan_number, pt_number)

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
