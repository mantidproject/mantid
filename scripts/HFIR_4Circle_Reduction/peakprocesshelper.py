#pylint: disable=W0403,R0902
import time
import random
from fourcircle_utility import *
from mantid.api import AnalysisDataService
from mantid.kernel import V3D

__author__ = 'wzz'


class PeakProcessRecord(object):
    """ Class containing a peak's information for GUI
    In order to manage some operations for a peak
    It does not contain peak workspace but will hold
    """

    def __init__(self, exp_number, scan_number, peak_ws_name):
        """ Initialization
        Purpose: set up unchanged parameters including experiment number, scan number and peak workspace's name
        """
        # check
        assert isinstance(exp_number, int) and isinstance(scan_number, int)
        assert isinstance(peak_ws_name, str), 'Peak workspace name %s must be a string.' \
                                              'but not %s.' % (str(peak_ws_name),
                                                               str(type(peak_ws_name)))
        assert AnalysisDataService.doesExist(peak_ws_name), 'Peak workspace %s does not' \
                                                            'exist.' % peak_ws_name

        # set
        self._isCurrentUserHKL = True
        self._myExpNumber = exp_number
        self._myScanNumber = scan_number
        self._myPeakWorkspaceName = peak_ws_name

        #
        self._myDataMDWorkspaceName = None

        # Define class variable
        # HKL list
        self._userHKL = None    # user specified HKL
        self._spiceHKL = None                        # spice HKL
        self._prevHKL = numpy.array([0., 0., 0.])    # previous HKL

        # peak center and PeaksWorkspace
        self._avgPeakCenter = None
        self._myPeakWSKey = (None, None, None)
        self._myPeakIndex = None

        self._myLastPeakUB = None

        self._myIntensity = None
        self._gaussIntensity = 0.
        self._gaussStdDev = 0.
        self._lorenzFactor = None

        # peak integration result
        self._integrationDict = None
        self._ptIntensityDict = None

        # some motor/goniometer information for further correction
        self._movingMotorTuple = None

        # Figure print
        self._fingerPrint = '{0:.7f}.{1}'.format(time.time(), random.randint(0, 10000000))

        print '[DB...BAT] Create PeakProcessRecord for Exp {0} Scan {1} ({2} | {3}).'.format(self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self)))
        return

    def calculate_peak_center(self):
        """ Calculate peak's center by averaging the peaks found and stored in PeakWorkspace
        :return:
        """
        # Go through the peak workspaces to calculate peak center with weight (monitor and counts)
        peak_ws = AnalysisDataService.retrieve(self._myPeakWorkspaceName)

        # spice table workspace
        spice_table_name = get_spice_table_name(self._myExpNumber, self._myScanNumber)
        spice_table_ws = AnalysisDataService.retrieve(spice_table_name)

        pt_spice_row_dict = build_pt_spice_table_row_map(spice_table_ws)
        det_col_index = spice_table_ws.getColumnNames().index('detector')
        monitor_col_index = spice_table_ws.getColumnNames().index('monitor')

        num_found_peaks = peak_ws.rowCount()

        q_sample_sum = numpy.array([0., 0., 0.])
        weight_sum = 0.

        for i_peak in xrange(num_found_peaks):
            # get peak
            peak_i = peak_ws.getPeak(i_peak)
            run_number = peak_i.getRunNumber()
            # get Pt. number
            pt_number = run_number % self._myScanNumber
            # get row number and then detector counts and monitor counts
            if pt_number not in pt_spice_row_dict:
                # skip
                print '[Error] Scan %d Peak %d Pt %d cannot be located.' % (self._myScanNumber, i_peak, pt_number)
                continue

            row_index = pt_spice_row_dict[pt_number]
            det_counts = spice_table_ws.cell(row_index, det_col_index)
            monitor_counts = spice_table_ws.cell(row_index, monitor_col_index)
            if monitor_counts < 1.:
                # skip zero-count
                continue
            # convert q sample from V3D to ndarray
            q_i = peak_i.getQSampleFrame()
            q_array = numpy.array([q_i.X(), q_i.Y(), q_i.Z()])
            # calculate weight
            weight_i = float(det_counts)/float(monitor_counts)
            # contribute to total
            weight_sum += weight_i
            q_sample_sum += q_array * weight_i
            # set non-normalized peak intensity as detector counts (roughly)
            peak_i.setIntensity(det_counts)
        # END-FOR (i_peak)

        self._avgPeakCenter = q_sample_sum/weight_sum

        return

    def generate_report(self):
        """

        :return:
        """
        # TODO/FIXME/NOW/ISSUE - Implement ASAP
        """
        scan_number_list = sorted(peak_integration_summary.keys())
        for scan_number in scan_number_list:
            spice_hkl = peak_integration_summary['SPICE HKL']
            calculated_hkl = peak_integration_summary['Mantid HKL']
            mask_name = peak_integration_summary['Mask']
            intensity1 = peak_integration_summary['Raw Intensity']
            error1 = peak_integration_summary['Raw Intensity Error']
            intensity2 = peak_integration_summary['Intensity 2']
            error2 = peak_integration_summary['Intensity 2 Error']
            intensity3 = peak_integration_summary['Gauss Intensity']
            error3 = peak_integration_summary['Gauss Intensity Error']
            lorentz_factor = peak_integration_summary['Lorentz']
            estimated_bkgd = peak_integration_summary['Estimated Background']
            gauss_bkgd = peak_integration_summary['Fitted Background']
            gauss_a = peak_integration_summary['Fitted A']
            gauss_sigma = peak_integration_summary['Fitted Sigma']
            motor_name = peak_integration_summary['Motor']
            motor_step = peak_integration_summary['Motor Step']
            k_shift = peak_integration_summary['K-vector']
            absorption_correction = peak_integration_summary['Absorption Correction']
        """

        return dict()

    def get_intensity(self, algorithm_type, lorentz_corrected):
        """
        get the integrated intensity with specified integration algorithm and whether
        the result should be corrected by Lorentz correction factor
        :param algorithm_type:
        :param lorentz_corrected:
        :return:
        """
        # check
        if self._integrationDict is None and self._myIntensity is None:
            raise RuntimeError('PeakInfo of Exp {0} Scan {1} ({2} | {3}) has not integrated setup.'
                               ''.format(self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self))))
        elif self._myIntensity is not None:
            # return ZERO intensity due to previously found error
            return self._myIntensity, 0.

        try:
            if algorithm_type == 0 or algorithm_type.startswith('simple'):
                # simple
                intensity = self._integrationDict['simple intensity']
                std_dev = self._integrationDict['simple error']
            elif algorithm_type == 1 or algorithm_type.count('mixed') > 0:
                # intensity 2: mixed simple and gaussian
                intensity = self._integrationDict['intensity 2']
                std_dev = self._integrationDict['error 2']
            elif algorithm_type == 2 or algorithm_type.count('gauss') > 0:
                # gaussian
                intensity = self._integrationDict['gauss intensity']
                std_dev = self._integrationDict['gauss error']
            else:
                raise RuntimeError('Type {0} not supported yet.')
        except KeyError as key_err:
            err_msg = 'Some key(s) does not exist in dictionary with keys {0}. FYI: {1}' \
                      ''.format(self._integrationDict.keys(), key_err)
            raise RuntimeError(err_msg)

        if intensity is None:
            intensity = 0.
            std_dev = 0.
        elif lorentz_corrected:
            intensity *= self._lorenzFactor
            std_dev *= self._lorenzFactor

        return intensity, std_dev

    def get_peak_centre(self):
        """ get weighted peak centre
        :return: Qx, Qy, Qz (3-double-tuple)
        """
        assert isinstance(self._avgPeakCenter, numpy.ndarray)
        return self._avgPeakCenter[0], self._avgPeakCenter[1], self._avgPeakCenter[2]

    def get_peak_centre_v3d(self):
        """ Returned the statistically averaged peak center in V3D
        :return:
        """
        q_x, q_y, q_z = self.get_peak_centre()
        q_3d = V3D(q_x, q_y, q_z)

        return q_3d

    def get_peak_workspace(self):
        """
        Get peak workspace related
        :return:
        """
        peak_ws = AnalysisDataService.retrieve(self._myPeakWorkspaceName)
        assert peak_ws
        return peak_ws

    def get_hkl(self, user_hkl):
        """ Get HKL from the peak process record
        :param user_hkl: if selected, then return the HKL set from client (GUI). Otherwise, HKL is retrieved
                        from original SPICE file.
        :return:
        """
        if user_hkl:
            # return user-specified HKL
            assert self._userHKL is not None, 'User HKL is None (not set up yet)'
            ret_hkl = self._userHKL
        else:
            # get HKL from SPICE file
            # if self._spiceHKL is None:
            self.retrieve_hkl_from_spice_table()
            ret_hkl = self._spiceHKL

        return ret_hkl

    def get_experiment_info(self):
        """

        :return: 2-tuple of integer as experiment number
        """
        return self._myExpNumber, self._myScanNumber

    def get_sample_frame_q(self, peak_index):
        """
        Get Q in sample frame
        :return: 3-tuple of floats as Qx, Qy, Qz
        """
        peak_ws = AnalysisDataService.retrieve(self._myPeakWorkspaceName)
        peak = peak_ws.getPeak(peak_index)
        q_sample = peak.getQSampleFrame()

        return q_sample.getX(), q_sample.getY(), q_sample.getZ()

    def get_weighted_peak_centres(self):
        """ Get the peak centers found in peak workspace.
        Guarantees: the peak centers and its weight (detector counts) are exported
        :return: 2-tuple: list of 3-tuple (Qx, Qy, Qz)
                          list of double (Det_Counts)
        """
        # get PeaksWorkspace
        if AnalysisDataService.doesExist(self._myPeakWorkspaceName) is False:
            raise RuntimeError('PeaksWorkspace %s does ot exit.' % self._myPeakWorkspaceName)

        peak_ws = AnalysisDataService.retrieve(self._myPeakWorkspaceName)

        # get peak center, peak intensity and etc.
        peak_center_list = list()
        peak_intensity_list = list()
        num_peaks = peak_ws.getNumberPeaks()
        for i_peak in xrange(num_peaks):
            peak_i = peak_ws.getPeak(i_peak)
            center_i = peak_i.getQSampleFrame()
            intensity_i = peak_i.getIntensity()
            peak_center_list.append((center_i.X(), center_i.Y(), center_i.Z()))
            peak_intensity_list.append(intensity_i)
        # END-FOR

        return peak_center_list, peak_intensity_list

    @property
    def lorentz_correction_factor(self):
        """

        :return:
        """
        if self._lorenzFactor is None:
            raise RuntimeError('Lorentz factor has not been calculated for Exp {0} Scan {1} ({2} | {3}).'
                               ''.format(self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self))))
        return self._lorenzFactor

    @lorentz_correction_factor.setter
    def lorentz_correction_factor(self, factor):
        """
        get lorenz factor
        :param factor:
        :return:
        """
        assert isinstance(factor, float), 'Lorentz correction factor'
        self._lorenzFactor = factor

        print '[DB...BAT] Exp {0} Scan {1}  ({2} | {3}) has Lorentz factor set up.' \
              ''.format(self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self)))

        return

    @property
    def md_workspace(self):
        """
        give out MDEventWorkspace name for merged scan
        :return:
        """
        return self._myDataMDWorkspaceName

    @property
    def peaks_workspace(self):
        """
        give out PeaksWorkspace
        :return:
        """
        return self._myPeakWorkspaceName

    def retrieve_hkl_from_spice_table(self):
        """ Get averaged HKL from SPICE table
        HKL will be averaged from SPICE table by assuming the value in SPICE might be right
        :return:
        """
        # get SPICE table
        spice_table_name = get_spice_table_name(self._myExpNumber, self._myScanNumber)
        assert AnalysisDataService.doesExist(spice_table_name), 'Spice table for Exp %d Scan %d cannot be found.' \
                                                                '' % (self._myExpNumber, self._myScanNumber)

        spice_table_ws = AnalysisDataService.retrieve(spice_table_name)

        # get HKL column indexes
        h_col_index = spice_table_ws.getColumnNames().index('h')
        k_col_index = spice_table_ws.getColumnNames().index('k')
        l_col_index = spice_table_ws.getColumnNames().index('l')

        # scan each Pt.
        hkl = numpy.array([0., 0., 0.])

        num_rows = spice_table_ws.rowCount()
        for row_index in xrange(num_rows):
            mi_h = spice_table_ws.cell(row_index, h_col_index)
            mi_k = spice_table_ws.cell(row_index, k_col_index)
            mi_l = spice_table_ws.cell(row_index, l_col_index)
            hkl += numpy.array([mi_h, mi_k, mi_l])
        # END-FOR

        self._spiceHKL = hkl/num_rows

        return

    def set_data_ws_name(self, md_ws_name):
        """ Set the name of MDEventWorkspace with merged Pts.
        :param md_ws_name:
        :return:
        """
        assert isinstance(md_ws_name, str)
        assert AnalysisDataService.doesExist(md_ws_name)

        self._myDataMDWorkspaceName = md_ws_name

        return

    def set_hkl_np_array(self, hkl):
        """ Set current HKL which may come from any source, such as user, spice or calculation
        :param hkl: 3-item-list or 3-tuple for HKL
        :return:
        """
        # check
        assert isinstance(hkl, numpy.ndarray), 'HKL must be a numpy array but not %s.' % type(hkl)
        assert hkl.shape == (3,), 'HKL must be a 3-element 1-D array but not %s.' % str(hkl.shape)

        # store the HKL
        if self._userHKL is not None:
            self._prevHKL = self._userHKL[:]
        self._userHKL = hkl

        return

    def set_hkl(self, mi_h, mi_k, mi_l):
        """
        Set HKL to this peak Info
        :param mi_h:
        :param mi_k:
        :param mi_l:
        :return:
        """
        assert isinstance(mi_h, float) or isinstance(mi_h, int), 'h must be a float or integer but not %s.' % type(mi_h)
        assert isinstance(mi_k, float)
        assert isinstance(mi_l, float)

        if isinstance(mi_h, int):
            mi_h = float(mi_h)
            mi_k = float(mi_k)
            mi_l = float(mi_l)
        # END-IF

        if self._userHKL is None:
            # init HKL
            self._userHKL = numpy.ndarray(shape=(3,), dtype='float')
        else:
            # save previous HKL
            self._prevHKL = self._userHKL[:]

        # set current
        self._userHKL[0] = mi_h
        self._userHKL[1] = mi_k
        self._userHKL[2] = mi_l

        return

    def set_motor(self, motor_name, motor_step, motor_std_dev):
        """
        set motor step information
        :param motor_name:
        :param motor_step:
        :param motor_std_dev:
        :return:
        """
        assert isinstance(motor_name, str), 'Motor name {0} must be a string but not {1}.' \
                                            ''.format(motor_name, type(motor_name))
        assert isinstance(motor_step, float), 'Motor float {0} must be a string but not {1}.' \
                                              ''.format(motor_step, type(motor_step))
        assert isinstance(motor_std_dev, float), 'Standard deviation type must be float'

        self._movingMotorTuple = (motor_name, motor_step, motor_std_dev)

        return

    def set_integration(self, peak_integration_dict):
        """
        set the integration result by information stored in a dictionary
        :param peak_integration_dict:
        :return:
        """
        assert isinstance(peak_integration_dict, dict),\
            'Integrated peak information {0} must be given by a dictionary but not a {1}.' \
            ''.format(peak_integration_dict, type(peak_integration_dict))

        print '[DB...BAT] Exp {0} Scan {1}  ({2} | {3}) has integrated dictionary set up.' \
              ''.format(self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self)))

        self._integrationDict = peak_integration_dict

        return

    def set_intensity_to_zero(self):
        """
        if peak integration is wrong, then set the intensity to zero
        :return:
        """
        self._myIntensity = -0.

        return

    def set_pt_intensity(self, pt_intensity_dict):
        """
        Set Pt. intensity
        :param pt_intensity_dict:
        :return:
        """
        assert isinstance(pt_intensity_dict, dict)

        self._ptIntensityDict = pt_intensity_dict

        return


def build_pt_spice_table_row_map(spice_table_ws):
    """
    Build a lookup dictionary for Pt number and row number
    :param spice_table_ws:
    :return:
    """
    pt_spice_row_dict = dict()
    num_rows = spice_table_ws.rowCount()
    pt_col_index = spice_table_ws.getColumnNames().index('Pt.')

    for i_row in xrange(num_rows):
        pt_number = int(spice_table_ws.cell(i_row, pt_col_index))
        pt_spice_row_dict[pt_number] = i_row

    return pt_spice_row_dict
