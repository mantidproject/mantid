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
from __future__ import (absolute_import, division, print_function)
try:
    # python3
    from urllib.request import urlopen
    from urllib.error import HTTPError
    from urllib.error import URLError
except ImportError:
    from urllib2 import urlopen
    from urllib2 import HTTPError
    from urllib2 import URLError
from six.moves import range
import csv
import random
import os

from HFIR_4Circle_Reduction.fourcircle_utility import *
import HFIR_4Circle_Reduction.fourcircle_utility as fourcircle_utility
from HFIR_4Circle_Reduction.peakprocesshelper import PeakProcessRecord
from HFIR_4Circle_Reduction import fputility
from HFIR_4Circle_Reduction import project_manager
from HFIR_4Circle_Reduction import peak_integration_utility
from HFIR_4Circle_Reduction import absorption
from HFIR_4Circle_Reduction import process_mask

import mantid
import mantid.simpleapi as mantidsimple
from mantid.api import AnalysisDataService
from mantid.kernel import V3D


DebugMode = True

# DET_X_SIZE = 512
# DET_Y_SIZE = 512

MAX_SCAN_NUMBER = 100000


def check_str_type(variable, var_name):
    """

    :param variable:
    :param var_name:
    :return:
    """
    assert isinstance(var_name, str), 'Variable name {0} must be an integer but not a {1}' \
                                      ''.format(var_name, type(var_name))
    assert isinstance(variable, str), '{0} {1} must be an string but not a {2}' \
                                      ''.format(var_name, variable, type(variable))

    return


def check_int_type(variable, var_name):
    """
    check whether a variable is an integer
    :except AssertionError:
    :param variable:
    :param var_name:
    :return:
    """
    assert isinstance(var_name, str), 'Variable name {0} must be an integer but not a {1}' \
                                      ''.format(var_name, type(var_name))
    assert isinstance(variable, int), '{0} {1} must be an integer but not a {2}' \
                                      ''.format(var_name, variable, type(variable))

    return


class CWSCDReductionControl(object):
    """ Controlling class for reactor-based single crystal diffraction reduction
    """

    RESERVED_ROI_NAME = '__temp_roi__'

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
        self._preprocessedDir = None
        # dictionary for pre-processed scans.  key = scan number, value = dictionary for all kinds of information
        self._preprocessedInfoDict = None

        self._myServerURL = ''

        # Some set up
        self._expNumber = None

        # instrument default constants
        self._defaultDetectorSampleDistance = None
        # geometry of pixel
        self._defaultPixelSizeX = None
        self._defaultPixelSizeY = None
        # user-defined wave length
        self._userWavelengthDict = dict()
        # default peak center
        self._defaultDetectorCenter = None

        # Container for MDEventWorkspace for each Pt.
        self._myMDWsList = list()
        # Container for loaded workspaces
        self._mySpiceTableDict = {}
        # Container for loaded raw pt workspace
        self._myRawDataWSDict = dict()
        self._myRawDataMasked = dict()
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

        # About K-shift for output of integrated peak
        self._kVectorIndex = 1
        self._kShiftDict = dict()

        # A dictionary to manage all loaded and processed MDEventWorkspaces
        # self._expDataDict = {}
        self._detSampleDistanceDict = dict()
        self._detCenterDict = dict()

        # detector geometry: initialized to unphysical value
        self._detectorSize = [-1, -1]
        self._defaultPixelNumberX = None
        self._defaultPixelNumberY = None

        # reference workspace for LoadMask
        self._refWorkspaceForMask = None
        # Region of interest: key = (experiment, scan), value = RegionOfInterest instance
        self._roiDict = dict()

        # register startup
        mantid.UsageService.registerFeatureUsage("Interface", "4-Circle Reduction", False)

        # debug mode
        self._debugPrintMode = True

        return

    @property
    def pre_processed_dir(self):
        """
        get the pre-processed directory
        :return:
        """
        return self._preprocessedDir

    @pre_processed_dir.setter
    def pre_processed_dir(self, dir_name):
        """
        setting pre-processed directory
        :param dir_name:
        :return:
        """
        # check
        assert isinstance(dir_name, str) or dir_name is None, 'Directory {0} must be None or string.'.format(dir_name)

        if os.path.exists(dir_name) is False:
            raise RuntimeError('Pre-processed scans directory {0} does not exist!'.format(dir_name))

        # set
        self._preprocessedDir = dir_name

        # load pre-processed scans' record file if possible
        if self._expNumber is None:
            raise RuntimeError('Experiment number {0} must be set up before pre-processesd scan directory is set.')
        record_file_name = fourcircle_utility.pre_processed_record_file(self._expNumber, self._preprocessedDir)
        if os.path.exists(record_file_name):
            self._preprocessedInfoDict = fourcircle_utility.read_pre_process_record(record_file_name)

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

    def add_k_shift_vector(self, k_x, k_y, k_z):
        """
        Add a k-shift vector
        :param k_x:
        :param k_y:
        :param k_z:
        :return: k_index of the (k_x, k_y, k_z)
        """
        # check
        assert isinstance(k_x, float), 'Kx is wrong'
        assert isinstance(k_y, float), 'Ky is wrong'
        assert isinstance(k_z, float), 'Kz is wrong'

        k_shift_vector = (k_x, k_y, k_z)
        self._kShiftDict[self._kVectorIndex] = [k_shift_vector, []]

        # make progress
        return_k_index = self._kVectorIndex
        self._kVectorIndex += 1

        return return_k_index

    def apply_mask(self, exp_number, scan_number, pt_number, roi_name=None):
        """
        Apply mask on a Pt./run. by using a standard non-tag-based mask workspace's name
        Requirements:
        1. exp number, scan number, and pt number are integers
        2. mask workspace for this can must exist!
        Guarantees:
            the detector-xml data file is loaded to workspace2D with detectors being masked
        :param exp_number:
        :param scan_number:
        :param pt_number:
        :param roi_name: a string or a None
        :return:
        """
        # check
        assert isinstance(exp_number, int), 'Exp number {0} must be an integer but not a {1}' \
                                            ''.format(exp_number, type(exp_number))
        assert isinstance(scan_number, int), 'Scan number {0} must be an integer but not a {1}' \
                                             ''.format(scan_number, type(scan_number))
        assert isinstance(pt_number, int), 'Pt number {0} must be an integer but not a {1}' \
                                           ''.format(pt_number, type(pt_number))

        # get raw workspace for counts
        raw_pt_ws_name = get_raw_data_workspace_name(exp_number, scan_number, pt_number)

        # an existing mask
        if roi_name not in self._roiDict:
            raise RuntimeError('ROI {0} is not in mask workspace dictionary.  Current keys are {1}'
                               ''.format(roi_name, self._roiDict.keys()))
        mask_ws_name = self._roiDict[roi_name].mask_workspace
        if mask_ws_name is None:
            raise RuntimeError('ROI {0} has no mask workspace set'.format(roi_name))

        # mask detectors
        mantidsimple.MaskDetectors(Workspace=raw_pt_ws_name, MaskedWorkspace=mask_ws_name)
        # record
        self._myRawDataMasked[(exp_number, scan_number, pt_number)] = roi_name

        return

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
            assert status, 'Unable to get Pt numbers from scan %d.' % scan_number
        assert isinstance(pt_number_list, list) and len(pt_number_list) > 0

        # Check whether the MDEventWorkspace used to find peaks exists
        if self.has_merged_data(exp_number, scan_number, pt_number_list):
            pass
        else:
            raise RuntimeError('Data must be merged before')

        # Find peak in Q-space
        merged_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_number_list)
        peak_ws_name = get_peak_ws_name(exp_number, scan_number, pt_number_list)
        mantidsimple.FindPeaksMD(InputWorkspace=merged_ws_name,
                                 MaxPeaks=10,
                                 PeakDistanceThreshold=5.,
                                 DensityThresholdFactor=0.1,
                                 OutputWorkspace=peak_ws_name)
        assert AnalysisDataService.doesExist(peak_ws_name), 'PeaksWorkspace {0} does not exist in ADS.' \
                                                            ''.format(peak_ws_name)

        # add peak to UB matrix workspace to manager
        self._set_peak_info(exp_number, scan_number, peak_ws_name, merged_ws_name)

        # add the merged workspace to list to manage
        self._add_merged_ws(exp_number, scan_number, pt_number_list)

        peak_center = self._myPeakInfoDict[(exp_number, scan_number)].get_peak_centre()

        return True, peak_center

    @staticmethod
    def find_detector_size(exp_directory, exp_number):
        """
        find detector size from experiment directory
        :param exp_directory:
        :param exp_number
        :return:
        """
        # guess the file name
        first_xm_file = os.path.join(exp_directory, 'HB3A_Exp{0}_Scan0001_00001.xml'.format(exp_number))
        if os.path.exists(first_xm_file):
            file_size = os.path.getsize(first_xm_file)
            if file_size < 136132 * 2:
                det_size = 256, 256
            elif file_size < 529887 * 2:
                det_size = 512, 512
            else:
                raise RuntimeError('File size is over {0}.  It is not supported.')

            return True, det_size

        return False, 'Unable to find first Pt file {0}'.format(first_xm_file)

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
            if isinstance(peak_info, PeakProcessRecord) is False:
                raise NotImplementedError('Input PeakList is of type %s.' % str(type(peak_info_list[0])))
            assert isinstance(peak_info, PeakProcessRecord)

        # Construct a new peak workspace by combining all single peak
        ub_peak_ws_name = 'Temp_UB_Peak'
        self._build_peaks_workspace(peak_info_list, ub_peak_ws_name,
                                    index_from_spice=True, hkl_to_int=True)

        # Calculate UB matrix
        try:
            mantidsimple.CalculateUMatrix(PeaksWorkspace=ub_peak_ws_name,
                                          a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma)
        except ValueError as val_err:
            return False, str(val_err)

        ub_peak_ws = AnalysisDataService.retrieve(ub_peak_ws_name)
        ub_matrix = ub_peak_ws.sample().getOrientedLattice().getUB()

        self._myLastPeakUB = ub_peak_ws

        return True, ub_matrix

    def does_raw_loaded(self, exp_no, scan_no, pt_no, roi_name):
        """
        Check whether the raw Workspace2D for a Pt. exists
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :param roi_name:
        :return:
        """
        # check input
        check_int_type(exp_no, 'Experiment number')
        check_int_type(scan_no, 'Scan number')
        check_int_type(pt_no, 'Pt number')

        loaded = (exp_no, scan_no, pt_no) in self._myRawDataWSDict
        if loaded:
            curr_roi = self._myRawDataMasked[(exp_no, scan_no, pt_no)]
            if roi_name != curr_roi:
                loaded = False

        return loaded

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
        :param over_write:
        :return: 2-tuple: status (successful or failed), string (file name or error message
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
            mantidsimple.DownloadFile(Address=file_url, Filename=file_name)
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
            mantidsimple.DownloadFile(Address=det_file_url,
                                      Filename=local_xml_file_name)
        except RuntimeError as run_err:
            return False, 'Unable to download Detector XML file %s from %s ' \
                          'due to %s.' % (local_xml_file_name, det_file_url, str(run_err))

        # Check file exist?
        if os.path.exists(local_xml_file_name) is False:
            return False, "Unable to locate downloaded file %s." % local_xml_file_name

        # NEXT ISSUE - This is a temporary fix for unsupported strings in XML
        os.system("sed -i -e 's/0<x<1/0 x 1/g' %s" % local_xml_file_name)

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
                # spice_table = self._mySpiceTableDict[(self._expNumber, scan_no)]
                spice_table = self._get_spice_workspace(self._expNumber, scan_no)
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

    def check_generate_mask_workspace(self, exp_number, scan_number, mask_tag, check_throw):
        """
        Check whether a MaskWorkspace exists according to the tag
        If it does not, then generate one according to the tag

        A MaskWorkspace's name is exactly the same as the tag of the mask specified by user in
        reduction GUI.

        :param exp_number: must be integer if not retrieve mask workspace
        :param scan_number: must be integer if not retrieve mask workspace
        :param mask_tag: string as the tag of the mask.
        :param check_throw
        :return:
        """
        # Check
        assert isinstance(mask_tag, str), 'Mask tag {0} ({1}) must be a string.'.format(mask_tag, type(mask_tag))

        # MaskWorkspace's name is same as mask's tag
        mask_ws_name = mask_tag

        if AnalysisDataService.doesExist(mask_ws_name) is False:
            # if the workspace does not exist, create a new mask workspace
            if exp_number is None:
                raise RuntimeError('Experiment number is not given with assumption that mask tag {0} shall '
                                   'be a workspace.'.format(mask_tag))

            # check for experiment and scan number
            assert isinstance(exp_number, int), 'Experiment number {0} must be an integer but not a {1}.' \
                                                ''.format(exp_number, type(exp_number))
            assert isinstance(scan_number, int), 'Scan number {0} ({1}) must be an integer.' \
                                                 ''.format(scan_number, type(scan_number))
            if mask_tag not in self._roiDict:
                raise RuntimeError('Mask tag |{0}| does not exist in ROI dictionary.'.format(mask_tag))

            region_of_interest = self._roiDict[mask_tag]
            ll = region_of_interest[0]
            ur = region_of_interest[1]
            self.generate_mask_workspace(exp_number, scan_number, ll, ur, mask_ws_name)

        if check_throw:
            assert AnalysisDataService.doesExist(mask_ws_name), 'MaskWorkspace {0} does not exist.'.format(mask_ws_name)

        return mask_ws_name

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
            try:
                file_name = os.path.join(self._dataDir, spice_file_name)
            except AttributeError:
                raise AttributeError('Unable to create SPICE file name from directory %s and file name %s.'
                                     '' % (self._dataDir, spice_file_name))
        else:
            # pt number given, then check
            xml_file_name = get_det_xml_file_name(self._instrumentName, exp_number, scan_number,
                                                  pt_number)
            file_name = os.path.join(self._dataDir, xml_file_name)
        # END-IF

        return os.path.exists(file_name)

    @staticmethod
    def estimate_background(pt_intensity_dict, bg_pt_list):
        """
        Estimate background value by average the integrated counts of some Pt.
        :param pt_intensity_dict:
        :param bg_pt_list: list of Pt. that are used to calculate background
        :return:
        """
        # Check
        assert isinstance(pt_intensity_dict, dict)
        assert isinstance(bg_pt_list, list) and len(bg_pt_list) > 0

        # Sum over all Pt.
        bg_sum = 0.
        for bg_pt in bg_pt_list:
            assert bg_pt in pt_intensity_dict, 'Pt. %d is not calculated.' % bg_pt
            bg_sum += pt_intensity_dict[bg_pt]

        avg_bg = float(bg_sum) / len(bg_pt_list)

        return avg_bg

    def get_ub_matrix(self, exp_number):
        """ Get UB matrix assigned to an experiment
        :param exp_number:
        :return:
        """
        # check
        assert isinstance(exp_number, int), 'Experiment number must be an integer but not %s.' % str(type(exp_number))
        if exp_number not in self._myUBMatrixDict:
            err_msg = 'Experiment number %d has no UB matrix set up. Here ' \
                      'are list of experiments that have UB matrix set up: %s.' \
                      '' % (exp_number, str(self._myUBMatrixDict.keys()))
            raise KeyError(err_msg)

        return self._myUBMatrixDict[exp_number]

    def get_calibrated_wave_length(self, exp_number):
        """ Get the user specified (i.e., calibrated) wave length for a specific experiment
        :param exp_number:
        :return:
        """
        # check inputs
        assert isinstance(exp_number, int), 'Experiment numbe {0} must be an integer but not a {1}' \
                                            ''.format(exp_number, type(exp_number))

        if exp_number not in self._userWavelengthDict:
            return None

        return self._userWavelengthDict[exp_number]

    def get_wave_length(self, exp_number, scan_number_list):
        """
        Get the wavelength.
        Exception: RuntimeError if there are more than 1 wavelength found with all given scan numbers
        :param exp_number:
        :param scan_number_list:
        :return:
        """
        # check whether there is use wave length
        if exp_number in self._userWavelengthDict:
            return self._userWavelengthDict[exp_number]

        # get the SPICE workspace
        wave_length_set = set()

        # go through all the SPICE table workspace
        for scan_number in scan_number_list:
            spice_table_name = get_spice_table_name(exp_number, scan_number)
            curr_wl = get_wave_length(spice_table_name)
            wave_length_set.add(curr_wl)
        # END-FOR

        if len(wave_length_set) > 1:
            raise RuntimeError('There are more than 1 (%s) wave length found in scans.' % str(wave_length_set))

        return wave_length_set.pop()

    @staticmethod
    def get_motor_step(exp_number, scan_number):
        """ For omega/phi scan, get the average step of the motor
        :param exp_number:
        :param scan_number:
        :return:
        """
        # check
        assert isinstance(exp_number, int), 'Experiment number {0} must be an integer but not a {1}.' \
                                            ''.format(exp_number, type(scan_number))
        assert isinstance(scan_number, int), 'Scan number {0} must be an integer but not a {1}.' \
                                             ''.format(scan_number, type(scan_number))

        # get SPICE table
        spice_table_name = get_spice_table_name(exp_number, scan_number)
        spice_table = AnalysisDataService.retrieve(spice_table_name)

        if spice_table.rowCount() == 0:
            raise RuntimeError('Spice table %s is empty.')
        elif spice_table.rowCount() == 0:
            raise RuntimeError('Only 1 row in Spice table %s. All motors are stationary.' % spice_table)

        # get the motors values
        omega_vec = get_log_data(spice_table, 'omega')
        omega_dev, omega_step, omega_step_dev = get_step_motor_parameters(omega_vec)
        omega_tup = omega_dev, ('omega', omega_step, omega_step_dev)

        chi_vec = get_log_data(spice_table, 'chi')
        chi_dev, chi_step, chi_step_dev = get_step_motor_parameters(chi_vec)
        chi_tup = chi_dev, ('chi', chi_step, chi_step_dev)

        phi_vec = get_log_data(spice_table, 'phi')
        phi_dev, phi_step, phi_step_dev = get_step_motor_parameters(phi_vec)
        phi_tup = phi_dev, ('phi', phi_step, phi_step_dev)

        # find the one that moves
        move_tup = max([omega_tup, chi_tup, phi_tup])

        return move_tup[1]

    def export_to_fullprof(self, exp_number, scan_number_list, user_header,
                           export_absorption, fullprof_file_name, high_precision):
        """
        Export peak intensities to Fullprof data file
        :param exp_number:
        :param scan_number_list:
        :param user_header:
        :param export_absorption:
        :param fullprof_file_name:
        :param high_precision: flag to write peak intensity as f18.5 if true; otherwise, output as f8.2
        :return: 2-tuples. status and return object ((mixed) file content or error message)
        """
        # check
        assert isinstance(exp_number, int), 'Experiment number must be an integer.'
        assert isinstance(scan_number_list, list), 'Scan number list must be a list but not %s.' \
                                                   '' % str(type(scan_number_list))
        assert len(scan_number_list) > 0, 'Scan number list must larger than 0, but ' \
                                          'now %d. ' % len(scan_number_list)

        # get wave-length
        try:
            exp_wave_length = self.get_wave_length(exp_number, scan_number_list)
        except RuntimeError as error:
            return False, 'RuntimeError: %s.' % str(error)

        # get the information whether there is any k-shift vector specified by user

        # form k-shift and peak intensity information
        scan_kindex_dict = dict()
        k_shift_dict = dict()
        for k_index in self._kShiftDict.keys():
            tup_value = self._kShiftDict[k_index]
            k_shift_dict[k_index] = tup_value[0]
            for scan_number in tup_value[1]:
                scan_kindex_dict[scan_number] = k_index
            # END-FOR (scan_number)
        # END-FOR (_kShiftDict)

        error_message = 'Number of scans with k-shift must either be 0 (no shift at all) or ' \
                        'equal to or larger than the number scans to export.'
        assert len(scan_kindex_dict) == 0 or len(scan_kindex_dict) >= len(scan_number_list), error_message

        # form peaks
        no_shift = len(scan_kindex_dict) == 0

        # get ub matrix
        ub_matrix = self.get_ub_matrix(exp_number)

        mixed_content = None
        for algorithm_type in ['simple', 'mixed', 'gauss']:
            # set list of peaks for exporting
            peaks = list()
            for scan_number in scan_number_list:
                peak_dict = dict()
                try:
                    peak_dict['hkl'] = self._myPeakInfoDict[(exp_number, scan_number)].get_hkl(user_hkl=True)
                except RuntimeError as run_err:
                    return False, str('Peak index error: %s.' % run_err)

                intensity, std_dev = self._myPeakInfoDict[(exp_number, scan_number)].get_intensity(
                    algorithm_type, lorentz_corrected=True)

                if intensity < std_dev:
                    # error is huge, very likely bad gaussian fit
                    if self._debugPrintMode:
                        print('[INFO] Integration Type {0}: Scan {1} Intensity {2} < Std Dev {2} '
                              'Excluded from exporting.'.format(algorithm_type, scan_number, intensity, std_dev))
                    continue
                # END-IF

                peak_dict['intensity'] = intensity
                peak_dict['sigma'] = std_dev
                if no_shift:
                    peak_dict['kindex'] = 0
                else:
                    peak_dict['kindex'] = scan_kindex_dict[scan_number]

                if export_absorption:
                    # calculate absorption correction
                    spice_ub = convert_mantid_ub_to_spice(ub_matrix)
                    up_cart, us_cart = absorption.calculate_absorption_correction_2(
                        exp_number, scan_number, spice_ub)
                    peak_dict['up'] = up_cart
                    peak_dict['us'] = us_cart

                # append peak (in dict) to peaks
                peaks.append(peak_dict)
            # END-FOR (scan_number)

            # get file name for this type
            this_file_name = fullprof_file_name.split('.')[0] + '_' + algorithm_type + '.dat'

            try:
                file_content = fputility.write_scd_fullprof_kvector(
                    user_header=user_header, wave_length=exp_wave_length,
                    k_vector_dict=k_shift_dict, peak_dict_list=peaks,
                    fp_file_name=this_file_name, with_absorption=export_absorption,
                    high_precision=high_precision)
                if algorithm_type == 'mixed':
                    mixed_content = file_content
            except AssertionError as error:
                return False, 'AssertionError: %s.' % str(error)
            except RuntimeError as error:
                return False, 'RuntimeError: %s.' % str(error)

            continue
        # END-FOR

        return True, mixed_content

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

        mantidsimple.ConvertCWSDMDtoHKL(InputWorkspace=md_ws_name,
                                        UBMatrix='1., 0., 0., 0., 1., 0., 0., 0., 1',
                                        OutputWorkspace=temp_out_ws,
                                        QSampleFileName=out_file_name)
        mantidsimple.DeleteWorkspace(Workspace=temp_out_ws)

        return out_file_name

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
        if 'Pt.' not in col_name_list:
            return False, 'No column with name Pt. can be found in SPICE table.'

        i_pt = col_name_list.index('Pt.')
        assert 0 <= i_pt < len(col_name_list), 'Impossible to have assertion error!'

        pt_number_list = []
        num_rows = table_ws.rowCount()
        for i in range(num_rows):
            pt_number = table_ws.cell(i, i_pt)
            pt_number_list.append(pt_number)

        return True, pt_number_list

    def get_raw_detector_counts(self, exp_no, scan_no, pt_no):
        """
        Get counts on raw detector
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :return: boolean, 2D numpy data
        """
        # Get workspace (in memory or loading)
        raw_ws = self.get_raw_data_workspace(exp_no, scan_no, pt_no)
        if raw_ws is None:
            return False, 'Raw data for Exp %d Scan %d Pt %d is not loaded.' % (exp_no, scan_no, pt_no)

        # Convert to numpy array
        det_shape = (self._detectorSize[0], self._detectorSize[1])
        array2d = numpy.ndarray(shape=det_shape, dtype='float')
        for i in range(det_shape[0]):
            for j in range(det_shape[1]):
                array2d[i][j] = raw_ws.readY(j * det_shape[0] + i)[0]

        # Flip the 2D array to look detector from sample
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

    def get_region_of_interest(self, roi_name):
        """ Get region of interest
        :param roi_name: name of the ROI
        :return: region of interest
        """
        assert isinstance(roi_name, str),\
            'ROI name {0} must be a string or None but not a {1}.'.format(roi_name, type(roi_name))

        if roi_name not in self._roiDict:
            # ROI: not saved
            raise RuntimeError('ROI not here blabla')

        # check...
        lower_left_corner = self._roiDict[roi_name].lower_left_corner
        upper_right_corner = self._roiDict[roi_name].upper_right_corner

        if lower_left_corner is None or upper_right_corner is None:
            raise RuntimeError('ROI positions not set')

        return lower_left_corner, upper_right_corner

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
        :param pt_number_list:
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
        :return: PeakInfo instance or None
        """
        # Check for type
        assert isinstance(exp_number, int), 'Experiment %s must be an integer but not of type %s.' \
                                            '' % (str(exp_number), type(exp_number))
        assert isinstance(scan_number, int), 'Scan number %s must be an integer but not of type %s.' \
                                             '' % (str(scan_number), type(scan_number))
        assert isinstance(pt_number, int) or pt_number is None, 'Pt number %s must be an integer or None, but ' \
                                                                'it is of type %s now.' % (str(pt_number),
                                                                                           type(pt_number))

        # construct key
        if pt_number is None:
            p_key = (exp_number, scan_number)
        else:
            p_key = (exp_number, scan_number, pt_number)

        # Check for existence
        if p_key in self._myPeakInfoDict:
            ret_value = self._myPeakInfoDict[p_key]
        else:
            ret_value = None

        return ret_value

    def get_peaks_integrated_intensities(self, exp_number, scan_number, pt_list):
        """
        Get the integrated intensities for a peak
        Requirements:
        1. the Pts in the scan must have been merged and intensity is calculated.
        2. experiment number and scan number must be integers
        Guarantees: get the x-y plot for intensities of all Pts. X is pt number, Y is for intensity
        :param exp_number:
        :param scan_number:
        :param pt_list:
        :return:
        """
        # check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(pt_list, list) or pt_list is None

        # deal with pt list if it is None
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
        for index in range(array_size):
            peak_i = int_peak_ws.getPeak(index)
            # Note: run number in merged workspace is a combination of pt number and scan number
            #       so it should have 1000 divided for the correct pt number
            pt_number = peak_i.getRunNumber() % 1000
            intensity = peak_i.getIntensity()
            vec_x[index] = pt_number
            vec_y[index] = intensity
        # END-FOR

        return vec_x, vec_y

    def generate_mask_workspace(self, exp_number, scan_number, roi_start, roi_end, mask_tag=None):
        """ Generate a mask workspace
        :param exp_number:
        :param scan_number:
        :param roi_start:
        :param roi_end:
        :return:
        """
        # assert ...
        assert isinstance(exp_number, int), 'Experiment number {0} ({1}) must be an integer.' \
                                            ''.format(exp_number, type(exp_number))
        assert isinstance(scan_number, int), 'Scan number {0} ({1}) must be an integer.' \
                                             ''.format(scan_number, type(scan_number))

        # create an xml file
        mask_file_name = get_mask_xml_temp(self._workDir, exp_number, scan_number)
        generate_mask_file(file_path=mask_file_name,
                           ll_corner=roi_start,
                           ur_corner=roi_end)

        # check reference workspace for mask workspace
        if self._refWorkspaceForMask is None:
            return False, 'There is no reference workspace. Plot a Pt. first!'
        elif AnalysisDataService.doesExist(self._refWorkspaceForMask) is False:
            return False, 'Previous reference workspace has been deleted. Plot a Pt. first'

        # get the name of the mask workspace to be loaded to
        if mask_tag is None:
            # use default name
            mask_ws_name = get_mask_ws_name(exp_number, scan_number)
        else:
            # use given name
            mask_ws_name = str(mask_tag)

        # load the mask workspace
        mantidsimple.LoadMask(Instrument='HB3A',
                              InputFile=mask_file_name,
                              OutputWorkspace=mask_ws_name,
                              RefWorkspace=self._refWorkspaceForMask)
        mantidsimple.InvertMask(InputWorkspace=mask_ws_name,
                                OutputWorkspace=mask_ws_name)

        # register
        self._roiDict[mask_tag].set_mask_workspace_name(mask_ws_name)

        return True, mask_tag

    def get_working_directory(self):
        """
        get working directory
        :return:
        """
        return self._workDir

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
                exp_number, scan_number = key
                spice_table_name = get_spice_table_name(exp_number, scan_number)
                ws_names_str += '%s,' % spice_table_name  # self._mySpiceTableDict[key].name()

        # Check
        if len(ws_names_str) == 0:
            return False, 'No workspace is found for experiment %d.' % exp_number

        # Remove last ','
        ws_names_str = ws_names_str[:-1]

        # Group
        mantidsimple.GroupWorkspaces(InputWorkspaces=ws_names_str,
                                     OutputWorkspace=group_name)

        return

    def has_integrated_peak(self, exp_number, scan_number, masked, pt_list=None,
                            normalized_by_monitor=False, normalized_by_time=False):
        """ Check whether the peak is integrated as designated
        :param exp_number:
        :param scan_number:
        :param masked:
        :param pt_list:
        :param normalized_by_monitor:
        :param normalized_by_time:
        :return:
        """
        # check requirements
        assert isinstance(exp_number,int), 'Experiment number must be an integer but not %s.' \
                                           '' % str(type(exp_number))
        assert isinstance(scan_number, int), 'Scan number must be an integer but not %s.' \
                                             '' % str(type(scan_number))

        # get default Pt list if required
        if pt_list is None:
            status, ret_obj = self.get_pt_numbers(exp_number, scan_number)
            if status is False:
                raise RuntimeError(ret_obj)
            pt_list = ret_obj
        # END-IF
        assert isinstance(pt_list, list) and len(pt_list) > 0

        peak_ws_name = get_integrated_peak_ws_name(exp_number, scan_number, pt_list, masked,
                                                   normalized_by_monitor, normalized_by_time)

        return AnalysisDataService.doesExist(peak_ws_name)

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

    def has_roi_generated(self, roi_name):
        """
        check whether a MaskWorkspace has been generated for an ROI
        :param roi_name:
        :return:
        """
        # check input
        assert isinstance(roi_name, str), 'ROI name {0} must be a string but not a {1}'.format(roi_name, type(roi_name))

        # check whether it is in the dicationary and has a mask workspace set
        has = True
        if roi_name not in self._roiDict:
            has = False
        elif self._roiDict[roi_name].mask_workspace is None:
            has = False

        return has

    def index_peak(self, ub_matrix, scan_number, allow_magnetic=False):
        """ Index peaks in a Pt. by create a temporary PeaksWorkspace which contains only 1 peak
        :param ub_matrix: numpy.ndarray (3, 3)
        :param scan_number:
        :param allow_magnetic: flag to allow magnetic reflections
        :return: boolean, object (list of HKL or error message)
        """
        # Check
        assert isinstance(ub_matrix, numpy.ndarray), 'UB matrix must be an ndarray'
        assert ub_matrix.shape == (3, 3), 'UB matrix must be a 3x3 matrix.'
        assert isinstance(scan_number, int), 'Scan number must be in integer.'

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
        mantidsimple.CreatePeaksWorkspace(NumberOfPeaks=0, OutputWorkspace=temp_index_ws_name)
        temp_index_ws = AnalysisDataService.retrieve(temp_index_ws_name)

        temp_index_ws.addPeak(peak_ws.getPeak(0))
        virtual_peak = temp_index_ws.getPeak(0)
        virtual_peak.setHKL(0, 0, 0)
        virtual_peak.setQSampleFrame(peak_info.get_peak_centre_v3d())

        # Set UB matrix to the peak workspace
        ub_1d = ub_matrix.reshape(9,)

        # Set UB
        mantidsimple.SetUB(Workspace=temp_index_ws_name, UB=ub_1d)

        # Note: IndexPeaks and CalculatePeaksHKL do the same job
        #       while IndexPeaks has more control on the output
        if allow_magnetic:
            tol = 0.5
        else:
            tol = 0.3

        num_peak_index, error = mantidsimple.IndexPeaks(PeaksWorkspace=temp_index_ws_name,
                                                        Tolerance=tol,
                                                        RoundHKLs=False)
        temp_index_ws = AnalysisDataService.retrieve(temp_index_ws_name)

        if num_peak_index == 0:
            return False, 'No peak can be indexed: {0}.'.format(error)
        elif num_peak_index > 1:
            raise RuntimeError('Case for PeaksWorkspace containing more than 1 peak is not '
                               'considered. Contact developer for this issue.')
        else:
            hkl_v3d = temp_index_ws.getPeak(0).getHKL()
            hkl = numpy.array([hkl_v3d.X(), hkl_v3d.Y(), hkl_v3d.Z()])

        # set HKL to peak
        peak_info.set_hkl(hkl[0], hkl[1], hkl[2])

        # delete temporary workspace
        mantidsimple.DeleteWorkspace(Workspace=temp_index_ws_name)

        return True, (hkl, error)

    def integrate_scan_peak(self, exp_number, scan_number, peak_centre, mask_name, normalization,
                            scale_factor, background_pt_tuple):
        """
        new way to integrate a peak in a scan
        Note: it is going to replace "integrate_scan_peaks()"
        :param exp_number:
        :param scan_number:
        :param peak_centre:
        :param mask_name:
        :param normalization:
        :param scale_factor:
        :param background_pt_tuple:
        :return:
        """
        # check inputs
        assert isinstance(exp_number, int), 'Experiment number {0} must be an integer but not a {1}.' \
                                            ''.format(exp_number, type(exp_number))
        assert isinstance(scan_number, int), 'Scan number {0} must be an integer but not a {1}.' \
                                             ''.format(scan_number, type(scan_number))
        assert isinstance(mask_name, str), 'Mask name {0} must be a string but not a {1}.' \
                                           ''.format(mask_name, type(mask_name))
        assert isinstance(normalization, str), 'Normalization type {0} must be a string but not a {1}.' \
                                               ''.format(normalization, type(normalization))
        assert isinstance(scale_factor, float) or isinstance(scale_factor, int),\
            'Scale factor {0} must be a float or integer but not a {1}.'.format(scale_factor, type(scale_factor))
        assert len(peak_centre) == 3, 'Peak center {0} must have 3 elements for (Qx, Qy, Qz).'.format(peak_centre)
        assert len(background_pt_tuple) == 2, 'Background tuple {0} must be of length 2.'.format(background_pt_tuple)

        # get input MDEventWorkspace name for merged scan
        status, ret_obj = self.get_pt_numbers(exp_number, scan_number)
        if status:
            pt_list = ret_obj
        else:
            raise RuntimeError('Unable to get Pt. list from Exp {0} Scan {1} due to {2}'
                               ''.format(exp_number,scan_number, ret_obj))
        md_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_list)

        # get the TableWorkspace name for Spice
        spice_table_ws = get_spice_table_name(exp_number, scan_number)

        # output PeaksWorkspace name and MaskWorkspace
        if len(mask_name) > 0:
            mask_ws_name = self.check_generate_mask_workspace(exp_number, scan_number, mask_name, check_throw=True)
        else:
            mask_ws_name = None
        peak_ws_name = get_integrated_peak_ws_name(exp_number, scan_number, pt_list, mask_name)

        # peak center
        int_peak_dict = peak_integration_utility.integrate_peak_full_version(scan_md_ws_name=md_ws_name,
                                                                             spice_table_name=spice_table_ws,
                                                                             output_peak_ws_name=peak_ws_name,
                                                                             peak_center=peak_centre,
                                                                             mask_workspace_name=mask_ws_name,
                                                                             norm_type=normalization,
                                                                             intensity_scale_factor=scale_factor,
                                                                             background_pt_tuple=background_pt_tuple)

        return int_peak_dict

    def integrate_scan_peaks(self, exp, scan, peak_radius, peak_centre,
                             merge_peaks=True, use_mask=False,
                             normalization='', mask_ws_name=None,
                             scale_factor=1.00, background_pt_tuple=None):
        """
        :param exp:
        :param scan:
        :param peak_radius:
        :param peak_centre:  a float radius or None for not using
        :param merge_peaks: If selected, merged all the Pts can return 1 integrated peak's value;
                            otherwise, integrate peak for each Pt.
        :param use_mask:
        :param normalization: normalization set up (by time or ...)
        :param mask_ws_name: mask workspace name or None
        :param scale_factor: integrated peaks' scaling factor
        :return: dictionary of Pts.
        """
        # check
        assert isinstance(exp, int)
        assert isinstance(scan, int)
        assert isinstance(peak_radius, float) or peak_radius is None
        assert len(peak_centre) == 3
        assert isinstance(merge_peaks, bool)

        peak_int_dict = self.integrate_scan_peak(exp_number=exp, scan_number=scan, peak_centre=peak_centre,
                                                 mask_name=mask_ws_name, normalization=normalization,
                                                 scale_factor=scale_factor, background_pt_tuple=background_pt_tuple)

        #
        # store the data into peak info
        if (exp, scan) not in self._myPeakInfoDict:
            raise RuntimeError('Exp %d Scan %d is not recorded in PeakInfo-Dict' % (exp, scan))
        self._myPeakInfoDict[(exp, scan)].set_pt_intensity(peak_int_dict)

        return True, peak_int_dict

    @staticmethod
    def gauss_correction_peak_intensity(pt_dict):
        """
        fit a peak along Pt. with Gaussian and thus calculate background automatically
        :param pt_dict:
        :return: 3-tuple (intensity, background and information string)
        """
        # check
        assert isinstance(pt_dict, dict), 'Input must be a dictionary but not {0}'.format(type(pt_dict))

        # convert to vector
        tup_list = list()
        for pt in pt_dict.keys():
            tup_list.append((pt, pt_dict[pt]))
        tup_list.sort()
        list_x = list()
        list_y = list()
        for tup in tup_list:
            list_x.append(float(tup[0]))
            list_y.append(float(tup[1]))
        vec_x = numpy.array(list_x)
        vec_y = numpy.array(list_y)
        vec_e = numpy.sqrt(vec_y)

        # do fit
        error, gauss_params, model_vec_y = peak_integration_utility.fit_gaussian_linear_background(vec_x, vec_y, vec_e)
        x0, gauss_sigma, gauss_a, gauss_bkgd = gauss_params
        if not (0 < x0 < vec_x[-1]):
            raise RuntimeError('Fitted center of the peak {0} is out of range, which is not correct'.format(x0))
        if gauss_a <= 0.:
            raise RuntimeError('Fitted peak height {0} is negative!'.format(gauss_a))

        # calculate the peak intensity
        peak_intensity = peak_integration_utility.calculate_peak_intensity_gauss(gauss_a, gauss_sigma)

        # information
        info_str = 'Fit error = {0}: a = {1}, x0 = {2}, sigma = {3}, b = {4}'.format(error, gauss_a, x0, gauss_sigma,
                                                                                     gauss_bkgd)

        return peak_intensity, gauss_bkgd, info_str

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
        :param exp_no:
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

        # load the SPICE table data if the target workspace does not exist
        if not AnalysisDataService.doesExist(out_ws_name):
            # Form standard name for a SPICE file if name is not given
            if spice_file_name is None:
                spice_file_name = os.path.join(self._dataDir,
                                               get_spice_file_name(self._instrumentName, exp_no, scan_no))

            # Download SPICE file if necessary
            if os.path.exists(spice_file_name) is False:
                file_available, download_result = self.download_spice_file(exp_no, scan_no, over_write=True)
            else:
                file_available = True
                download_result = None

            if not file_available:
                raise IOError('SPICE file for Exp {0} Scan {1} cannot be found at {2} or downloaded ({3})'
                              ''.format(exp_no, scan_no, spice_file_name, download_result))

            try:
                spice_table_ws, info_matrix_ws = mantidsimple.LoadSpiceAscii(Filename=spice_file_name,
                                                                             OutputWorkspace=out_ws_name,
                                                                             RunInfoWorkspace='TempInfo')
                mantidsimple.DeleteWorkspace(Workspace=info_matrix_ws)
            except RuntimeError as run_err:
                return False, 'Unable to load SPICE data %s due to %s' % (spice_file_name, str(run_err))
        else:
            spice_table_ws = AnalysisDataService.retrieve(out_ws_name)
        # END-IF

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
        assert isinstance(spice_table_ws, mantid.dataobjects.TableWorkspace), 'SPICE table workspace must be a ' \
                                                                              'TableWorkspace but not %s.' \
                                                                              '' % type(spice_table_ws)
        spice_table_name = spice_table_ws.name()

        # load SPICE Pt.  detector file
        pt_ws_name = get_raw_data_workspace_name(exp_no, scan_no, pt_no)
        try:
            mantidsimple.LoadSpiceXML2DDet(Filename=xml_file_name,
                                           OutputWorkspace=pt_ws_name,
                                           SpiceTableWorkspace=spice_table_name,
                                           PtNumber=pt_no)
            if self._refWorkspaceForMask is None or AnalysisDataService.doesExist(pt_ws_name) is False:
                self._refWorkspaceForMask = pt_ws_name
        except RuntimeError as run_err:
            return False, str(run_err)

        # Add data storage
        assert AnalysisDataService.doesExist(pt_ws_name), 'Unable to locate workspace {0}.'.format(pt_ws_name)
        raw_matrix_ws = AnalysisDataService.retrieve(pt_ws_name)
        self._add_raw_workspace(exp_no, scan_no, pt_no, raw_matrix_ws)
        # clear the mask/ROI information
        self._myRawDataMasked[(exp_no, scan_no, pt_no)] = None

        return True, pt_ws_name

    @staticmethod
    def merge_multiple_scans(scan_md_ws_list, scan_peak_centre_list, merged_ws_name):
        """
        Merge multiple scans
        :param scan_md_ws_list: List of MDWorkspace, each of which is for a scan.
        :param scan_peak_centre_list: list of peak centres for all scans.
        :param merged_ws_name:
        :return:
        """
        # check validity
        assert isinstance(scan_md_ws_list, list), 'Scan MDWorkspace name list cannot be of type %s.' \
                                                  '' % type(scan_md_ws_list)
        assert isinstance(scan_peak_centre_list, list), 'Scan peak center list cannot be of type %s.' \
                                                        '' % type(scan_peak_centre_list)
        assert len(scan_md_ws_list) >= 2 and len(scan_md_ws_list) == len(scan_peak_centre_list),\
            'Number of MDWorkspace %d and peak centers %d are not correct.' % (len(scan_md_ws_list),
                                                                               len(scan_peak_centre_list))
        assert isinstance(merged_ws_name, str), 'Target MDWorkspace name for merged scans %s (%s) must ' \
                                                'be a string.' % (str(merged_ws_name), type(merged_ws_name))

        # get the workspace
        ws_name_list = ''
        for i_ws, ws_name in enumerate(scan_md_ws_list):
            # build the input MDWorkspace list
            if i_ws != 0:
                ws_name_list += ', '
            ws_name_list += ws_name

            # rebin the MDEventWorkspace to make all MDEventWorkspace have same MDGrid
            md_ws = AnalysisDataService.retrieve(ws_name)
            frame = md_ws.getDimension(0).getMDFrame().name()

            if frame == 'HKL':
                mantidsimple.SliceMD(InputWorkspace=ws_name,
                                     AlignedDim0='H,-10,10,1',
                                     AlignedDim1='K,-10,10,1',
                                     AlignedDim2='L,-10,10,1',
                                     OutputWorkspace=ws_name)
            else:
                mantidsimple.SliceMD(InputWorkspace=ws_name,
                                     AlignedDim0='Q_sample_x,-10,10,1',
                                     AlignedDim1='Q_sample_y,-10,10,1',
                                     AlignedDim2='Q_sample_z,-10,10,1',
                                     OutputWorkspace=ws_name)
        # END-FOR

        # merge
        mantidsimple.MergeMD(InputWorkspaces=ws_name_list,
                             OutputWorkspace=merged_ws_name)

        # get the unit of MD workspace
        md_ws = AnalysisDataService.retrieve(scan_md_ws_list[0])
        frame = md_ws.getDimension(0).getMDFrame().name()

        # calculating the new binning boundaries. It will not affect the merge result. but only for user's reference.
        axis0_range = list()
        axis1_range = list()
        axis2_range = list()
        for i_peak, peak in enumerate(scan_peak_centre_list):
            if i_peak == 0:
                axis0_range = [peak[0], peak[0], 0.]
                axis1_range = [peak[1], peak[1], 0.]
                axis2_range = [peak[2], peak[2], 0.]
            else:
                # axis 0
                if peak[0] < axis0_range[0]:
                    axis0_range[0] = peak[0]
                elif peak[0] > axis0_range[1]:
                    axis0_range[1] = peak[0]

                # axis 1
                if peak[1] < axis1_range[0]:
                    axis1_range[0] = peak[1]
                elif peak[1] > axis1_range[1]:
                    axis1_range[1] = peak[1]

                # axis 2
                if peak[2] < axis2_range[0]:
                    axis2_range[0] = peak[2]
                elif peak[2] > axis2_range[1]:
                    axis2_range[1] = peak[2]
        # END-FOR

        axis0_range[2] = axis0_range[1] - axis0_range[0]
        axis1_range[2] = axis1_range[1] - axis1_range[0]
        axis2_range[2] = axis2_range[1] - axis2_range[0]

        # edit the message to BinMD for the merged scans
        binning_script = 'Peak centers are :\n'
        for peak_center in scan_peak_centre_list:
            binning_script += '\t%.5f, %.5f, %.5f\n' % (peak_center[0], peak_center[1], peak_center[2])

        if frame == 'HKL':
            # HKL space
            binning_script += 'BinMD(InputWorkspace=%s, ' \
                              'AlignedDim0=\'H,%.5f,%.5f,100\', ' \
                              'AlignedDim1=\'K,%.5f,%.5f,100\', ' \
                              'AlignedDim2=\'L,%.5f,%.5f,100\', ' \
                              'OutputWorkspace=%s)' % (merged_ws_name, axis0_range[0]-1, axis0_range[1] + 1,
                                                       axis1_range[0] - 1, axis1_range[1] + 1,
                                                       axis2_range[0] - 1, axis2_range[1] + 1,
                                                       merged_ws_name + '_Histogram')
        elif frame == 'QSample':
            # Q-space
            binning_script += 'BinMD(InputWorkspace=%s, ' \
                              'AlignedDim0=\'Q_sample_x,%.5f,%.5f,100\', ' \
                              'AlignedDim1=\'Q_sample_y,%.5f,%.5f,100\', ' \
                              'AlignedDim2=\'Q_sample_z,%.5f,%.5f,100\', ' \
                              'OutputWorkspace=%s)' % (merged_ws_name, axis0_range[0]-1, axis0_range[1] + 1,
                                                       axis1_range[0] - 1, axis1_range[1] + 1,
                                                       axis2_range[0] - 1, axis2_range[1] + 1,
                                                       merged_ws_name + '_Histogram')
        # END-IF

        binning_script += '\nNote: Here the resolution is 100.  You may modify it and view by SliceViewer.'

        binning_script += '\n\nRange: \n'
        binning_script += 'Axis 0: %.5f, %5f (%.5f)\n' % (axis0_range[0], axis0_range[1], axis0_range[2])
        binning_script += 'Axis 1: %.5f, %5f (%.5f)\n' % (axis1_range[0], axis1_range[1], axis1_range[2])
        binning_script += 'Axis 2: %.5f, %5f (%.5f)\n' % (axis2_range[0], axis2_range[1], axis2_range[2])

        return binning_script

    def is_calibration_match(self, exp_number, scan_number):
        """
        check whether the pre-processed data has a set of matching calibrated parameters comparing to
        the current one
        :param exp_number:
        :param scan_number:
        :return:
        """
        # no record is found. it should not happen!
        if self._preprocessedInfoDict is None:
            return False
        if scan_number not in self._preprocessedInfoDict:
            return False

        # check others
        unmatch_score = 0

        # center
        center_x, center_y = self.get_calibrated_det_center(exp_number)
        if (center_x, center_y) != self._preprocessedInfoDict[scan_number]['Center']:
            unmatch_score += 2

        # wave length
        wavelength = self.get_calibrated_wave_length(exp_number)
        record_lambda = self._preprocessedInfoDict[scan_number]['WaveLength']
        if type(record_lambda) != type(wavelength):
            unmatch_score += 20
        elif wavelength is not None and abs(wavelength - record_lambda) > 1.E-5:
            unmatch_score += 40

        # detector distance
        det_sample_distance = self.get_calibrated_det_sample_distance(exp_number)
        record_distance = self._preprocessedInfoDict[scan_number]['DetSampleDistance']
        if type(det_sample_distance) != type(record_distance):
            unmatch_score += 200
        elif det_sample_distance is not None and abs(det_sample_distance - record_distance) > 1.E-5:
            unmatch_score += 400

        if unmatch_score > 0:
            if self._debugPrintMode:
                print('[INFO] Exp {0} Scan {1} has a unmatched calibrated record from pre-processed data. ID = {2}'
                      ''.format(exp_number, scan_number, unmatch_score))
            return False

        if self._debugPrintMode:
            print('[INFO] Exp {0} Scan {1} has a matched calibrated record from pre-processed data.')

        return True

    def load_mask_file(self, mask_file_name, mask_tag):
        """
        load an XML mask file to a workspace and parse to ROI that can be mapped pixels in 2D notion
        :param mask_file_name:
        :param mask_tag
        :return: 2-tuple (lower left corner (size = 2), upper right corner (size = 2))
                both of them are in order of row and column number (y and x respectively)
        """
        # load mask file
        assert isinstance(mask_file_name, str), 'Mask file {0} shall be a string but not a {1}.' \
                                                ''.format(mask_file_name, type(mask_file_name))
        assert isinstance(mask_tag, str), 'Mask tag {0} shall be a string but not a {1}.' \
                                          ''.format(mask_tag, type(mask_tag))
        if os.path.exists(mask_file_name) is False:
            raise RuntimeError('Mask file name {0} cannot be found.'.format(mask_tag))

        # load
        mantidsimple.LoadMask(Instrument='HB3A',
                              InputFile=mask_file_name,
                              OutputWorkspace=mask_tag)
        # record
        self.set_roi_workspace(roi_name=mask_tag, mask_ws_name=mask_tag)

        # find out the range of the ROI in (Low left, upper right) mode
        roi_range = process_mask.get_region_of_interest(mask_tag)
        self.set_roi(mask_tag, roi_range[0], roi_range[1])

        return roi_range

    def load_preprocessed_scan(self, exp_number, scan_number, md_dir, output_ws_name):
        """ load preprocessed scan from hard disk
        :return: (bool, str): loaded, message
        """
        # check inputs
        assert isinstance(exp_number, int), 'Experiment number {0} ({1}) must be an integer' \
                                            ''.format(exp_number, type(exp_number))
        assert isinstance(scan_number, int), 'Scan number {0} ({1}) must be an integer.' \
                                             ''.format(scan_number, type(scan_number))
        assert isinstance(md_dir, str), 'MD file directory {0} ({1}) must be a string.' \
                                        ''.format(md_dir, type(md_dir))
        assert isinstance(output_ws_name, str), 'Output workspace name {0} ({1}) must be a string.' \
                                                ''.format(output_ws_name, type(output_ws_name))

        if os.path.exists(md_dir) is False:
            raise RuntimeError('Pre-processed directory {0} does not exist.'.format(md_dir))

        # ws_name = 'Exp{0}_Scan{1}_MD'.format(exp_number, scan_number)
        # md_file_path = os.path.join(md_dir, ws_name + '.nxs')

        # 2-ways to get file name
        if self._preprocessedInfoDict is None or scan_number not in self._preprocessedInfoDict:
            md_file_path = fourcircle_utility.pre_processed_file_name(exp_number, scan_number, md_dir)
        else:
            md_file_path = self._preprocessedInfoDict[scan_number]['MD']

        # check
        if os.path.exists(md_file_path) is False:
            message = 'Pre-processed MD file {0} does not exist.'.format(md_file_path)
            return False, message

        # load and check
        status = False
        try:
            # load
            mantidsimple.LoadMD(Filename=md_file_path, OutputWorkspace=output_ws_name)
            # check
            status = AnalysisDataService.doesExist(output_ws_name)
            message = '{0} is loaded from {1} with status {2}'.format(output_ws_name, md_file_path, status)
        except RuntimeError as run_err:
            message = 'Unable to load file {0} due to RuntimeError {1}.'.format(md_file_path, run_err)
        except OSError as run_err:
            message = 'Unable to load file {0} due to OSError {1}.'.format(md_file_path, run_err)
        except IOError as run_err:
            message = 'Unable to load file {0} due to IOError {1}.'.format(md_file_path, run_err)

        return status, message

    def _process_pt_list(self, exp_no, scan_no, pt_num_list):
        """
        convert list of Pt (in int) to a string like a list of integer
        :param exp_no:
        :param scan_no:
        :return:
        """
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
        if pt_list_str == '-1':
            return False, err_msg

        return True, (pt_num_list, pt_list_str)

    def merge_pts_in_scan(self, exp_no, scan_no, pt_num_list, rewrite, preprocessed_dir):
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
        :param rewrite: if True, then the data will be re-merged regardless workspace exists or not
        :param preprocessed_dir: If None, then merge Pts. Otherwise, try to search and load preprocessed data first
        :return: (boolean, error message) # (merged workspace name, workspace group name)
        """
        # Check
        if exp_no is None:
            exp_no = self._expNumber
        assert isinstance(exp_no, int) and isinstance(scan_no, int)
        assert isinstance(pt_num_list, list), 'Pt number list must be a list but not %s' % str(type(pt_num_list))

        # Get list of Pt.
        status, ret_obj = self._process_pt_list(exp_no, scan_no, pt_num_list)
        if not status:
            error_msg = ret_obj
            return False, error_msg
        pt_num_list, pt_list_str = ret_obj

        # create output workspace's name
        out_q_name = get_merged_md_name(self._instrumentName, exp_no, scan_no, pt_num_list)

        # find out the cases that rewriting is True
        if not rewrite:
            if AnalysisDataService.doesExist(out_q_name):
                # not re-write, target workspace exists
                pass
            elif preprocessed_dir is not None:
                # not re-write, target workspace does not exist, attempt to load from preprocessed
                if self.is_calibration_match(exp_no, scan_no):
                    data_loaded, message = self.load_preprocessed_scan(exp_number=exp_no,
                                                                       scan_number=scan_no,
                                                                       md_dir=preprocessed_dir,
                                                                       output_ws_name=out_q_name)
                    rewrite = not data_loaded
                else:
                    rewrite = True
            else:
                print ('[WARNING] Target MDWorkspace does not exist. And preprocessed directory is not given '
                       '. Why re-write flag is turned off in the first place?')
                rewrite = True
            # END-IF (ADS)
        # END-IF (rewrite)

        # now to load the data
        # check whether it is an option load preprocessed (merged) data
        if rewrite:
            # collect HB3A Exp/Scan information
            # - construct a configuration with 1 scan and multiple Pts.
            scan_info_table_name = get_merge_pt_info_ws_name(exp_no, scan_no)
            try:
                # collect HB3A exp info only need corrected detector position to build virtual instrument.
                # so it is not necessary to specify the detector center now as virtual instrument
                # is abandoned due to speed issue.
                mantidsimple.CollectHB3AExperimentInfo(ExperimentNumber=exp_no,
                                                       ScanList='%d' % scan_no,
                                                       PtLists=pt_list_str,
                                                       DataDirectory=self._dataDir,
                                                       GenerateVirtualInstrument=False,
                                                       OutputWorkspace=scan_info_table_name,
                                                       DetectorTableWorkspace='MockDetTable')
            except RuntimeError as rt_error:
                return False, 'Unable to merge scan %d dur to %s.' % (scan_no, str(rt_error))
            else:
                # check
                assert AnalysisDataService.doesExist(scan_info_table_name), 'Workspace %s does not exist.' \
                                                                            '' % scan_info_table_name
            # END-TRY-EXCEPT

            # create MD workspace in Q-sample
            try:
                # set up the basic algorithm parameters
                alg_args = dict()
                alg_args['InputWorkspace'] = scan_info_table_name
                alg_args['CreateVirtualInstrument'] = False
                alg_args['OutputWorkspace'] = out_q_name
                alg_args['Directory'] = self._dataDir

                # Add Detector Center and Detector Distance!!!  - Trace up how to calculate shifts!
                # calculate the sample-detector distance shift if it is defined
                if exp_no in self._detSampleDistanceDict:
                    alg_args['DetectorSampleDistanceShift'] \
                        = self._detSampleDistanceDict[exp_no] - self._defaultDetectorSampleDistance
                # calculate the shift of detector center
                if exp_no in self._detCenterDict:
                    user_center_row, user_center_col = self._detCenterDict[exp_no]
                    delta_row = user_center_row - self._defaultDetectorCenter[0]
                    delta_col = user_center_col - self._defaultDetectorCenter[1]
                    # use LoadSpiceXML2DDet's unit test as a template
                    shift_x = float(delta_col) * self._defaultPixelSizeX
                    shift_y = float(delta_row) * self._defaultPixelSizeY * -1.
                    # set to argument
                    alg_args['DetectorCenterXShift'] = shift_x
                    alg_args['DetectorCenterYShift'] = shift_y

                # set up the user-defined wave length
                if exp_no in self._userWavelengthDict:
                    alg_args['UserDefinedWavelength'] = self._userWavelengthDict[exp_no]

                # call:
                mantidsimple.ConvertCWSDExpToMomentum(**alg_args)

                self._myMDWsList.append(out_q_name)
            except RuntimeError as e:
                err_msg += 'Unable to convert scan %d data to Q-sample MDEvents due to %s' % (scan_no, str(e))
                return False, err_msg
            except ValueError as e:
                err_msg += 'Unable to convert scan %d data to Q-sample MDEvents due to %s.' % (scan_no, str(e))
                return False, err_msg
            # END-TRY

        else:
            # analysis data service has the target MD workspace. do not load again
            if out_q_name not in self._myMDWsList:
                self._myMDWsList.append(out_q_name)
        # END-IF-ELSE

        return True, (out_q_name, '')

    def convert_merged_ws_to_hkl(self, exp_number, scan_number, pt_num_list):
        """
        convert a merged scan in MDEventWorkspace to HKL
        :param exp_number:
        :param scan_number:
        :param pt_num_list:
        :return:
        """
        # check inputs' validity
        assert isinstance(exp_number, int), 'Experiment number must be an integer.'
        assert isinstance(scan_number, int), 'Scan number must be an integer.'

        # retrieve UB matrix stored and convert to a 1-D array
        if exp_number not in self._myUBMatrixDict:
            raise RuntimeError('There is no UB matrix associated with experiment %d.' % exp_number)
        else:
            ub_matrix_1d = self._myUBMatrixDict[exp_number].reshape(9,)

        # convert to HKL
        input_md_qsample_ws = get_merged_md_name(self._instrumentName, exp_number, scan_number, pt_list=pt_num_list)
        out_hkl_name = get_merged_hkl_md_name(self._instrumentName, exp_number, scan_number, pt_num_list)
        try:
            mantidsimple.ConvertCWSDMDtoHKL(InputWorkspace=input_md_qsample_ws,
                                            UBMatrix=ub_matrix_1d,
                                            OutputWorkspace=out_hkl_name)

        except RuntimeError as e:
            err_msg = 'Failed to reduce scan %d from MDWorkspace %s due to %s' % (scan_number, input_md_qsample_ws,
                                                                                  str(e))
            return False, err_msg

        return True, out_hkl_name

    def save_merged_scan(self, exp_number, scan_number, pt_number_list, merged_ws_name, output):
        """

        :param exp_number:
        :param scan_number:
        :param pt_number_list:
        :param merged_ws_name:
        :param output: output file path
        :return:
        """
        assert isinstance(exp_number, int), 'Experiment number {0} must be an integer but not a {1}.' \
                                            ''.format(scan_number, type(scan_number))
        assert isinstance(scan_number, int), 'Scan number {0} must be an integer but not a {1}.' \
                                             ''.format(exp_number, type(exp_number))
        assert isinstance(output, str), 'Output file name {0} must be give as a string but not {1}.' \
                                        ''.format(output, type(output))

        # get input workspace

        if merged_ws_name is None:
            merged_ws_name = get_merged_md_name(self._instrumentName, exp_number, scan_number,
                                                pt_list=pt_number_list)
        mantidsimple.SaveMD(InputWorkspace=merged_ws_name, Filename=output)

        return

    def set_roi(self, roi_name, lower_left_corner, upper_right_corner):
        """
        Purpose: Set region of interest and record it by the combination of experiment number
                 and scan number
        :param roi_name
        :param lower_left_corner:
        :param upper_right_corner:
        :return:
        """
        # Check
        check_str_type(roi_name, 'ROI')
        if len(lower_left_corner) != 2:
            raise RuntimeError('Size of ROI[0] must be 2 but not {0}'
                               ''.format(len(lower_left_corner[0])))
        if len(upper_right_corner) != 2:
            raise RuntimeError('Size of ROI[1] must be 2 but not {0}'
                               ''.format(len(upper_right_corner)))

        if roi_name not in self._roiDict:
            self._roiDict[roi_name] = process_mask.RegionOfInterest(roi_name)

        self._roiDict[roi_name].set_roi_positions(lower_left_corner, upper_right_corner)

        return

    def set_roi_workspace(self, roi_name, mask_ws_name):
        """ set region of interest (mask) workspace's name to RegionOfInterest
        instance
        :param roi_name:
        :return:
        """
        # check input
        check_str_type(roi_name, 'ROI')
        check_str_type(mask_ws_name, 'Mask workspace name')

        # add new RegionOfInterest if needed
        if roi_name not in self._roiDict:
            self._roiDict[roi_name] = process_mask.RegionOfInterest(roi_name)

        self._roiDict[roi_name].set_mask_workspace_name(mask_ws_name)

        return

    def set_roi_by_name(self, roi_name, lower_left_corner, upper_right_corner):
        """
        Set region of interest and record it by user-specified ROI name
        :param roi_name:
        :param lower_left_corner:
        :param upper_right_corner:
        :return:
        """
        assert isinstance(roi_name, str), 'ROI name {0} must be an integer but not a {1}' \
                                          ''.format(roi_name, type(roi_name))
        assert not isinstance(lower_left_corner, str) and len(lower_left_corner) == 2
        assert not isinstance(upper_right_corner, str) and len(upper_right_corner) == 2

        ll_x = int(lower_left_corner[0])
        ll_y = int(lower_left_corner[1])
        ur_x = int(upper_right_corner[0])
        ur_y = int(upper_right_corner[1])
        if ll_x >= ur_x or ll_y >= ur_y:
            err_msg = 'Lower left corner ({0}, {1}) and upper right corner are in a line ({2}, {3})' \
                      ''.format(ll_x, ll_y, ur_x, ur_y)
            raise RuntimeError(err_msg)

        # Add to dictionary.  Because usually one ROI is defined for all scans in an experiment,
        # then it is better and easier to support client to search this ROI by experiment number
        # and only the latest is saved by this key
        self._roiDict[roi_name] = ((ll_x, ll_y), (ur_x, ur_y))

        return

    def get_calibrated_det_center(self, exp_number):
        """
        get calibrated/user-specified detector center or the default center
        :param exp_number:
        :return: 2-tuple (int, int) as pixel ID in X and Y directory
        """
        # check inputs
        assert isinstance(exp_number, int), 'Experiment number {0} ({1}) must be an integer.' \
                                            ''.format(exp_number, type(exp_number))

        if exp_number not in self._detCenterDict:
            return self._defaultDetectorCenter

        return self._detCenterDict[exp_number]

    def set_detector_center(self, exp_number, center_row, center_col, default=False):
        """
        Set detector center
        :param exp_number:
        :param center_row:
        :param center_col:
        :param default:
        :return:
        """
        # check
        assert isinstance(exp_number, int) and exp_number > 0, 'Experiment number must be integer'
        assert center_row is None or (isinstance(center_row, int) and center_row >= 0), \
            'Center row number {0} of type {1} must either None or non-negative integer.' \
            ''.format(center_row, type(center_row))
        assert center_col is None or (isinstance(center_col, int) and center_col >= 0), \
            'Center column number {0} of type {1} must be either None or non-negative integer.' \
            ''.format(center_col, type(center_col))

        if default:
            self._defaultDetectorCenter = center_row, center_col
        else:
            self._detCenterDict[exp_number] = center_row, center_col

        return

    def set_detector_geometry(self, size_x, size_y):
        """
        set the detector's geometry, i.e., size
        :param size_x:
        :param size_y:
        :return:
        """
        # check inputs
        assert isinstance(size_x, int) and size_x > 0, 'Input detector size-X {0} must be a positive integer.' \
                                                       ''.format(size_x)
        assert isinstance(size_y, int) and size_y > 0, 'Input detector size-Y {0} must be a positive integer.' \
                                                       ''.format(size_y)

        self._detectorSize[0] = size_x
        self._detectorSize[1] = size_y

        return

    def get_calibrated_det_sample_distance(self, exp_number):
        """

        :param exp_number:
        :return:
        """
        # check inputs
        assert isinstance(exp_number, int) and exp_number > 0, 'Experiment number must be integer'

        if exp_number not in self._detSampleDistanceDict:
            return None

        return self._detSampleDistanceDict[exp_number]

    def set_detector_sample_distance(self, exp_number, sample_det_distance):
        """
        set instrument's detector - sample distance
        :param exp_number:
        :param sample_det_distance:
        :return:
        """
        # check
        assert isinstance(exp_number, int) and exp_number > 0, 'Experiment number must be integer'
        assert isinstance(sample_det_distance, float) and sample_det_distance > 0, \
            'Sample - detector distance must be a positive float.'

        # set
        self._detSampleDistanceDict[exp_number] = sample_det_distance

        return

    def set_default_detector_sample_distance(self, default_det_sample_distance):
        """set default detector-sample distance
        :param default_det_sample_distance:
        :return:
        """
        assert isinstance(default_det_sample_distance, float) and default_det_sample_distance > 0,\
            'Wrong %s' % str(default_det_sample_distance)

        self._defaultDetectorSampleDistance = default_det_sample_distance

        return

    def set_default_pixel_size(self, pixel_x_size, pixel_y_size):
        """set default pixel size, i.e., physical dimension of a pixel
        :param pixel_x_size:
        :param pixel_y_size:
        :return:
        """
        assert isinstance(pixel_x_size, float) and pixel_x_size > 0, 'Pixel size-X %s is bad!' % str(pixel_x_size)
        assert isinstance(pixel_y_size, float) and pixel_y_size > 0, 'Pixel size-Y %s is bad!' % str(pixel_y_size)

        self._defaultPixelSizeX = pixel_x_size
        self._defaultPixelSizeY = pixel_y_size

        return

    def set_default_pixel_number(self, num_pixel_x, num_pixel_y):
        """
        set the default number of pixels on the detector
        :param num_pixel_x:
        :param num_pixel_y:
        :return:
        """
        assert isinstance(num_pixel_x, int) and num_pixel_x > 0, 'Wrong input'
        assert isinstance(num_pixel_y, int) and num_pixel_y > 0, 'Wrong input'

        self._defaultPixelNumberX = num_pixel_x
        self._defaultPixelNumberY = num_pixel_y

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
                result = urlopen(self._myServerURL)
            except HTTPError as err:
                error_message = str(err.code)
            except URLError as err:
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

        # Check whether the target is writable: if and only if the data directory is not from data server
        if not local_dir.startswith('/HFIR/HB3A/') and os.access(local_dir, os.W_OK) is False:
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

        return

    def set_user_wave_length(self, exp_number, wave_length):
        """
        set the user wave length for future operation
        :param exp_number:
        :param wave_length:
        :return:
        """
        assert isinstance(exp_number, int)
        assert isinstance(wave_length, float) and wave_length > 0, 'Wave length %s must be a positive float but ' \
                                                                   'not %s.' % (str(wave_length), type(wave_length))

        self._userWavelengthDict[exp_number] = wave_length

        return

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

    def refine_ub_matrix_indexed_peaks(self, peak_info_list):
        """ Refine UB matrix by SPICE-indexed peaks
        Requirements: input is a list of PeakInfo objects and there are at least 3
                        non-degenerate peaks
        Guarantees: UB matrix is refined.  Refined UB matrix and lattice parameters
                    with errors are returned
        :param peak_info_list: list of PeakInfo
        :return: 2-tuple: (True, (ub matrix, lattice parameters, lattice parameters errors))
                          (False, error message)
        """
        # Check inputs
        assert isinstance(peak_info_list, list)
        assert len(peak_info_list) >= 3

        # Construct a new peak workspace by combining all single peak
        ub_peak_ws_name = 'TempUBIndexedPeaks'
        self._build_peaks_workspace(peak_info_list, ub_peak_ws_name)

        # Calculate UB matrix
        try:
            mantidsimple.FindUBUsingIndexedPeaks(PeaksWorkspace=ub_peak_ws_name, Tolerance=0.5)
        except RuntimeError as e:
            return False, 'Unable to refine UB matrix due to %s.' % str(e)

        # Get peak workspace
        self._refinedUBTup = self._get_refined_ub_data(ub_peak_ws_name)

        return

    def read_spice_file(self, exp_number, scan_number):
        """
        Read SPICE file
        :param exp_number: experiment number
        :param scan_number: scan number
        :return: a list of string for each line
        """
        # check inputs' validity
        assert isinstance(exp_number, int) and exp_number > 0, 'Experiment number must be a positive integer.'
        assert isinstance(scan_number, int) and scan_number > 0, 'Scan number must be a positive integer.'

        # get the local SPICE file
        status, ret_string = self.download_spice_file(exp_number, scan_number, over_write=False)
        assert status, ret_string
        spice_file_name = ret_string

        # read the SPICE file
        spice_file = open(spice_file_name, 'r')
        spice_line_list = spice_file.readlines()
        spice_file.close()

        return spice_line_list

    def refine_ub_matrix_by_lattice(self, peak_info_list, ub_matrix_str, unit_cell_type):
        """
        Refine UB matrix by fixing unit cell type
        Requirements:
          1. PeakProcessRecord in peak_info_list must have right HKL set as user specified
          2. the index of the peaks that are used for refinement are given in PeakProcessRecord's user specified HKL

        :param peak_info_list:
        :param ub_matrix_str:
        :param unit_cell_type:
        :return:
        """
        # check inputs and return if not good
        assert isinstance(peak_info_list, list), 'peak_info_list must be a list but not %s.' % type(peak_info_list)
        if len(peak_info_list) < 6:
            return False, 'There must be at least 6 peaks for refining UB. Now only %d is given.' % len(peak_info_list)

        assert isinstance(ub_matrix_str, str), 'UB matrix must be input in form of string but not %s.' \
                                               '' % type(ub_matrix_str)
        if len(ub_matrix_str.split(',')) != 9:
            return False, 'UB matrix string must have 9 values. Now given %d as %s.' % (len(ub_matrix_str.split(',')),
                                                                                        ub_matrix_str)
        assert isinstance(unit_cell_type, str) and len(unit_cell_type) >= 5,\
            'Unit cell type must be given as a string but not %s.' % type(unit_cell_type)

        # construct a new workspace by combining all single peaks
        ub_peak_ws_name = 'TempRefineUBLatticePeaks'
        self._build_peaks_workspace(peak_info_list, ub_peak_ws_name)

        # set UB matrix from input string. It is UB(0, 0), UB(0, 1), UB(0, 2), UB(1, 0), ..., UB(3, 3)
        mantidsimple.SetUB(Workspace=ub_peak_ws_name,
                           UB=ub_matrix_str)

        # optimize UB matrix by constraining lattice parameter to unit cell type
        mantidsimple.OptimizeLatticeForCellType(PeaksWorkspace=ub_peak_ws_name,
                                                CellType=unit_cell_type,
                                                Apply=True,
                                                OutputDirectory=self._workDir)

        # get refined ub matrix
        self._refinedUBTup = self._get_refined_ub_data(ub_peak_ws_name)

        return True, ''

    def refine_ub_matrix_least_info(self, peak_info_list, d_min, d_max, tolerance):
        """
        Refine UB matrix with least information from user, i.e., using FindUBFFT
        Requirements: at least 6 PeakInfo objects are given
        Guarantees: Refine UB matrix by FFT
        :return:
        """
        # Check
        assert isinstance(peak_info_list, list), 'peak_info_list must be a list but not of type %s.' \
                                                 '' % type(peak_info_list)
        assert isinstance(d_min, float) and isinstance(d_max, float), 'd_min and d_max must be float but not ' \
                                                                      '%s and %s.' % (type(d_min), type(d_max))

        if len(peak_info_list) < 6:
            raise RuntimeError('There must be at least 6 peaks to refine UB matrix by FFT. Only %d peaks '
                               'are given.' % len(peak_info_list))
        if not (0 < d_min < d_max):
            raise RuntimeError('It is required to have 0 < d_min (%f) < d_max (%f).' % (d_min, d_max))

        # Build a new PeaksWorkspace
        peak_ws_name = 'TempUBFFTPeaks'
        self._build_peaks_workspace(peak_info_list, peak_ws_name)

        # Refine
        mantidsimple.FindUBUsingFFT(PeaksWorkspace=peak_ws_name,
                                    Tolerance=tolerance,
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

        result_tuple = (peak_ws, refined_ub_matrix, lattice, lattice_error)

        return result_tuple

    @staticmethod
    def _build_peaks_workspace(peak_info_list, peak_ws_name):
        """
        From a list of PeakInfo, using the averaged peak centre of each of them
        to build a new PeaksWorkspace
        Requirements: a list of PeakInfo with HKL specified by user
        Guarantees: a PeaksWorkspace is created in AnalysisDataService.
        :param peak_info_list: peak information list.  only peak center in Q-sample is required
        :param peak_ws_name:
        :return:
        """
        # check
        assert isinstance(peak_info_list, list), 'Peak Info List must be a list.'
        assert len(peak_info_list) > 0, 'Peak Info List cannot be empty.'
        assert isinstance(peak_ws_name, str), 'Peak workspace name must be a string.'

        # create an empty
        mantidsimple.CreatePeaksWorkspace(NumberOfPeaks=0, OutputWorkspace=peak_ws_name)
        assert AnalysisDataService.doesExist(peak_ws_name)
        peak_ws = AnalysisDataService.retrieve(peak_ws_name)

        # add peak
        num_peak_info = len(peak_info_list)
        for i_peak_info in range(num_peak_info):
            # Set HKL as optional
            peak_info_i = peak_info_list[i_peak_info]
            peak_ws_i = peak_info_i.get_peak_workspace()
            assert peak_ws_i.getNumberPeaks() > 0

            # get any peak to add. assuming that each peak workspace has one and only one peak
            peak_temp = peak_ws_i.getPeak(0)
            peak_ws.addPeak(peak_temp)
            peak_i = peak_ws.getPeak(i_peak_info)

            # set the peak indexing to each pear
            index_h, index_k, index_l = peak_info_i.get_hkl(user_hkl=True)
            peak_i.setHKL(index_h, index_k, index_l)
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
        titles = ['Max Counts', 'Scan', 'Max Counts Pt', 'H', 'K', 'L', 'Q', 'Sample T']
        with open(file_name, 'w') as csv_file:
            csv_writer = csv.writer(csv_file, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
            csv_writer.writerow(titles)

            for sum_index, scan_summary in enumerate(self._scanSummaryList):
                # check type
                assert isinstance(scan_summary, list), 'TODO'
                assert len(scan_summary) == len(titles), '{0}-th scan summary {1} must have same items as title {2}' \
                                                         ''.format(sum_index, scan_summary, titles)
                # write to csv
                csv_writer.writerow(scan_summary)
            # END-FOR
        # END-WITH

        return

    # def save_roi(self, roi_name, region_of_interest):
    #     """
    #     Save region of interest to controller for future use
    #     :param roi_name:
    #     :param region_of_interest: a 2-tuple for 2-tuple as lower-left and upper-right corners of the region
    #     :return:
    #     """
    #     # check
    #     assert isinstance(roi_name, str), 'Tag {0} must be a string {1}'.format(roi_name, type(roi_name))
    #
    #
    #     # save ROI to ROI dictionary
    #     self._roiDict[roi_name] = region_of_interest
    #
    #     # if ROI already been used to mask detectors, then need to change mask workspace dictionary
    #     if self.RESERVED_ROI_NAME in self._maskWorkspaceDict:
    #         self._maskWorkspaceDict[roi_name] = self._maskWorkspaceDict[self.RESERVED_ROI_NAME]
    #         del self._maskWorkspaceDict[self.RESERVED_ROI_NAME]
    #     # END-IF
    #
    #     return

    def save_roi_to_file(self, exp_number, scan_number, tag, file_name):
        """
        save ROI to file
        Notice: the saved file is a mask file which masks the detectors out of ROI
        :param exp_number:
        :param scan_number:
        :param tag:
        :param file_name:
        :return:
        """
        # check input
        # assert isinstance(exp_number, int), 'Experiment number {0} shall be an integer but not a {1}' \
        #                                     ''.format(exp_number, type(exp_number))
        # assert isinstance(scan_number, int), 'Scan number {0} shall be an integer but not a {1}' \
        #                                      ''.format(scan_number, type(scan_number))
        assert isinstance(tag, str), 'Tag {0} shall be a string but not a {1}'.format(tag, type(tag))
        assert isinstance(file_name, str), 'File name {0} shall be a string but not a {1}' \
                                           ''.format(file_name, type(file_name))

        # get mask workspace name
        if tag in self._roiDict:
            mask_ws_name = self._roiDict[tag].mask_workspace
            if mask_ws_name is None or AnalysisDataService.doesExist(mask_ws_name) is False:
                raise RuntimeError('Mask workspace {0} of tag {1} does not exist in ADS.'
                                   ''.format(mask_ws_name, tag))
        else:
            mask_ws_name = self.check_generate_mask_workspace(exp_number=exp_number, scan_number=scan_number,
                                                              mask_tag=tag, check_throw=True)

        # save
        mantidsimple.SaveMask(InputWorkspace=mask_ws_name, OutputFile=file_name)

        return

    def set_exp_number(self, exp_number):
        """ Add experiment number
        :param exp_number:
        :return:
        """
        assert isinstance(exp_number, int)
        self._expNumber = exp_number

        return True

    def set_k_shift(self, scan_number_list, k_index):
        """ Set k-shift vector
        :param scan_number_list:
        :param k_index:
        :return:
        """
        # check
        assert isinstance(scan_number_list, list) and len(scan_number_list) > 0
        assert isinstance(k_index, int)
        assert k_index == 0 or k_index in self._kShiftDict, \
            'K-index %d is not in K-shift dictionary (%s).' % (k_index, str(self._kShiftDict.keys()))

        # add to the new and remove from the previous placeholder
        for scan_number in scan_number_list:
            # add to the target k-index list
            if k_index > 0 and scan_number not in self._kShiftDict[k_index][1]:
                self._kShiftDict[k_index][1].append(scan_number)

            # add to the peak info
            peak_info = self.get_peak_info(self._expNumber, scan_number)
            peak_info.set_k_vector(self._kShiftDict[k_index][0])

            # remove from the previous placeholder
            for k_i in self._kShiftDict.keys():
                # skip current one
                if k_i == k_index:
                    continue

                # check whether scan number is in this list
                if scan_number in self._kShiftDict[k_i][1]:
                    self._kShiftDict[k_i][1].remove(scan_number)
                    break
            # END-FOR (k_i)
        # END-FOR (scan_number)

        return

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
        :return: (boolean, PeakInfo/string)
        """
        # check
        assert isinstance(exp_number, int), 'Experiment number must be an integer.'
        assert isinstance(scan_number, int), 'Scan number must an be integer.'
        assert isinstance(peak_ws_name, str), 'PeaksWorkspace must be a string.'
        assert isinstance(md_ws_name, str), 'MDEventWorkspace name must be a string.'

        # check whether there is a redundant creation of PeakProcessRecord for the same (exp, scan) combination
        if (exp_number, scan_number) in self._myPeakInfoDict:
            peak_info = self._myPeakInfoDict[(exp_number, scan_number)]
            if self._debugPrintMode:
                print('[ERROR] PeakProcessRecord for Exp {0} Scan {1} shall not '
                      'be created twice!'.format(exp_number, scan_number))
                print('[CONTINUE] New PeaksWorkspace = {0} vs Existing '
                      'PeaksWorkspace = {1}.'.format(peak_ws_name, peak_info.peaks_workspace))
                print('[CONTINUE] New MDEventWorkspace = {0} vs Existing '
                      'MDEventWorkspace = {1}.'.format(md_ws_name, peak_info.md_workspace))
            return False, peak_info
        # END-IF

        # create a PeakInfo instance if it does not exist
        peak_info = PeakProcessRecord(exp_number, scan_number, peak_ws_name)
        self._myPeakInfoDict[(exp_number, scan_number)] = peak_info

        # set the other information
        peak_info.set_data_ws_name(md_ws_name)
        err_msg = peak_info.calculate_peak_center()
        if self._debugPrintMode and len(err_msg) > 0:
            print ('[Error] during calculating peak center:{0}'.format(err_msg))

        return True, peak_info

    def _add_spice_workspace(self, exp_no, scan_no, spice_table_ws):
        """
        """
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)
        assert isinstance(spice_table_ws, mantid.dataobjects.TableWorkspace)
        self._mySpiceTableDict[(exp_no, scan_no)] = str(spice_table_ws)

        return

    @staticmethod
    def _get_spice_workspace(exp_no, scan_no):
        """ Get SPICE's scan table workspace
        :param exp_no:
        :param scan_no:
        :return: Table workspace or None
        """
        # try:
        #     ws = self._mySpiceTableDict[(exp_no, scan_no)]
        # except KeyError:
        #     return None

        spice_ws_name = get_spice_table_name(exp_no, scan_no)
        if AnalysisDataService.doesExist(spice_ws_name):
            ws = AnalysisDataService.retrieve(spice_ws_name)
        else:
            raise KeyError('Spice table workspace %s does not exist in ADS.' % spice_ws_name)

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
        for irow in range(numrows):
            ptno = int(spice_table_ws.cell(irow, 0))
            ptlist.append(ptno)

        return ptlist

    def set_zero_peak_intensity(self, exp_number, scan_number):
        """
        Set peak intensity to a scan and set to PeakInfo
        :param exp_number:
        :param scan_number:
        :param intensity:
        :return:
        """
        # check
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)

        # get dictionary item
        err_msg = 'Exp %d Scan %d does not exist in peak information' \
                  ' dictionary.' % (exp_number, scan_number)
        assert (exp_number, scan_number) in self._myPeakInfoDict, err_msg
        peak_info = self._myPeakInfoDict[(exp_number, scan_number)]

        # set intensity
        peak_info.set_intensity_to_zero()

        return True, ''

    @staticmethod
    def simple_integrate_peak(pt_intensity_dict, bg_value):
        """
        A simple approach to integrate peak in a cuboid with background removed.
        :param pt_intensity_dict:
        :param bg_value:
        :return:
        """
        # check
        assert isinstance(pt_intensity_dict, dict)
        assert isinstance(bg_value, float) and bg_value >= 0.

        # loop over Pt. to sum for peak's intensity
        sum_intensity = 0.
        for intensity in pt_intensity_dict.values():
            sum_intensity += intensity - bg_value

        return sum_intensity

    def survey(self, exp_number, start_scan, end_scan):
        """ Load all the SPICE ascii file to get the big picture such that
        * the strongest peaks and their HKL in order to make data reduction and analysis more convenient
        :param exp_number: experiment number
        :param start_scan:
        :param end_scan:
        :return: 3-tuple (status, scan_summary list, error message)
        """
        # Check
        assert isinstance(exp_number, int), 'Experiment number must be an integer but not %s.' % type(exp_number)
        if isinstance(start_scan, int) is False:
            start_scan = 1
        if isinstance(end_scan , int) is False:
            end_scan = MAX_SCAN_NUMBER

        # Output workspace
        scan_sum_list = list()

        error_message = ''

        # Download and
        for scan_number in range(start_scan, end_scan+1):
            # check whether file exists
            if self.does_file_exist(exp_number, scan_number) is False:
                # SPICE file does not exist in data directory. Download!
                # set up URL and target file name
                spice_file_url = get_spice_file_url(self._myServerURL, self._instrumentName, exp_number, scan_number)
                spice_file_name = get_spice_file_name(self._instrumentName, exp_number, scan_number)
                spice_file_name = os.path.join(self._dataDir, spice_file_name)

                # download file and load
                try:
                    mantidsimple.DownloadFile(Address=spice_file_url, Filename=spice_file_name)
                except RuntimeError as download_error:
                    error_message += 'Unable to access/download scan {0} from {1} due to {2}.\n' \
                                     ''.format(scan_number, spice_file_url, download_error)
                    continue
            else:
                spice_file_name = get_spice_file_name(self._instrumentName, exp_number, scan_number)
                spice_file_name = os.path.join(self._dataDir, spice_file_name)

            # Load SPICE file and retrieve information
            try:
                spice_table_ws_name = 'TempTable'
                mantidsimple.LoadSpiceAscii(Filename=spice_file_name,
                                            OutputWorkspace=spice_table_ws_name,
                                            RunInfoWorkspace='TempInfo')
                spice_table_ws = AnalysisDataService.retrieve(spice_table_ws_name)
                num_rows = spice_table_ws.rowCount()

                if num_rows == 0:
                    # it is an empty table
                    error_message += 'Scan %d: empty spice table.\n' % scan_number
                    continue

                col_name_list = spice_table_ws.getColumnNames()
                h_col_index = col_name_list.index('h')
                k_col_index = col_name_list.index('k')
                l_col_index = col_name_list.index('l')
                col_2theta_index = col_name_list.index('2theta')
                m1_col_index = col_name_list.index('m1')
                time_col_index = col_name_list.index('time')
                det_count_col_index = col_name_list.index('detector')
                # optional as T-Sample
                if 'tsample' in col_name_list:
                    tsample_col_index = col_name_list.index('tsample')
                else:
                    tsample_col_index = None

                max_count = 0
                max_row = 0
                max_h = max_k = max_l = 0
                max_tsample = 0.

                two_theta = m1 = -1

                for i_row in range(num_rows):
                    det_count = spice_table_ws.cell(i_row, det_count_col_index)
                    count_time = spice_table_ws.cell(i_row, time_col_index)
                    # normalize max count to count time
                    det_count = float(det_count)/count_time
                    if det_count > max_count:
                        max_count = det_count
                        max_row = i_row
                        max_h = spice_table_ws.cell(i_row, h_col_index)
                        max_k = spice_table_ws.cell(i_row, k_col_index)
                        max_l = spice_table_ws.cell(i_row, l_col_index)
                        two_theta = spice_table_ws.cell(i_row, col_2theta_index)
                        m1 = spice_table_ws.cell(i_row, m1_col_index)
                        # t-sample is not a mandatory sample log in SPICE
                        if tsample_col_index is None:
                            max_tsample = 0.
                        else:
                            max_tsample = spice_table_ws.cell(i_row, tsample_col_index)
                # END-FOR

                # calculate wavelength
                wavelength = get_hb3a_wavelength(m1)
                if wavelength is None:
                    q_range = 0.
                    error_message += 'Scan number {0} has invalid m1 for wavelength.\n'.format(scan_number)
                else:
                    q_range = 4.*math.pi*math.sin(two_theta/180.*math.pi*0.5)/wavelength

                # appending to list
                scan_sum_list.append([max_count, scan_number, max_row, max_h, max_k, max_l,
                                      q_range, max_tsample])

            except RuntimeError as e:
                return False, None, str(e)
            except ValueError as e:
                # Unable to import a SPICE file without necessary information
                error_message += 'Scan %d: unable to locate column h, k, or l. See %s.' % (scan_number, str(e))
        # END-FOR (scan_number)

        if error_message != '':
            print('[Error]\n%s' % error_message)

        self._scanSummaryList = scan_sum_list

        return True, scan_sum_list, error_message

    def save_project(self, project_file_name, ui_dict):
        """ Export project
        - the data structure and information will be written to a ProjectManager file
        :param project_file_name:
        :param ui_dict:
        :return:
        """
        # check inputs' validity
        assert isinstance(project_file_name, str), 'Project file name must be a string but not of type ' \
                                                   '%s.' % type(project_file_name)

        project = project_manager.ProjectManager(mode='export', project_file_path=project_file_name)

        project.add_workspaces(self._myMDWsList)
        project.set('data dir', self._dataDir)
        project.set('gui parameters', ui_dict)

        err_msg = project.save_to_disk(overwrite=False)
        if len(err_msg) == 0:
            err_msg = None

        return err_msg

    def load_project(self, project_file_name):
        """
        Load project from a project file suite
        :param project_file_name:
        :return:
        """
        # check validity
        assert isinstance(project_file_name, str), 'Project file name must be a string but not of type ' \
                                                   '%s.' % type(project_file_name)

        print('[INFO] Load project from %s.' % project_file_name)

        # instantiate a project manager instance and load the project
        saved_project = project_manager.ProjectManager(mode='import', project_file_path=project_file_name)
        err_msg = saved_project.load_from_disk()
        if len(err_msg) == 0:
            err_msg = None

        # set current value
        try:
            self._dataDir = saved_project.get('data dir')
        except KeyError:
            self._dataDir = None

        try:
            ui_dict = saved_project.get('gui parameters')
        except KeyError:
            ui_dict = dict()

        return ui_dict, err_msg


def convert_spice_ub_to_mantid(spice_ub):
    """ Convert SPICE UB matrix to Mantid UB matrix
    :param spice_ub:
    :return: UB matrix in Mantid format
    """
    mantid_ub = numpy.ndarray((3, 3), 'float')
    # row 0
    for i in range(3):
        mantid_ub[0][i] = spice_ub[0][i]
    # row 1
    for i in range(3):
        mantid_ub[1][i] = spice_ub[2][i]
    # row 2
    for i in range(3):
        mantid_ub[2][i] = -1.*spice_ub[1][i]

    return mantid_ub


def convert_mantid_ub_to_spice(mantid_ub):
    """
    """
    spice_ub = numpy.ndarray((3, 3), 'float')
    # row 0
    for i in range(3):
        spice_ub[0, i] = mantid_ub[0, i]
    # row 1
    for i in range(3):
        spice_ub[2, i] = mantid_ub[1, i]
    # row 2
    for i in range(3):
        spice_ub[1, i] = -1.*mantid_ub[2, i]

    return spice_ub
