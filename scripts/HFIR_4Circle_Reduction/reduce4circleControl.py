#pylint: disable=C0302,C0103,R0902,R0904,R0913,W0212,W0621,R0912,R0921,R0914,W0403
################################################################################
#
# Controlling class
#
# == Data download and storage ==
# - Local data storage (local-mode)
# - Download from internet to cache (download-mode)
#
################################################################################
import csv
import math
import random
import numpy

from fourcircle_utility import *
from peakinfo import PeakInfo

import mantid
import mantid.simpleapi as api
from mantid.api import AnalysisDataService
from mantid.kernel import V3D

DebugMode = True

DET_X_SIZE = 256
DET_Y_SIZE = 256

MAX_SCAN_NUMBER = 100000


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
        # self._myUBPeakWSDict = dict()
        # Container for UB  matrix
        self._myUBMatrixDict = dict()

        # Peak Info
        self._myPeakInfoDict = dict()
        # Last UB matrix calculated
        self._myLastPeakUB = None
        # Flag for data storage
        self._cacheDataOnly = False

        # Dictionary to store survey information
        self._scanSummaryList = list()

        # Tuple to hold the result of refining UB
        self._refinedUBTup = None

        # Record for merged scans
        self._mergedWSManager = list()

        # Region of interest: key = (experiment, scan), value = 2-tuple of 2-tuple: ( (lx, ly), (ux, uy))
        self._roiDict = dict()

        # A dictionary to manage all loaded and processed MDEventWorkspaces
        # self._expDataDict = {}

        return

    def _add_merged_ws(self, exp_number, scan_number, pt_number_list):
        """ Record a merged workspace to
        Requirements: experiment number, scan number and pt numbers are valid
        :param exp_number:
        :param scan_number:
        :param pt_number_list:
        :return:
        """
        assert isinstance(exp_number, int) and isinstance(scan_number, int)
        assert isinstance(pt_number_list, list) and len(pt_number_list) > 0

        if (exp_number, scan_number, pt_number_list) in self._mergedWSManager:
            return 'Exp %d Scan %d Pt %s has already been merged and recorded.' % (exp_number,
                                                                                   scan_number,
                                                                                   str(pt_number_list))

        self._mergedWSManager.append((exp_number, scan_number, pt_number_list))
        self._mergedWSManager.sort()

        return

    @staticmethod
    def apply_mask(exp_number, scan_number, pt_number):
        """

        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return:
        """
        # TODO/NOW - Doc and check
        # assert ... ...

        # get workspaces' names
        raw_pt_ws_name = get_raw_data_workspace_name(exp_number, scan_number, pt_number)
        mask_ws_name = get_mask_ws_name(exp_number, scan_number)

        # TODO/NOW - check workspace existing
        # if ... raise

        api.MaskDetectors(Workspace=raw_pt_ws_name, MaskedWorkspace=mask_ws_name)

        return

    def calculate_ub_matrix(self, peak_info_list, a, b, c, alpha, beta, gamma):
        """
        Calculate UB matrix
        Requirements: two or more than 2 peaks (PeakInfo) are specified
        Set Miller index from raw data in Workspace2D.
        :param peak_info_list:
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
        num_peak_info = len(peak_info_list)
        if num_peak_info < 2:
            return False, 'Too few peaks are input to calculate UB matrix.  Must be >= 2.'
        for peak_info in peak_info_list:
            if isinstance(peak_info, PeakInfo) is False:
                raise NotImplementedError('Input PeakList is of type %s.' % str(type(peak_info_list[0])))
            assert isinstance(peak_info, PeakInfo)

        # Construct a new peak workspace by combining all single peak
        ub_peak_ws_name = 'Temp_UB_Peak'
        zero_hkl = False
        hkl_to_int = True
        self._build_peaks_workspace(peak_info_list, ub_peak_ws_name, zero_hkl, hkl_to_int)

        # Calculate UB matrix
        try:
            api.CalculateUMatrix(PeaksWorkspace=ub_peak_ws_name,
                                 a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        except ValueError as val_err:
            return False, str(val_err)

        ub_peak_ws = AnalysisDataService.retrieve(ub_peak_ws_name)
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
        file_url = get_spice_file_url(self._myServerURL, self._instrumentName, exp_number, scan_number)

        file_name = get_spice_file_name(self._instrumentName, exp_number, scan_number)
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
        det_xml_file_name = get_det_xml_file_name(self._instrumentName, exp_no, scan_no, pt_no)
        local_xml_file_name = os.path.join(self._dataDir, det_xml_file_name)
        if os.path.exists(local_xml_file_name) is True and overwrite is False:
            return True, local_xml_file_name

        # Generate the URL for XML file
        det_file_url = get_det_xml_file_url(self._myServerURL, self._instrumentName, exp_no, scan_no, pt_no)

        # Download
        try:
            api.DownloadFile(Address=det_file_url,
                             Filename=local_xml_file_name)
        except RuntimeError as run_err:
            return False, 'Unable to download Detector XML file %s from %s ' \
                          'due to %s.' % (local_xml_file_name, det_file_url, str(run_err))

        # Check file exist?
        if os.path.exists(local_xml_file_name) is False:
            return False, "Unable to locate downloaded file %s." % local_xml_file_name

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

    def does_file_exist(self, exp_number, scan_number, pt_number=None):
        """
        Check whether data file for a scan or pt number exists on the
        :param exp_number: experiment number or None (default to current experiment number)
        :param scan_number:
        :param pt_number: if None, check SPICE file; otherwise, detector xml file
        :return:
        """
        # check inputs
        assert isinstance(exp_number, int) or pt_number is None
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int) or pt_number is None

        # deal with default experiment number
        if exp_number is None:
            exp_number = self._expNumber

        # 2 cases
        if pt_number is None:
            # no pt number, then check SPICE file
            spice_file_name = get_spice_file_name(self._instrumentName, exp_number, scan_number)
            file_name = os.path.join(self._dataDir, spice_file_name)
        else:
            # pt number given, then check
            xml_file_name = get_det_xml_file_name(self._instrumentName, exp_number, scan_number,
                                                  pt_number)
            file_name = os.path.join(self._dataDir, xml_file_name)
        # END-IF

        return os.path.exists(file_name)

    def export_md_data(self, exp_number, scan_number, base_file_name):
        """
        Export MD data to an external file
        :param exp_number:
        :param scan_number:
        :param base_file_name:
        :return: output file name
        """
        # get output file name and source workspace name
        out_file_name = os.path.join(self._workDir, base_file_name)

        status, pt_list = self.get_pt_numbers(exp_number, scan_number)
        assert status, pt_list
        md_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_list)
        temp_out_ws = base_file_name

        api.ConvertCWSDMDtoHKL(InputWorkspace=md_ws_name,
                               UBMatrix='1., 0., 0., 0., 1., 0., 0., 0., 1',
                               OutputWorkspace=temp_out_ws,
                               QSampleFileName=out_file_name)
        api.DeleteWorkspace(Workspace=temp_out_ws)

        return out_file_name

    def find_peak(self, exp_number, scan_number, pt_number_list=None):
        """ Find 1 peak in sample Q space for UB matrix
        :param exp_number:
        :param scan_number:
        :param pt_number_list:
        :return:tuple as (boolean, object) such as (false, error message) and (true, PeakInfo object)

        This part will be redo as 11847_Load_HB3A_Experiment
        """
        # Check & set pt. numbers
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        if pt_number_list is None:
            status, pt_number_list = self.get_pt_numbers(exp_number, scan_number)
            assert status
        assert isinstance(pt_number_list, list) and len(pt_number_list) > 0

        # Check whether the MDEventWorkspace used to find peaks exists
        if self.has_merged_data(exp_number, scan_number, pt_number_list):
            pass
        else:
            raise RuntimeError('Data must be merged before')

        # Find peak in Q-space
        merged_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_number_list)
        peak_ws_name = get_peak_ws_name(exp_number, scan_number, pt_number_list)
        print '[DB] Found peaks are output workspace %s.' % peak_ws_name
        api.FindPeaksMD(InputWorkspace=merged_ws_name,
                        MaxPeaks=10,
                        PeakDistanceThreshold=5.,
                        DensityThresholdFactor=0.1,
                        OutputWorkspace=peak_ws_name)
        assert AnalysisDataService.doesExist(peak_ws_name)

        # add peak to UB matrix workspace to manager
        self._set_peak_info(exp_number, scan_number, peak_ws_name, merged_ws_name)

        # add the merged workspace to list to manage
        self._add_merged_ws(exp_number, scan_number, pt_number_list)

        return True, ''

    def get_experiment(self):
        """
        Get experiment number
        :return:
        """
        return self._expNumber

    def get_pt_numbers(self, exp_no, scan_no):
        """ Get Pt numbers (as a list) for a scan in an experiment
        :param exp_no:
        :param scan_no:
        :return: (Boolean, Object) as (status, pt number list/error message)
        """
        # Check
        if exp_no is None:
            exp_no = self._expNumber
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)

        # Get workspace
        status, ret_obj = self.load_spice_scan_file(exp_no, scan_no)
        if status is False:
            return False, ret_obj
        else:
            table_ws_name = ret_obj
            table_ws = AnalysisDataService.retrieve(table_ws_name)

        # Get column for Pt.
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

        # Rotate the output matrix
        # array2d = numpy.rot90(array2d, 1)
        # FIXME - Later
        array2d = numpy.flipud(array2d)

        return array2d

    def get_refined_ub_matrix(self):
        """
        Get refined UB matrix and lattice parameters
        :return:
        """
        assert isinstance(self._refinedUBTup, tuple)
        assert len(self._refinedUBTup) == 4

        return self._refinedUBTup[1], self._refinedUBTup[2], self._refinedUBTup[3]

    def get_region_of_interest(self, exp_number, scan_number):
        """ Get region of interest
        :param exp_number:
        :param scan_number:
        :return:
        """
        # check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int) or scan_number is None

        if (exp_number, scan_number) in self._roiDict:
            # able to find region of interest for this scan
            ret_status = True
            ret_value = self._roiDict[(exp_number, scan_number)]
        elif exp_number in self._roiDict:
            # able to find region of interest for this experiment
            ret_status = True
            ret_value = self._roiDict[exp_number]
        else:
            # region of interest of experiment is not defined
            ret_status = False
            ret_value = 'Unable to find ROI for experiment %d. Existing includes %s.' % (exp_number,
                                                                                         str(self._roiDict.keys()))

        return ret_status, ret_value

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
            status, pt_number_list = self.get_pt_numbers(exp_number, scan_number)
            assert status
            md_ws_name = get_merged_md_name(self._instrumentName, exp_number,
                                            scan_number, pt_number_list)
            md_ws = AnalysisDataService.retrieve(md_ws_name)
        except KeyError as ke:
            return 'Unable to find log value %s due to %s.' % (log_name, str(ke))

        return md_ws.getExperimentInfo(0).run().getProperty(log_name).value

    def get_merged_data(self, exp_number, scan_number, pt_number_list):
        """
        Get merged data in format of numpy.ndarray to plot
        :param exp_number:
        :param scan_number:
        :return: numpy.ndarray. shape = (?, 3)
        """
        # check
        assert isinstance(exp_number, int) and isinstance(scan_number, int)
        assert isinstance(pt_number_list, list)

        # get MDEventWorkspace
        md_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_number_list)
        assert AnalysisDataService.doesExist(md_ws_name)

        # call ConvertCWMDtoHKL to write out the temp file
        base_name = 'temp_%d_%d_rand%d' % (exp_number, scan_number, random.randint(1, 10000))
        out_file_name = self.export_md_data(exp_number, scan_number, base_name)

        # load the merged data back from the ASCII data file
        q_space_array, counts_array = load_hb3a_md_data(out_file_name)

        return q_space_array, counts_array

    def get_merged_scans(self):
        """
        Get merged scans and Pts.
        :return:
        """
        return self._mergedWSManager[:]

    def get_peak_info(self, exp_number, scan_number, pt_number=None):
        """
        get PeakInfo instance
        :param exp_number: experiment number
        :param scan_number:
        :param pt_number:
        :return: PeakInfo instance
        """
        # Check for type
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int) or pt_number is None

        # construct key
        if pt_number is None:
            p_key = (exp_number, scan_number)
        else:
            p_key = (exp_number, scan_number, pt_number)

        # Check for existence
        assert p_key in self._myPeakInfoDict, 'Exp/Scan/Pt %s does not exist in PeakInfo dictionary.' % str(p_key)

        return self._myPeakInfoDict[p_key]

    def get_peaks_integrated_intensities(self, exp_number, scan_number, pt_list):
        """

        :param exp_number:
        :param scan_number:
        :param pt_list:
        :return:
        """
        # TODO/NOW: doc & check

        if pt_list is None:
            status, pt_list = self.get_pt_numbers(exp_number, scan_number)
            assert status
        int_peak_ws_name = get_integrated_peak_ws_name(exp_number, scan_number, pt_list)

        assert AnalysisDataService.doesExist(int_peak_ws_name)
        int_peak_ws = AnalysisDataService.retrieve(int_peak_ws_name)

        num_peaks = int_peak_ws.getNumberPeaks()
        array_size = num_peaks
        vec_x = numpy.ndarray(shape=(array_size,))
        vec_y = numpy.ndarray(shape=(array_size,))
        for index in xrange(array_size):
            peak_i = int_peak_ws.getPeak(index)
            pt_number = peak_i.getRunNumber() % 1000
            intensity = peak_i.getIntensity()
            vec_x[index] = pt_number
            vec_y[index] = intensity
        # END-FOR

        return vec_x, vec_y

    def generate_mask_workspace(self, exp_number, scan_number, roi_start, roi_end):
        """ Generate a mask workspace
        :param exp_number:
        :param scan_number:
        :param roi_start:
        :param roi_end:
        :return:
        """
        # TODO/NOW : check ...
        # assert ...
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)

        # create an xml file
        mask_file_name = get_mask_xml_temp(self._workDir, exp_number, scan_number)
        generate_mask_file(file_path=mask_file_name,
                           ll_corner=roi_start,
                           ur_corner=roi_end)

        # load the mask workspace
        mask_ws_name = get_mask_ws_name(exp_number, scan_number)
        api.LoadMask(Instrument='HB3A',
                     InputFile=mask_file_name,
                     OutputWorkspace=mask_ws_name)
        api.InvertMask(InputWorkspace=mask_ws_name,
                       OutputWorkspace=mask_ws_name)

        return True, mask_ws_name

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

    def has_merged_data(self, exp_number, scan_number, pt_number_list=None):
        """
        Check whether the data has been merged to an MDEventWorkspace
        :param exp_number:
        :param scan_number:
        :param pt_number_list:
        :return:
        """
        # check and retrieve pt number list
        assert isinstance(exp_number, int) and isinstance(scan_number, int)
        if pt_number_list is None:
            status, pt_number_list = self.get_pt_numbers(exp_number, scan_number)
            if status is False:
                return False
        else:
            assert isinstance(pt_number_list, list)

        # get MD workspace name
        md_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_number_list)

        return AnalysisDataService.doesExist(md_ws_name)

    def has_peak_info(self, exp_number, scan_number, pt_number=None):
        """ Check whether there is a peak found...
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :return:
        """
        # Check for type
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_number, int) or pt_number is None

        # construct key
        if pt_number is None:
            p_key = (exp_number, scan_number)
        else:
            p_key = (exp_number, scan_number, pt_number)

        return p_key in self._myPeakInfoDict

    def index_peak(self, ub_matrix, scan_number):
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

        # Find out the PeakInfo
        exp_number = self._expNumber
        peak_info = self.get_peak_info(exp_number, scan_number)

        # Find out the peak workspace
        status, pt_list = self.get_pt_numbers(exp_number, scan_number)
        assert status
        peak_ws_name = get_peak_ws_name(exp_number, scan_number, pt_list)
        peak_ws = AnalysisDataService.retrieve(peak_ws_name)
        assert peak_ws.getNumberPeaks() > 0

        # Create a temporary peak workspace for indexing
        temp_index_ws_name = 'TempIndexExp%dScan%dPeak' % (exp_number, scan_number)
        api.CreatePeaksWorkspace(NumberOfPeaks=0, OutputWorkspace=temp_index_ws_name)
        temp_index_ws = AnalysisDataService.retrieve(temp_index_ws_name)

        temp_index_ws.addPeak(peak_ws.getPeak(0))
        virtual_peak = temp_index_ws.getPeak(0)
        virtual_peak.setHKL(0, 0, 0)
        virtual_peak.setQSampleFrame(peak_info.get_peak_centre_v3d())

        # Set UB matrix to the peak workspace
        ub_1d = ub_matrix.reshape(9,)

        # Set UB
        api.SetUB(Workspace=temp_index_ws_name, UB=ub_1d)

        # Note: IndexPeaks and CalculatePeaksHKL do the same job
        #       while IndexPeaks has more control on the output
        num_peak_index, error = api.IndexPeaks(PeaksWorkspace=temp_index_ws_name,
                                               Tolerance=0.4,
                                               RoundHKLs=False)
        temp_index_ws = AnalysisDataService.retrieve(temp_index_ws_name)

        if num_peak_index == 0:
            return False, 'No peak can be indexed.'
        elif num_peak_index > 1:
            raise RuntimeError('Case for PeaksWorkspace containing more than 1 peak is not '
                               'considered. Contact developer for this issue.')
        else:
            hkl_v3d = temp_index_ws.getPeak(0).getHKL()
            hkl = [hkl_v3d.X(), hkl_v3d.Y(), hkl_v3d.Z()]

        peak_info.set_indexed_hkl(hkl)

        # delete temporary workspace
        api.DeleteWorkspace(Workspace=temp_index_ws_name)

        return True, (hkl, error)

    def integrate_peak(self, exp_num, scan_num, pt_num, peak_center, peak_radius):
        """
        Integrate a peak from an individual pt number and with a given peak centre in Q
        :param exp_num:
        :param scan_num:
        :param pt_num:
        :param peak_center:
        :param peak_radius:
        :return: 3-tuple as peak workspace name, intensity and counts
        """
        # Check
        assert isinstance(exp_num, int) and isinstance(scan_num, int) and isinstance(pt_num, int)

        # Merge
        merged_ws_name = get_merged_md_name(self._instrumentName, exp_num, scan_num, [pt_num])
        if AnalysisDataService.doesExist(merged_ws_name) is False:
            self.merge_pts_in_scan(exp_num, scan_num, [pt_num], 'q-sample')
        assert AnalysisDataService.doesExist(merged_ws_name)

        # Build peak workspace
        peak_ws_name = get_peak_ws_name(exp_num, scan_num, [pt_num])
        peak_info = PeakInfo(exp_num, scan_num, peak_ws_name)
        zero_hkl = True
        hkl_to_int = False
        self._build_peaks_workspace([peak_info], peak_ws_name, zero_hkl, hkl_to_int)
        assert AnalysisDataService.doesExist(peak_ws_name)

        # Integrate by rewriting the temporary input peak workspace
        api.IntegratePeaksCWSD(InputWorkspace=merged_ws_name,
                               PeaksWorkspace=peak_ws_name,
                               OutputWorkspace=peak_ws_name,
                               PeakCentre=peak_center,
                               PeakRadius=peak_radius,
                               MergePeaks=False)

        # Get integrated value
        peak_ws = AnalysisDataService.retrieve(peak_ws_name)
        intensity = peak_ws.getPeak(0).getIntensity()
        counts = peak_ws.getPeak(0).getBinCounts()

        return peak_ws_name, intensity, counts

    def integrate_scan_peaks(self, exp, scan, peak_radius, peak_centre,
                             merge=True):
        """

        :param exp:
        :param scan:
        :param peak_centre:
        :param merge:
        :return:
        """
        # TODO/NOW - documentation and check

        # FIXME - combine the download and naming for common use
        # get spice file
        spice_table_name = get_spice_table_name(exp, scan)
        if AnalysisDataService.doesExist(spice_table_name) is False:
            self.download_spice_file(exp, scan, False)
            self.load_spice_scan_file(exp, scan)

        # get MD workspace name
        status, pt_list = self.get_pt_numbers(exp, scan)
        assert status
        md_ws_name = get_merged_md_name(self._instrumentName, exp, scan, pt_list)

        peak_centre_str = '%f, %f, %f' % (peak_centre[0], peak_centre[1],
                                          peak_centre[2])

        integrated_peak_ws_name = get_integrated_peak_ws_name(exp, scan, pt_list)
        api.IntegratePeaksCWSD(InputWorkspace=md_ws_name,
                               OutputWorkspace=integrated_peak_ws_name,
                               PeakRadius=peak_radius,
                               PeakCentre=peak_centre_str,
                               MergePeaks=merge,
                               NormalizeByMonitor=True,
                               NormalizeByTime=False)

        return

    def integrate_peaks_q(self, exp_no, scan_no):
        """
        Integrate peaks in Q-space
        :param exp_no:
        :param scan_no:
        :return:
        """
        # Check inputs
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)

        # Get the SPICE file
        spice_table_name = get_spice_table_name(exp_no, scan_no)
        if AnalysisDataService.doesExist(spice_table_name) is False:
            self.download_spice_file(exp_no, scan_no, False)
            self.load_spice_scan_file(exp_no, scan_no)

        # Find peaks & get the peak centers
        spice_table = AnalysisDataService.retrieve(spice_table_name)
        num_rows = spice_table.rowCount()

        sum_peak_center = [0., 0., 0.]
        sum_bin_counts = 0.

        for i_row in xrange(num_rows):
            pt_no = spice_table.cell(i_row, 0)
            self.download_spice_xml_file(scan_no, pt_no, exp_no)
            # self.load_spice_xml_file(exp_no, scan_no, pt_no)
            self.find_peak(exp_no, scan_no, pt_no)
            peak_ws_name = get_peak_ws_name(exp_no, scan_no, pt_no)
            peak_ws = AnalysisDataService.retrieve(peak_ws_name)
            if peak_ws.getNumberPeaks() == 1:
                peak = peak_ws.getPeak(0)
                peak_center = peak.getQSampleFrame()
                bin_count = peak.getBinCount()

                sum_peak_center[0] += bin_count * peak_center.X()
                sum_peak_center[1] += bin_count * peak_center.Y()
                sum_peak_center[2] += bin_count * peak_center.Z()

                sum_bin_counts += bin_count

            elif peak_ws.getNumberPeaks() > 1:
                raise NotImplementedError('More than 1 peak???')
        # END-FOR

        final_peak_center = [0., 0., 0.]
        for i in xrange(3):
            final_peak_center[i] = sum_peak_center[i] * (1./sum_bin_counts)
        #final_peak_center = sum_peak_center * (1./sum_bin_counts)

        print 'Avg peak center = ', final_peak_center, 'Total counts = ', sum_bin_counts

        # Integrate peaks
        total_intensity = 0.
        for i_row in xrange(num_rows):
            pt_no = spice_table.cell(i_row, 0)
            md_ws_name = get_single_pt_md_name(exp_no, scan_no, pt_no)
            peak_ws_name = get_peak_ws_name(exp_no, scan_no, pt_no)
            out_ws_name = peak_ws_name + '_integrated'
            api.IntegratePeaksCWSD(InputWorkspace=md_ws_name,
                                   PeaksWorkspace=peak_ws_name,
                                   OutputWorkspace=out_ws_name)
            out_peak_ws = AnalysisDataService.retrieve(out_ws_name)
            peak = out_peak_ws.getPeak(0)
            intensity = peak.getIntensity()
            total_intensity += intensity
        # END-FOR

        return total_intensity

    def integrate_peaks(self, exp_no, scan_no, pt_list, md_ws_name,
                        peak_radius, bkgd_inner_radius, bkgd_outer_radius,
                        is_cylinder):
        """
        Integrate peaks
        :return: Boolean as successful or failed
        """
        # Check input
        if is_cylinder is True:
            raise RuntimeError('Cylinder peak shape has not been implemented yet!')

        if exp_no is None:
            exp_no = self._expNumber
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(peak_radius, float)
        assert isinstance(bkgd_inner_radius, float)
        assert isinstance(bkgd_outer_radius, float)
        assert bkgd_inner_radius >= peak_radius
        assert bkgd_outer_radius >= bkgd_inner_radius

        # NEXT - Need to re-write this method according to documentation of IntegratePeaksCWSD()

        # Get MD WS
        if md_ws_name is None:
            raise RuntimeError('Implement how to locate merged MD workspace name from '
                               'Exp %d Scan %d Pt %s' % (exp_no, scan_no, str(pt_list)))
        # Peak workspace
        # create an empty peak workspace
        if AnalysisDataService.doesExist('spicematrixws') is False:
            raise RuntimeError('Workspace spicematrixws does not exist.')
        api.LoadInstrument(Workspace='', InstrumentName='HB3A')
        target_peak_ws_name = 'MyPeakWS'
        api.CreatePeaksWorkspace(InstrumentWorkspace='spicematrixws', OutputWorkspace=target_peak_ws_name)
        target_peak_ws = AnalysisDataService.retrieve(target_peak_ws_name)
        # copy a peak
        temp_peak_ws_name = 'peak1'
        api.FindPeaksMD(InputWorkspace='MergedSan0017_QSample',
                        PeakDistanceThreshold=0.5,
                        MaxPeaks=10,
                        DensityThresholdFactor=100,
                        OutputWorkspace=temp_peak_ws_name)

        src_peak_ws = AnalysisDataService.retrieve(temp_peak_ws_name)
        centre_peak = src_peak_ws.getPeak(0)
        target_peak_ws.addPeak(centre_peak)
        target_peak_ws.removePeak(0)

        # Integrate peak
        api.IntegratePeaksMD(InputWorkspace='MergedSan0017_QSample',
                             PeakRadius=1.5,
                             BackgroundInnerRadius=1.5,
                             BackgroundOuterRadius=3,
                             PeaksWorkspace=target_peak_ws_name,
                             OutputWorkspace='SinglePeak1',
                             IntegrateIfOnEdge=False,
                             AdaptiveQBackground=True,
                             Cylinder=False)

        raise RuntimeError('Implement ASAP!')

    @staticmethod
    def load_scan_survey_file(csv_file_name):
        """ Load scan survey from a csv file
        :param csv_file_name:
        :return: 2-tuple as header and list
        """
        # check
        assert isinstance(csv_file_name, str)
        row_list = list()

        # open file and parse
        with open(csv_file_name, 'r') as csv_file:
            reader = csv.reader(csv_file, delimiter=',', quotechar='|')

            # get header
            header = reader.next()

            # body
            for row in reader:
                # check
                assert isinstance(row, list)
                assert len(row) == 7
                # convert
                counts = float(row[0])
                scan = int(row[1])
                pt = int(row[2])
                h = float(row[3])
                k = float(row[4])
                l = float(row[5])
                q_range = float(row[6])
                # append
                row_list.append([counts, scan, pt, h, k, l, q_range])
            # END-FOR
        # END-WITH

        return header, row_list

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
            spice_file_name = os.path.join(self._dataDir,
                                           get_spice_file_name(self._instrumentName, exp_no, scan_no))

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
        Load SPICE's detector counts XML file from local data directory
        Requirements: the SPICE detector counts file does exist. The XML file's name is given either
                    explicitly by user or formed according to a convention with given experiment number,
                    scan number and Pt number
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :param xml_file_name:
        :return:
        """
        # Get XML file name with full path
        if xml_file_name is None:
            # use default
            assert isinstance(exp_no, int) and isinstance(scan_no, int) and isinstance(pt_no, int)
            xml_file_name = os.path.join(self._dataDir, get_det_xml_file_name(self._instrumentName,
                                                                              exp_no, scan_no, pt_no))
        # END-IF

        # check whether file exists
        assert os.path.exists(xml_file_name)

        # retrieve and check SPICE table workspace
        spice_table_ws = self._get_spice_workspace(exp_no, scan_no)
        assert isinstance(spice_table_ws, mantid.dataobjects.TableWorkspace)
        spice_table_name = spice_table_ws.name()

        # load SPICE Pt.  detector file
        pt_ws_name = get_raw_data_workspace_name(exp_no, scan_no, pt_no)
        try:
            api.LoadSpiceXML2DDet(Filename=xml_file_name,
                                  OutputWorkspace=pt_ws_name,
                                  DetectorGeometry='256,256',
                                  SpiceTableWorkspace=spice_table_name,
                                  PtNumber=pt_no)
        except RuntimeError as run_err:
            return False, str(run_err)

        # Add data storage
        assert AnalysisDataService.doesExist(pt_ws_name)
        raw_matrix_ws = AnalysisDataService.retrieve(pt_ws_name)
        self._add_raw_workspace(exp_no, scan_no, pt_no, raw_matrix_ws)

        return True, pt_ws_name

    def merge_pts_in_scan(self, exp_no, scan_no, pt_num_list, target_frame):
        """
        Merge Pts in Scan
        All the workspaces generated as internal results will be grouped
        Requirements:
          1. target_frame must be either 'q-sample' or 'hkl'
          2. pt_list must be a list.  an empty list means to merge all Pts. in the scan
        Guarantees: An MDEventWorkspace is created containing merged Pts.
        :param exp_no:
        :param scan_no:
        :param pt_num_list: If empty, then merge all Pt. in the scan
        :param target_frame: string, either 'hkl' or 'q-sample'
        :return: (boolean, error message) # (merged workspace name, workspace group name)
        """
        # Check
        if exp_no is None:
            exp_no = self._expNumber
        assert isinstance(exp_no, int) and isinstance(scan_no, int)
        assert isinstance(pt_num_list, list), 'Pt number list must be a list but not %s' % str(type(pt_num_list))
        assert isinstance(target_frame, str)

        # Get list of Pt.
        if len(pt_num_list) > 0:
            # user specified
            pt_num_list = pt_num_list
        else:
            # default: all Pt. of scan
            status, pt_num_list = self.get_pt_numbers(exp_no, scan_no)
            if status is False:
                err_msg = pt_num_list
                return False, err_msg
        # END-IF-ELSE

        # construct a list of Pt as the input of CollectHB3AExperimentInfo
        pt_list_str = '-1'  # header
        err_msg = ''
        for pt in pt_num_list:
            # Download file
            try:
                self.download_spice_xml_file(scan_no, pt, exp_no=exp_no, overwrite=False)
            except RuntimeError as e:
                err_msg += 'Unable to download xml file for pt %d due to %s\n' % (pt, str(e))
                continue
            pt_list_str += ',%d' % pt
        # END-FOR (pt)
        print '[DB] Pt list = %s' % pt_list_str
        if pt_list_str == '-1':
            return False, err_msg

        # Collect HB3A Exp/Scan information
        # construct a configuration with 1 scan and multiple Pts.
        scan_info_ws_name = get_merge_pt_info_ws_name(exp_no, scan_no)
        try:
            api.CollectHB3AExperimentInfo(ExperimentNumber=exp_no,
                                          ScanList='%d' % scan_no,
                                          PtLists=pt_list_str,
                                          DataDirectory=self._dataDir,
                                          GenerateVirtualInstrument=False,
                                          OutputWorkspace=scan_info_ws_name,
                                          DetectorTableWorkspace='MockDetTable')
        except RuntimeError as rt_error:
            return False, 'Unable to merge scan %d dur to %s.' % (scan_no, str(rt_error))
        else:
            assert AnalysisDataService.doesExist(scan_info_ws_name)

        # Convert to Q-sample
        out_q_name = get_merged_md_name(self._instrumentName, exp_no, scan_no, pt_num_list)
        if AnalysisDataService.doesExist(out_q_name) is False:
            try:
                api.ConvertCWSDExpToMomentum(InputWorkspace=scan_info_ws_name,
                                             CreateVirtualInstrument=False,
                                             OutputWorkspace=out_q_name,
                                             Directory=self._dataDir)
            except RuntimeError as e:
                err_msg += 'Unable to convert scan %d data to Q-sample MDEvents due to %s' % (scan_no, str(e))
                return False, err_msg

        # Optionally converted to HKL space # Target frame
        if target_frame.lower().startswith('hkl'):
            # retrieve UB matrix stored and convert to a 1-D array
            assert exp_no in self._myUBMatrixDict
            ub_matrix_1d = self._myUBMatrixDict[exp_no].reshape(9,)
            # convert to HKL
            out_hkl_name = get_merged_hkl_md_name(self._instrumentName, exp_no, scan_no, pt_num_list)
            try:
                api.ConvertCWSDMDtoHKL(InputWorkspace=out_q_name,
                                       UBMatrix=ub_matrix_1d,
                                       OutputWorkspace=out_hkl_name)

            except RuntimeError as e:
                err_msg += 'Failed to reduce scan %d due to %s' % (scan_no, str(e))
                return False, err_msg

            # set up output
            out_ws_name = out_hkl_name

        elif target_frame.lower().startswith('q-sample'):
            # Q-sample
            out_ws_name = out_q_name

        else:
            # Unsupported
            raise RuntimeError('Target frame %s is not supported.' % target_frame)

        return True, (out_ws_name, '')

    def set_roi(self, exp_number, scan_number, lower_left_corner, upper_right_corner):
        """

        :param exp_number:
        :param scan_number:
        :param lower_left_corner:
        :param upper_right_corner:
        :return:
        """
        # Check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert not isinstance(lower_left_corner, str) and len(lower_left_corner) == 2
        assert not isinstance(upper_right_corner, str) and len(upper_right_corner) == 2

        ll_x = int(lower_left_corner[0])
        ll_y = int(lower_left_corner[1])
        ur_x = int(upper_right_corner[0])
        ur_y = int(upper_right_corner[1])
        assert ll_x < ur_x and ll_y < ur_y

        # Add to dictionary.  Because usually one ROI is defined for all scans in an experiment,
        # then it is better and easier to support client to search this ROI by experiment number
        # and only the latest is saved by this key
        self._roiDict[(exp_number, scan_number)] = ((ll_x, ll_y), (ur_x, ur_y))
        self._roiDict[exp_number] = ((ll_x, ll_y), (ur_x, ur_y))

        return

    def set_server_url(self, server_url, check_link=True):
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
        if check_link:
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
        else:
            is_url_good = True
            error_message = ''

        return is_url_good, error_message

    def set_web_access_mode(self, mode):
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

    def refine_ub_matrix_indexed_peaks(self, peak_info_list, set_hkl_int):
        """ Refine UB matrix by indexed peaks
        Requirements: input is a list of PeakInfo objects and there are at least 3
                        non-degenerate peaks
        Guarantees: UB matrix is refined.  Refined UB matrix and lattice parameters
                    with errors are returned
        :param peak_info_list: list of PeakInfo
        :param set_hkl_int: set HKL to nearest integer
        :return: 2-tuple: (True, (ub matrix, lattice parameters, lattice parameters errors))
                          (False, error message)
        """
        # Check inputs
        assert isinstance(peak_info_list, list)
        assert len(peak_info_list) >= 3

        # Construct a new peak workspace by combining all single peak
        ub_peak_ws_name = 'TempUBIndexedPeaks'
        self._build_peaks_workspace(peak_info_list, ub_peak_ws_name, False, set_hkl_int)

        # Calculate UB matrix
        try:
            api.FindUBUsingIndexedPeaks(PeaksWorkspace=ub_peak_ws_name, Tolerance=0.5)
        except RuntimeError as e:
            return False, 'Unable to refine UB matrix due to %s.' % str(e)

        # Get peak workspace
        self._refinedUBTup = self._get_refined_ub_data(ub_peak_ws_name)

        return

    def refine_ub_matrix_least_info(self, peak_info_list, d_min, d_max):
        """
        Refine UB matrix with least information from user, i.e., using FindUBFFT
        Requirements: at least 6 PeakInfo objects are given
        Guarantees: Refine UB matrix by FFT
        :return:
        """
        # Check
        assert isinstance(peak_info_list, list) and len(peak_info_list) >= 6
        assert 0 < d_min < d_max

        # Build a new PeaksWorkspace
        peak_ws_name = 'TempUBFFTPeaks'
        self._build_peaks_workspace(peak_info_list, peak_ws_name, True, False)

        # Refine
        api.FindUBUsingFFT(PeaksWorkspace=peak_ws_name,
                           Tolerance=0.15,
                           MinD=d_min,
                           MaxD=d_max)

        # Get result
        self._refinedUBTup = self._get_refined_ub_data(peak_ws_name)

        return

    @staticmethod
    def _get_refined_ub_data(peak_ws_name):
        """ Get UB matrix, lattice parameters and their errors from refined UB matrix
        :param peak_ws_name:
        :return:
        """
        peak_ws = AnalysisDataService.retrieve(peak_ws_name)
        assert peak_ws is not None

        oriented_lattice = peak_ws.sample().getOrientedLattice()

        refined_ub_matrix = oriented_lattice.getUB()
        lattice = [oriented_lattice.a(), oriented_lattice.b(),
                   oriented_lattice.c(), oriented_lattice.alpha(),
                   oriented_lattice.beta(), oriented_lattice.gamma()]
        lattice_error = [oriented_lattice.errora(), oriented_lattice.errorb(),
                         oriented_lattice.errorc(), oriented_lattice.erroralpha(),
                         oriented_lattice.errorbeta(), oriented_lattice.errorgamma()]

        print '[DB-BAT] Refined UB = ', refined_ub_matrix, 'of type', type(refined_ub_matrix)

        result_tuple = (peak_ws, refined_ub_matrix, lattice, lattice_error)

        return result_tuple

    @staticmethod
    def _build_peaks_workspace(peak_info_list, peak_ws_name, zero_hkl, hkl_to_int):
        """ From a list of PeakInfo, using the averaged peak centre of each of them
        to build a new PeaksWorkspace
        Requirements: a list of PeakInfo
        Guarantees: a PeaksWorkspace is created in AnalysisDataService.
        :param peak_info_list:
        :param peak_ws_name:
        :return:
        """
        # check
        assert isinstance(peak_info_list, list)
        assert len(peak_info_list) > 0
        assert isinstance(peak_ws_name, str)

        # create an empty
        api.CreatePeaksWorkspace(NumberOfPeaks=0, OutputWorkspace=peak_ws_name)
        assert AnalysisDataService.doesExist(peak_ws_name)
        peak_ws = AnalysisDataService.retrieve(peak_ws_name)

        # add peak
        num_peak_info = len(peak_info_list)
        for i_peak_info in xrange(num_peak_info):
            # Set HKL as optional
            peak_info_i = peak_info_list[i_peak_info]
            peak_ws_i = peak_info_i.get_peak_workspace()
            assert peak_ws_i.getNumberPeaks() > 0
            # get any peak to add
            peak_temp = peak_ws_i.getPeak(0)
            peak_ws.addPeak(peak_temp)

            # set the peak in ub peak workspace right
            peak_i = peak_ws.getPeak(i_peak_info)
            # user HKL
            if zero_hkl is True:
                h = k = l = 0.
            else:
                h, k, l = peak_info_i.get_user_hkl()
                if hkl_to_int:
                    # convert hkl to integer
                    h = float(math.copysign(1, h)*int(abs(h)+0.5))
                    k = float(math.copysign(1, k)*int(abs(k)+0.5))
                    l = float(math.copysign(1, l)*int(abs(l)+0.5))
            # END-IF
            peak_i.setHKL(h, k, l)
            # q-sample
            q_x, q_y, q_z = peak_info_i.get_peak_centre()
            q_sample = V3D(q_x, q_y, q_z)
            peak_i.setQSampleFrame(q_sample)
        # END-FOR(i_peak_info)

        return

    def save_scan_survey(self, file_name):
        """
        Save scan-survey's result to a csv file
        :param file_name:
        :return:
        """
        # Check requirements
        assert isinstance(file_name, str)
        assert len(self._scanSummaryList) > 0

        # Sort
        self._scanSummaryList.sort(reverse=True)

        # File name
        if file_name.endswith('.csv') is False:
            file_name = '%s.csv' % file_name

        # Write file
        titles = ['Max Counts', 'Scan', 'Max Counts Pt', 'H', 'K', 'L', 'Q']
        with open(file_name, 'w') as csvfile:
            csv_writer = csv.writer(csvfile, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
            csv_writer.writerow(titles)

            for scan_summary in self._scanSummaryList:
                # check type
                assert isinstance(scan_summary, list)
                assert len(scan_summary) == len(titles)
                # write to csv
                csv_writer.writerow(scan_summary)
            # END-FOR
        # END-WITH

        return

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

    def _set_peak_info(self, exp_number, scan_number, peak_ws_name, md_ws_name):
        """ Add or modify a PeakInfo object for UB matrix calculation and etc.
        :param exp_number:
        :param scan_number:
        :param peak_ws_name:
        :param md_ws_name:
        :param peak_centre: calculated peak center
        :return: (boolean, PeakInfo/string)
        """
        # check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(peak_ws_name, str)
        assert isinstance(md_ws_name, str)

        # create a PeakInfo instance if it does not exist
        peak_info = PeakInfo(exp_number, scan_number, peak_ws_name)
        self._myPeakInfoDict[(exp_number, scan_number)] = peak_info

        # set the other information
        peak_info.set_data_ws_name(md_ws_name)
        peak_info.calculate_peak_center()

        return True, peak_info

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

    def survey(self, exp_number, start_scan, end_scan):
        """ Load all the SPICE ascii file to get the big picture such that
        * the strongest peaks and their HKL in order to make data reduction and analysis more convenient
        :param exp_number: experiment number
        :param start_scan:
        :param end_scan:
        :return: a list. first item is max_count
        """
        # Check
        assert isinstance(exp_number, int)
        if isinstance(start_scan, int) is False:
            start_scan = 1
        if isinstance(end_scan , int) is False:
            end_scan = MAX_SCAN_NUMBER

        # Output workspace
        scan_sum_list = list()

        # Download and
        for scan_number in xrange(start_scan, end_scan):
            # check whether file exists
            if self.does_file_exist(exp_number, scan_number) is False:
                # SPICE file does not exist in data directory. Download!
                # set up URL and target file name
                spice_file_url = get_spice_file_url(self._myServerURL, self._instrumentName, exp_number, scan_number)
                spice_file_name = get_spice_file_name(self._instrumentName, exp_number, scan_number)
                spice_file_name = os.path.join(self._dataDir, spice_file_name)

                # download file and load
                try:
                    api.DownloadFile(Address=spice_file_url, Filename=spice_file_name)
                except RuntimeError as download_error:
                    print 'Unable to download scan %d due to %s.' % (scan_number, str(download_error))
                    break
            else:
                spice_file_name = get_spice_file_name(self._instrumentName, exp_number, scan_number)
                spice_file_name = os.path.join(self._dataDir, spice_file_name)

            # Load SPICE file and retrieve information
            try:
                spice_table_ws_name = 'TempTable'
                api.LoadSpiceAscii(Filename=spice_file_name,
                                   OutputWorkspace=spice_table_ws_name,
                                   RunInfoWorkspace='TempInfo')
                spice_table_ws = AnalysisDataService.retrieve(spice_table_ws_name)
                num_rows = spice_table_ws.rowCount()
                col_name_list = spice_table_ws.getColumnNames()
                h_col_index = col_name_list.index('h')
                k_col_index = col_name_list.index('k')
                l_col_index = col_name_list.index('l')
                col_2theta_index = col_name_list.index('2theta')
                m1_col_index = col_name_list.index('m1')

                max_count = 0
                max_row = 0
                max_h = max_k = max_l = 0

                two_theta = m1 = -1

                for i_row in xrange(num_rows):
                    det_count = spice_table_ws.cell(i_row, 5)
                    if det_count > max_count:
                        max_count = det_count
                        max_row = i_row
                        max_h = spice_table_ws.cell(i_row, h_col_index)
                        max_k = spice_table_ws.cell(i_row, k_col_index)
                        max_l = spice_table_ws.cell(i_row, l_col_index)
                        two_theta = spice_table_ws.cell(i_row, col_2theta_index)
                        m1 = spice_table_ws.cell(i_row, m1_col_index)
                # END-FOR

                # calculate wavelength
                wavelength = get_hb3a_wavelength(m1)
                q_range = 4.*math.pi*math.sin(two_theta/180.*math.pi*0.5)/wavelength
                print '[DB-BAT] 2theta = %f, lambda = %f, Q = %f' % (two_theta, wavelength, q_range)

                # appending to list
                scan_sum_list.append([max_count, scan_number, max_row, max_h, max_k, max_l,
                                      q_range])

            except RuntimeError as e:
                print e
                return False, str(e)
            except ValueError as e:
                return False, 'Unable to locate column h, k, or l. See %s.' % str(e)

        # END-FOR (scan_number)

        self._scanSummaryList = scan_sum_list

        return True, scan_sum_list


def convert_spice_ub_to_mantid(spice_ub):
    """ Convert SPICE UB matrix to Mantid UB matrix
    :param spice_ub:
    :return: UB matrix in Mantid format
    """
    mantid_ub = numpy.ndarray((3, 3), 'float')
    # row 0
    for i in xrange(3):
        mantid_ub[0][i] = spice_ub[0][i]
    # row 1
    for i in xrange(3):
        mantid_ub[1][i] = spice_ub[2][i]
    # row 2
    for i in xrange(3):
        mantid_ub[2][i] = -1.*spice_ub[1][i]

    return mantid_ub
