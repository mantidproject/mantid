# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=W0403,R0902
import numpy
import time
import random
from mantidqtinterfaces.HFIR_4Circle_Reduction.fourcircle_utility import *
from mantid.api import AnalysisDataService
from mantid.kernel import V3D

__author__ = "wzz"


class PeakProcessRecord(object):
    """Class containing a peak's information for GUI
    In order to manage some operations for a peak
    It does not contain peak workspace but will hold all the parameters about peak integration
    """

    def __init__(self, exp_number, scan_number, peak_ws_name, two_theta):
        """Initialization
        Purpose: set up unchanged parameters including experiment number, scan number and peak workspace's name
        """
        # check
        assert isinstance(exp_number, int) and isinstance(scan_number, int)
        assert isinstance(peak_ws_name, str), "Peak workspace name %s must be a string. but not %s." % (
            str(peak_ws_name),
            str(type(peak_ws_name)),
        )
        assert AnalysisDataService.doesExist(peak_ws_name), "Peak workspace %s does not exist." % peak_ws_name

        # set
        self._myExpNumber = exp_number
        self._myScanNumber = scan_number
        self._myPeakWorkspaceName = peak_ws_name

        # related detector information
        self._2theta = two_theta

        #
        self._myDataMDWorkspaceName = None

        # Define class variable
        # HKL list
        self._calculatedHKL = None  # user specified HKL
        self._spiceHKL = None  # spice HKL
        self._prevHKL = numpy.array([0.0, 0.0, 0.0])  # previous HKL

        # magnetic peak set up
        self._kShiftVector = [0, 0, 0]
        self._absorptionCorrection = 1.0

        # peak center and PeaksWorkspace
        self._avgPeakCenter = None
        self._myPeakWSKey = (None, None, None)
        self._myPeakIndex = None

        self._myLastPeakUB = None

        self._myIntensity = None
        self._gaussIntensity = 0.0
        self._gaussStdDev = 0.0
        self._lorenzFactor = None

        # Gaussian fitting related
        self._gaussFWHM = None
        self._peakMotorCenter = None
        self._gaussBackground = None

        # peak integration result: all the fitted parameters such as Sigma are in this dictionary
        self._integrationDict = None
        # pt-based Gaussian integration result dictionary.
        # details can be found in peak_integration_utility.integrate_peak_full_version
        self._gaussIntegrationInfoDict = None

        # some motor/goniometer information for further correction
        self._movingMotorTuple = None

        # Figure print
        self._fingerPrint = "{0:.7f}.{1}".format(time.time(), random.randint(0, 10000000))

        return

    def calculate_peak_center(self, allow_bad_monitor=True):
        """Calculate peak's center by averaging the peaks found and stored in PeakWorkspace
        :param allow_bad_monitor: if specified as True, then a bad monitor (zero) is allowed and set the value to 1.
        :return: str (error message)
        """
        # Go through the peak workspaces to calculate peak center with weight (monitor and counts)
        peak_ws = AnalysisDataService.retrieve(self._myPeakWorkspaceName)

        # spice table workspace
        spice_table_name = get_spice_table_name(self._myExpNumber, self._myScanNumber)
        spice_table_ws = AnalysisDataService.retrieve(spice_table_name)

        pt_spice_row_dict = build_pt_spice_table_row_map(spice_table_ws)
        det_col_index = spice_table_ws.getColumnNames().index("detector")
        monitor_col_index = spice_table_ws.getColumnNames().index("monitor")

        num_found_peaks = peak_ws.rowCount()

        q_sample_sum = numpy.array([0.0, 0.0, 0.0])
        weight_sum = 0.0

        err_msg = ""
        for i_peak in range(num_found_peaks):
            # get peak
            peak_i = peak_ws.getPeak(i_peak)
            run_number = peak_i.getRunNumber()
            # get Pt. number
            pt_number = run_number % self._myScanNumber
            # get row number and then detector counts and monitor counts
            if pt_number not in pt_spice_row_dict:
                # skip
                err_msg += "\nScan %d Peak %d Pt %d cannot be located." % (self._myScanNumber, i_peak, pt_number)
                continue

            row_index = pt_spice_row_dict[pt_number]
            det_counts = spice_table_ws.cell(row_index, det_col_index)
            monitor_counts = spice_table_ws.cell(row_index, monitor_col_index)
            if monitor_counts < 1.0:
                # bad monitor counts
                if allow_bad_monitor:
                    monitor_counts = 1
                else:
                    continue
            # convert q sample from V3D to ndarray
            q_i = peak_i.getQSampleFrame()
            q_array = numpy.array([q_i.X(), q_i.Y(), q_i.Z()])
            # calculate weight
            weight_i = float(det_counts) / float(monitor_counts)
            # contribute to total
            weight_sum += weight_i
            q_sample_sum += q_array * weight_i
            # set non-normalized peak intensity as detector counts (roughly)
            peak_i.setIntensity(det_counts)
        # END-FOR (i_peak)

        try:
            self._avgPeakCenter = q_sample_sum / weight_sum
        except Exception as e:
            raise RuntimeError("Unable to calculate average peak center due to value error as {0}.".format(e))

        return err_msg

    @property
    def gaussian_fwhm(self):
        """
        return the gaussian FWHM
        :return:
        """
        return self._gaussFWHM

    def generate_integration_report(self):
        """
        generate a dictionary for this PeakInfo
        :return:
        """
        print("[DB..BAT] generate_integration_report is called!")

        report = dict()

        if self._spiceHKL is not None:
            report["SPICE HKL"] = str_format(self._spiceHKL)
        else:
            report["SPICE HKL"] = ""
        if self._calculatedHKL is not None:
            report["Mantid HKL"] = str_format(self._calculatedHKL)
        else:
            report["Mantid HKL"] = None
        if self._integrationDict:
            report["Mask"] = self._integrationDict["mask"]
            report["Raw Intensity"] = self._integrationDict["simple intensity"]
            report["Raw Intensity Error"] = self._integrationDict["simple error"]
            report["Intensity 2"] = self._integrationDict["intensity 2"]
            report["Intensity 2 Error"] = self._integrationDict["error 2"]
            report["Gauss Intensity"] = self._integrationDict["gauss intensity"]
            report["Gauss Error"] = self._integrationDict["gauss error"]
            report["Estimated Background"] = self._integrationDict["simple background"]
            if "gauss parameters" in self._integrationDict:
                report["Fitted Background"] = self._integrationDict["gauss parameters"]["B"]
                report["Fitted A"] = self._integrationDict["gauss parameters"]["A"]
                report["Fitted Sigma"] = self._integrationDict["gauss parameters"]["s"]
            else:
                report["Fitted Background"] = ""
                report["Fitted A"] = ""
                report["Fitted Sigma"] = ""
        else:
            report["Raw Intensity"] = ""
            report["Raw Intensity Error"] = ""
            report["Intensity 2"] = ""
            report["Intensity 2 Error"] = ""
            report["Gauss Intensity"] = ""
            report["Gauss Error"] = ""
            report["Lorentz"] = ""
            report["Estimated Background"] = ""
            report["Fitted Background"] = ""
            report["Fitted A"] = ""
            report["Fitted Sigma"] = ""
            report["Mask"] = ""

        report["Lorentz"] = self._lorenzFactor
        if self._movingMotorTuple is None:
            report["Motor"] = ""
            report["Motor Step"] = None
        else:
            report["Motor"] = self._movingMotorTuple[0]
            report["Motor Step"] = self._movingMotorTuple[1]
        report["K-vector"] = self._kShiftVector
        report["Absorption Correction"] = self._absorptionCorrection

        if self._gaussIntegrationInfoDict:
            print("[FLAG-SigmaError] {0}  {1}".format(self._myScanNumber, self._gaussIntegrationInfoDict["gauss errors"]["s"]))

        return report

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
            raise RuntimeError(
                "PeakInfo of Exp {0} Scan {1} ({2} | {3}) has not integrated setup.".format(
                    self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self))
                )
            )
        elif self._myIntensity is not None:
            # return ZERO intensity due to previously found error
            return self._myIntensity, 0.0

        try:
            if algorithm_type == 0 or algorithm_type.startswith("simple"):
                # simple
                intensity = self._integrationDict["simple intensity"]
                std_dev = self._integrationDict["simple error"]
            elif algorithm_type == 1 or algorithm_type.count("mixed") > 0:
                # intensity 2: mixed simple and gaussian
                intensity = self._integrationDict["intensity 2"]
                std_dev = self._integrationDict["error 2"]
            elif algorithm_type == 2 or algorithm_type.count("gauss") > 0:
                # gaussian
                intensity = self._integrationDict["gauss intensity"]
                std_dev = self._integrationDict["gauss error"]
            else:
                raise RuntimeError("Type {0} not supported yet.")
        except KeyError as key_err:
            err_msg = "Some key(s) does not exist in dictionary with keys {0}. FYI: {1}".format(self._integrationDict.keys(), key_err)
            raise RuntimeError(err_msg)

        if intensity is None:
            intensity = 0.0
            std_dev = 0.0
        elif lorentz_corrected:
            intensity *= self._lorenzFactor
            std_dev *= self._lorenzFactor

        return intensity, std_dev

    def get_parameter(self, par_name):
        """
        get some parameters for peak fitting or etc
        :param par_name:
        :return:
        """
        # TODO (future): Allow for more parameters
        if par_name == "2theta":
            par_value = self._2theta
            par_error = 0
        elif par_name == "sigma":
            par_value = self._integrationDict["gauss parameters"]["s"]
            par_error = self._gaussIntegrationInfoDict["gauss errors"]["s"]
        else:
            raise RuntimeError("Parameter {0} is not set up for get_parameter()".format(par_name))

        return par_value, par_error

    def get_peak_centre(self):
        """get weighted peak centre
        :return: Qx, Qy, Qz (3-double-tuple)
        """
        assert isinstance(self._avgPeakCenter, numpy.ndarray)
        return self._avgPeakCenter[0], self._avgPeakCenter[1], self._avgPeakCenter[2]

    def get_peak_centre_v3d(self):
        """Returned the statistically averaged peak center in V3D
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

    def get_integration_gauss_fit_params(self):
        """
        get the parameters of the Gaussian fit on 3D scan peak integration
        :return:
        """
        return self._gaussIntegrationInfoDict

    def get_hkl(self, user_hkl):
        """Get HKL from the peak process record
        :param user_hkl: if selected, then return the HKL set from client (GUI). Otherwise, HKL is retrieved
                        from original SPICE file.
        :return:
        """
        if user_hkl:
            # return user-specified HKL
            assert self._calculatedHKL is not None, "User HKL is None (not set up yet)"
            ret_hkl = self._calculatedHKL
        else:
            # get HKL from SPICE file
            # if self._spiceHKL is None:
            self.retrieve_hkl_from_spice_table()
            ret_hkl = self._spiceHKL
        # END-IF-ELSE

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
        """Get the peak centers found in peak workspace.
        Guarantees: the peak centers and its weight (detector counts) are exported
        :return: 2-tuple: list of 3-tuple (Qx, Qy, Qz)
                          list of double (Det_Counts)
        """
        # get PeaksWorkspace
        if AnalysisDataService.doesExist(self._myPeakWorkspaceName) is False:
            raise RuntimeError("PeaksWorkspace %s does not exist." % self._myPeakWorkspaceName)

        peak_ws = AnalysisDataService.retrieve(self._myPeakWorkspaceName)

        # get peak center, peak intensity and etc.
        peak_center_list = list()
        peak_intensity_list = list()
        num_peaks = peak_ws.getNumberPeaks()
        for i_peak in range(num_peaks):
            peak_i = peak_ws.getPeak(i_peak)
            center_i = peak_i.getQSampleFrame()
            intensity_i = peak_i.getIntensity()
            peak_center_list.append((center_i.X(), center_i.Y(), center_i.Z()))
            peak_intensity_list.append(intensity_i)
        # END-FOR

        return peak_center_list, peak_intensity_list

    def set_k_vector(self, k_vector):
        """

        :param k_vector:
        :return:
        """
        # check input
        assert not isinstance(k_vector, str) and len(k_vector) == 3, "K-vector {0} must have 3 items.".format(k_vector)

        self._kShiftVector = k_vector[:]

        return

    @property
    def lorentz_correction_factor(self):
        """

        :return:
        """
        if self._lorenzFactor is None:
            raise RuntimeError(
                "Lorentz factor has not been calculated for Exp {0} Scan {1} ({2} | {3}).".format(
                    self._myExpNumber, self._myScanNumber, self._fingerPrint, hex(id(self))
                )
            )
        return self._lorenzFactor

    @lorentz_correction_factor.setter
    def lorentz_correction_factor(self, factor):
        """
        get lorenz factor
        :param factor:
        :return:
        """
        assert isinstance(factor, float), "Lorentz correction factor"
        self._lorenzFactor = factor

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
        """Get averaged HKL from SPICE table
        HKL will be averaged from SPICE table by assuming the value in SPICE might be right
        :return:
        """
        # get SPICE table
        spice_table_name = get_spice_table_name(self._myExpNumber, self._myScanNumber)
        assert AnalysisDataService.doesExist(spice_table_name), "Spice table for Exp %d Scan %d cannot be found." % (
            self._myExpNumber,
            self._myScanNumber,
        )

        spice_table_ws = AnalysisDataService.retrieve(spice_table_name)

        # get HKL column indexes
        h_col_index = spice_table_ws.getColumnNames().index("h")
        k_col_index = spice_table_ws.getColumnNames().index("k")
        l_col_index = spice_table_ws.getColumnNames().index("l")

        # scan each Pt.
        hkl = numpy.array([0.0, 0.0, 0.0])

        num_rows = spice_table_ws.rowCount()
        for row_index in range(num_rows):
            mi_h = spice_table_ws.cell(row_index, h_col_index)
            mi_k = spice_table_ws.cell(row_index, k_col_index)
            mi_l = spice_table_ws.cell(row_index, l_col_index)
            hkl += numpy.array([mi_h, mi_k, mi_l])
        # END-FOR

        self._spiceHKL = hkl / num_rows

        return

    def set_absorption_factor(self, abs_factor):
        """
        set absorption correction factor
        :return:
        """
        # check
        assert isinstance(abs_factor, float) or isinstance(
            abs_factor, int
        ), "Absorption correction {0} must be an integer but not {1}.".format(abs_factor, type(abs_factor))

        self._absorptionCorrection = abs_factor

        return

    def set_data_ws_name(self, md_ws_name):
        """Set the name of MDEventWorkspace with merged Pts.
        :param md_ws_name:
        :return:
        """
        assert isinstance(md_ws_name, str)
        assert AnalysisDataService.doesExist(md_ws_name)

        self._myDataMDWorkspaceName = md_ws_name

        return

    def set_gaussian_fit_params(self, intensity, fwhm, position, background, intensity_error=None):
        """
        set Gaussian fit parameters
        :param intensity:
        :param fwhm:
        :param position:
        :param background: list as [A0, A1, ...]
        :return:
        """
        # check inputs
        assert isinstance(intensity, float), "Intensity {0} must be a float but not a {1}".format(intensity, type(intensity))
        assert isinstance(fwhm, float), "Peak FWHM {0} must be a float but not a {1}".format(fwhm, type(fwhm))
        assert isinstance(position, float), "Peak center {0} must be a float but not a {1}".format(position, type(position))
        assert isinstance(background, list), "Background {0} must be given as a list, i.e., [A0, A1, ...]".format(background)

        # set value
        self._gaussIntensity = intensity
        self._gaussStdDev = intensity_error
        self._gaussFWHM = fwhm
        self._peakMotorCenter = position
        self._gaussBackground = background

        return

    def set_hkl_np_array(self, hkl):
        """Set current HKL which may come from any source, such as user, spice or calculation
        :param hkl: 3-item-list or 3-tuple for HKL
        :return:
        """
        # check
        assert isinstance(hkl, numpy.ndarray), "HKL must be a numpy array but not %s." % type(hkl)
        assert hkl.shape == (3,), "HKL must be a 3-element 1-D array but not %s." % str(hkl.shape)

        # store the HKL
        if self._calculatedHKL is not None:
            self._prevHKL = self._calculatedHKL[:]
        self._calculatedHKL = hkl

        return

    def set_hkl(self, mi_h, mi_k, mi_l):
        """
        Set HKL to this peak Info
        :param mi_h:
        :param mi_k:
        :param mi_l:
        :return:
        """
        assert isinstance(mi_h, float) or isinstance(mi_h, int), "h must be a float or integer but not %s." % type(mi_h)
        assert isinstance(mi_k, float)
        assert isinstance(mi_l, float)

        if isinstance(mi_h, int):
            mi_h = float(mi_h)
            mi_k = float(mi_k)
            mi_l = float(mi_l)
        # END-IF

        if self._calculatedHKL is None:
            # init HKL
            self._calculatedHKL = numpy.ndarray(shape=(3,), dtype="float")
        else:
            # save previous HKL
            self._prevHKL = self._calculatedHKL[:]

        # set current
        self._calculatedHKL[0] = mi_h
        self._calculatedHKL[1] = mi_k
        self._calculatedHKL[2] = mi_l

        return

    def set_motor(self, motor_name, motor_step, motor_std_dev):
        """
        set motor step information
        :param motor_name:
        :param motor_step:
        :param motor_std_dev:
        :return:
        """
        assert isinstance(motor_name, str), "Motor name {0} must be a string but not {1}.".format(motor_name, type(motor_name))
        assert isinstance(motor_step, float), "Motor float {0} must be a string but not {1}.".format(motor_step, type(motor_step))
        assert isinstance(motor_std_dev, float), "Standard deviation type must be float"

        self._movingMotorTuple = (motor_name, motor_step, motor_std_dev)

        return

    def set_integration(self, peak_integration_dict):
        """
        set the integration result by information stored in a dictionary
        :param peak_integration_dict:
        :return:
        """
        assert isinstance(
            peak_integration_dict, dict
        ), "Integrated peak information {0} must be given by a dictionary but not a {1}.".format(
            peak_integration_dict, type(peak_integration_dict)
        )

        self._integrationDict = peak_integration_dict

        return

    def set_intensity_to_zero(self):
        """
        if peak integration is wrong, then set the intensity to zero
        :return:
        """
        self._myIntensity = -0.0

        return

    def set_pt_intensity(self, pt_intensity_dict):
        """
        Set Pt. intensity
        :param pt_intensity_dict:
        :return:
        """
        assert isinstance(pt_intensity_dict, dict)

        self._gaussIntegrationInfoDict = pt_intensity_dict

        return


class SinglePointPeakIntegration(object):
    """
    simple class to store the result of ONE and ONLY ONE single point measurement peak integration
    """

    def __init__(self, exp_number, scan_number, roi_name, pt_number, two_theta):
        """
        initialization
        :param exp_number:
        :param scan_number:
        :param roi_name:
        :param pt_number:
        """
        # check inputs
        check_integer("Experiment number", exp_number)
        check_integer("Scan number", scan_number)
        check_integer("Pt number", pt_number)
        check_string("ROI name", roi_name)
        check_float("Two theta", two_theta)

        self._exp_number = exp_number
        self._scan_number = scan_number
        self._roi_name = roi_name
        self._pt_number = pt_number
        self._two_theta = two_theta

        self._integral_direction = None

        self._pt_intensity = None
        self._peak_intensity = None

        self._vec_x = None
        self._vec_y = None

        self._gauss_x0 = None
        self._gauss_sigma = None
        self._flat_b = None
        self._a1 = None  # background first order

        # reference peak width
        self._ref_peak_sigma = None

        # fitting cost (goodness)
        self._fit_cost = None

        self._spiceHKL = None

        return

    # TODO FIXME NOW3 - Need to set up all the Gaussian/Background parameters
    def get_gaussian_parameters(self):
        """
        get the Gaussian
        :return:
        """
        return self._gauss_x0, self._gauss_sigma, self._pt_intensity, self._flat_b

    def get_hkl(self, user_hkl):
        """Get HKL (originally from SPICE)
        :param user_hkl: if selected, then return the HKL set from client (GUI). Otherwise, HKL is retrieved
                        from original SPICE file.
        :return:
        """
        # get HKL from SPICE file
        # if self._spiceHKL is None:
        self.retrieve_hkl_from_spice_table()
        ret_hkl = self._spiceHKL
        # END-IF-ELSE

        return ret_hkl

    def get_pt_intensity(self):
        """
        get single-pt-can intensity
        :return:
        """
        return self._pt_intensity

    def get_intensity(self, algorithm_type, lorentz_corrected):
        """
        get the integrated intensity with specified integration algorithm and whether
        the result should be corrected by Lorentz correction factor
        :param algorithm_type:
        :param lorentz_corrected:
        :return:
        """
        # check
        if self._pt_intensity is None:
            raise RuntimeError(
                "SinglePtPeakInfo of Exp {0} Scan {1} ({2} | {3}) has not integrated setup.".format(
                    self._exp_number, self._scan_number, self._roi_name, hex(id(self))
                )
            )

        # get intensity
        intensity = self._pt_intensity
        std_dev = numpy.sqrt(intensity)

        if lorentz_corrected:
            # use the instrument 2theta: L = sin(2theta)
            lorentz_factor = numpy.sin(self._two_theta * numpy.pi / 180.0)

            intensity *= lorentz_factor
            std_dev *= lorentz_factor

        return intensity, std_dev

    def get_vec_x_y(self):
        """
        get single Pt processed intensity
        :return:
        """
        return self._vec_x, self._vec_y

    def has_fit_result(self):
        """
        check whether this single pt scan has Gassian function fit for the summed counts
        :return:
        """

    # TODO NOW3 Code Quality: this has a duplicate in the same file!
    def retrieve_hkl_from_spice_table(self):
        """Get averaged HKL from SPICE table
        HKL will be averaged from SPICE table by assuming the value in SPICE might be right
        :return:
        """
        # get SPICE table
        spice_table_name = get_spice_table_name(self._exp_number, self._scan_number)
        assert AnalysisDataService.doesExist(spice_table_name), "Spice table for Exp %d Scan %d cannot be found." % (
            self._exp_number,
            self._scan_number,
        )

        spice_table_ws = AnalysisDataService.retrieve(spice_table_name)

        # get HKL column indexes
        h_col_index = spice_table_ws.getColumnNames().index("h")
        k_col_index = spice_table_ws.getColumnNames().index("k")
        l_col_index = spice_table_ws.getColumnNames().index("l")

        # scan each Pt.
        hkl = numpy.array([0.0, 0.0, 0.0])

        num_rows = spice_table_ws.rowCount()
        for row_index in range(num_rows):
            mi_h = spice_table_ws.cell(row_index, h_col_index)
            mi_k = spice_table_ws.cell(row_index, k_col_index)
            mi_l = spice_table_ws.cell(row_index, l_col_index)
            hkl += numpy.array([mi_h, mi_k, mi_l])
        # END-FOR

        self._spiceHKL = hkl / num_rows

        return

    def set_xy_vector(self, vec_x, vec_y, integral_direction):
        """
        set the X and Y vector
        :param vec_x:
        :param vec_y:
        :param integral_direction: integration direction
        :return:
        """
        #  check input
        assert isinstance(vec_x, numpy.ndarray), "X vector must be a numpy array"
        assert isinstance(vec_y, numpy.ndarray), "Y vector must be a numpy array"
        assert integral_direction in [
            "vertical",
            "horizontal",
        ], "Peak integration direction {} must be a string (now a {}) being either vertical or horizontal".format(
            integral_direction, type(integral_direction)
        )

        # set
        self._vec_x = vec_x
        self._vec_y = vec_y
        self._integral_direction = integral_direction

        return

    def set_fit_cost(self, cost):
        """
        set the cost (goodness) of fit
        :param cost:
        :return:
        """
        self._fit_cost = cost

        return

    def set_peak_intensity(self, peak_intensity):
        """
        set peak intensity
        :param peak_intensity:
        :return:
        """
        assert isinstance(peak_intensity, float), "Peak intensity to set must be a float."
        if peak_intensity < 0:
            raise RuntimeError("Peak intensity {0} cannot be negative!".format(peak_intensity))

        self._peak_intensity = peak_intensity

    def set_fit_params(self, x0, sigma, height, a0, a1):
        """
        set the parameters for fitting
        :param x0:
        :param sigma:
        :param a:
        :param b:
        :return:
        """
        # TODO - 2018 - clean!
        self._pt_intensity = height
        self._gauss_x0 = x0
        self._gauss_sigma = sigma
        self._flat_b = a0
        self._a1 = a1

        return

    def set_ref_fwhm(self, ref_fwhm, is_fwhm):
        """
        set reference scan's FWHM from same/similar 2theta value
        :param ref_fwhm:
        :param is_fwhm: flag whether the input is FWHM or Sigma
        :return:
        """
        check_float("Reference scan FWHM", ref_fwhm)

        self._ref_peak_sigma = ref_fwhm
        if is_fwhm:
            self._ref_peak_sigma /= 2.355

        return


# END-CLASS


class SinglePtScansIntegrationOperation(object):
    """
    a class to handle and manage Mantid Workspace2D instance created from integrated single pt-scan peaks
    along either vertical direction or horizontal direction
    """

    def __init__(self, exp_number, scan_number_list, matrix_ws_name, scan_spectrum_map, spectrum_scan_map):
        """
        initialization
        :param exp_number:
        :param scan_number_list:
        :param matrix_ws_name:
        :param scan_spectrum_map:
        :param spectrum_scan_map:
        """
        # check input
        check_integer("Experiment number", exp_number)
        check_list("Scan numbers", scan_number_list)
        check_string("Workspace2D name", matrix_ws_name)
        check_dictionary("Scan number spectrum number mapping", scan_spectrum_map)
        check_dictionary("Spectrum number scan number mapping", spectrum_scan_map)

        if AnalysisDataService.doesExist(matrix_ws_name) is False:
            raise RuntimeError("Workspace {} does not exist.".format(matrix_ws_name))

        # store
        self._exp_number = exp_number
        self._scan_number_list = scan_number_list[:]
        self._matrix_ws_name = matrix_ws_name
        self._scan_spectrum_map = scan_spectrum_map
        self._spectrum_scan_map = spectrum_scan_map

        # TODO - 20180814 - Add pt number, rio name and integration direction for future check!

        # others
        self._model_ws_name = None

        return

    def check_scan_numbers_same(self, scans_to_check):
        """
        check whether the scan numbers are same or not
        :param scans_to_check:
        :return:
        """
        check_list("Scan numbers to check with", scans_to_check)

        if len(scans_to_check) != len(self._scan_number_list):
            return False

        scans_to_check_set = set(scans_to_check)
        my_scans_set = set(self._scan_number_list)

        return scans_to_check_set == my_scans_set

    @property
    def exp_number(self):
        """
        experiment number
        :return:
        """
        return self._exp_number

    def get_model_workspace(self):
        """
        get the workspace name for calculated data (model)
        :return:
        """
        return self._model_ws_name

    def get_workspace(self):
        """get workspace name
        :return:
        """
        return self._matrix_ws_name

    def get_scan_number(self, spectrum_number, from_zero=True):
        """
        get the scan number of a spectrum
        :param spectrum_number:
        :param from_zero:
        :return:
        """
        if not from_zero:
            spectrum_number -= 1

        if spectrum_number not in self._spectrum_scan_map:
            raise RuntimeError(
                "Spectrum  number {} of type {} cannot be found in spectrum-scan map".format(spectrum_number, type(spectrum_number))
            )

        return self._spectrum_scan_map[spectrum_number]

    def get_spectrum_number(self, scan_number, from_zero=True):
        """
        get the spectrum number of a scan
        :param scan_number:
        :param from_zero:
        :return:
        """
        if scan_number not in self._scan_spectrum_map:
            raise RuntimeError("Scan  number {} of type {} cannot be found in spectrum-scan map".format(scan_number, type(scan_number)))

        spectrum_number = self._scan_spectrum_map[scan_number]

        if not from_zero:
            spectrum_number += 1

        return spectrum_number

    def set_model_workspace(self, model_ws_name):
        """
        set the workspace name for calculated peak (model) workspace from FitPeaks
        :param model_ws_name:
        :return:
        """
        self._model_ws_name = model_ws_name

        return


# END-CLASS


def build_pt_spice_table_row_map(spice_table_ws):
    """
    Build a lookup dictionary for Pt number and row number
    :param spice_table_ws:
    :return:
    """
    pt_spice_row_dict = dict()
    num_rows = spice_table_ws.rowCount()
    pt_col_index = spice_table_ws.getColumnNames().index("Pt.")

    for i_row in range(num_rows):
        pt_number = int(spice_table_ws.cell(i_row, pt_col_index))
        pt_spice_row_dict[pt_number] = i_row

    return pt_spice_row_dict


def str_format(float_items):
    """

    :param float_items:
    :return:
    """
    format_str = ""
    for index, value in enumerate(float_items):
        if index > 0:
            format_str += ", "
        if isinstance(value, float):
            format_str += "{0:.4f}".format(value)
        else:
            format_str += "{0}".format(value)
    # END-FOR

    return format_str
