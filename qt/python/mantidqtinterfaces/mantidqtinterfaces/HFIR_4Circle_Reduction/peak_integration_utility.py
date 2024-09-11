# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Utility methods to do peak integration
import numpy
import math
import os
import csv
from scipy.optimize import curve_fit
import mantid.simpleapi as mantidsimple
from mantid.api import AnalysisDataService
from mantidqtinterfaces.HFIR_4Circle_Reduction.fourcircle_utility import *


def apply_lorentz_correction(peak_intensity, q, wavelength, step_omega):
    """Apply lorentz correction to intensity"""
    # calculate theta
    sin_theta = q * wavelength / (4 * numpy.pi)
    theta = math.asin(sin_theta)
    corrected_intensity = peak_intensity * numpy.sin(2 * theta) * step_omega

    return corrected_intensity


def calculate_lorentz_correction_factor(q_sample, wavelength, motor_step):
    """
    Lorenz correction = sin(2theta)
    :param q_sample:
    :param wavelength:
    :param motor_step:
    :return:
    """
    # TODO/FIXME/NOW2 - Q sample shall be calculated from HKL and UB matrix but not from observation
    sin_theta = q_sample * wavelength / (4 * numpy.pi)
    theta = math.asin(sin_theta)
    # MOTOR step does not make sense!
    # factor = numpy.sin(2 * theta) * motor_step
    factor = numpy.sin(2 * theta)

    return factor


def calculate_motor_step(motor_pos_array, motor_step_tolerance=0.5):
    """
    calculate the motor steps
    :param motor_step_tolerance:
    :param motor_pos_array:
    :return:
    """
    assert isinstance(motor_pos_array, numpy.ndarray), "Motor positions {0} must be given as a numpy array but not " "a {1}.".format(
        motor_pos_array, type(motor_pos_array)
    )

    motor_step_vector = motor_pos_array[1:] - motor_pos_array[:-1]

    motor_step = numpy.average(motor_step_vector)
    motor_step_std = motor_step_vector.std()

    if motor_step_std > motor_step_tolerance:
        raise RuntimeError("Step deviation too large. Cannot use average!")

    return motor_step


def calculate_peak_intensity_gauss(gauss_a, gauss_sigma, error_a_sq=None, error_sigma_sq=None, error_a_sigma=None):
    """
    calculate the peak intensity, which is the area under the peak
    if sigma == 1, then the integral is sqrt(pi);
    then the value is sqrt(pi) * e^{-1/(2.*sigma**2)}
    :param gauss_a:
    :param gauss_sigma:
    :param error_a_sq: error(a)**2
    :param error_sigma_sq: error(sigma)**2
    :param error_a_sigma: correlated error for a and sigma
    :return:
    """
    integral = numpy.sqrt(2.0 * numpy.pi) * gauss_a * gauss_sigma

    if error_a_sq is not None:
        # calculate integral intensity error by propagation
        # check
        assert isinstance(error_a_sq, float), "Error(a)**2 must be a float but not a {0}.".format(type(error_a_sq))
        assert isinstance(error_sigma_sq, float), "Error(sigma)**2 must be a float but not a {0}." "".format(type(error_sigma_sq))
        assert isinstance(error_a_sigma, float), "Error(a,sigma) must be a float but not a {0}." "".format(type(error_a_sigma))
        # calculate
        error2 = gauss_a**2 * error_sigma_sq + error_a_sq * gauss_sigma**2 + 2.0 * gauss_a * gauss_sigma * error_a_sigma
        error = numpy.sqrt(error2)
    else:
        error = numpy.sqrt(integral)

    return integral, error


def calculate_penalty(model_vec_y, exp_vec_y):
    """
    calculate the penalty/cost of the model to experimental data
    say: error = 1/(N-1)sqrt(\\sum(y_i - m_i)**2)
    :param model_vec_y:
    :param exp_vec_y:
    :return:
    """
    # check inputs
    assert isinstance(model_vec_y, numpy.ndarray), "Model vec Y cannot be {0}." "".format(type(model_vec_y))
    assert isinstance(exp_vec_y, numpy.ndarray), "Experimental vec Y cannot be {0}." "".format(type(exp_vec_y))
    if model_vec_y.shape != exp_vec_y.shape or len(model_vec_y) <= 1:
        raise RuntimeError("Model and experimental data array do not match! Or too short!")

    # calculate
    diff_y = model_vec_y - exp_vec_y

    diff_y2 = numpy.ndarray(shape=(len(diff_y),), dtype="float")
    numpy.power(diff_y, 2, out=diff_y2)

    cost = numpy.sqrt(diff_y2.sum()) / (len(model_vec_y) - 1.0)

    return cost


def calculate_single_pt_scan_peak_intensity(peak_height, fwhm, is_fwhm):
    """
    calculate peak intensity for single-measurement peak (1-measurement-1-scan)
    :param peak_height:
    :param fwhm:
    :param is_fwhm:
    :return:
    """
    # check input
    assert isinstance(fwhm, float), "blabla"

    # convert fwhm
    if is_fwhm:
        sigma = fwhm / 2.355
    else:
        sigma = fwhm

    peak_intensity, no_error = calculate_peak_intensity_gauss(peak_height, sigma)

    return peak_intensity


def convert_motor_pos_intensity(integrated_pt_dict, motor_pos_dict):
    """
    :except: raise RuntimeError if
    :param integrated_pt_dict:
    :param motor_pos_dict:
    :return: motor_pos_vec, pt_intensity_vec
    """
    pt_list = sorted(integrated_pt_dict.keys())

    if len(motor_pos_dict) != len(pt_list):
        raise RuntimeError("Integrated Pt intensities does not match motor positions")

    pt_intensity_vec = numpy.ndarray(shape=(len(pt_list),), dtype="float")
    motor_pos_vec = numpy.ndarray(shape=(len(pt_list),), dtype="float")

    for i_pt, pt in enumerate(pt_list):
        pt_intensity_vec[i_pt] = integrated_pt_dict[pt]
        motor_pos_vec[i_pt] = motor_pos_dict[pt]

    return motor_pos_vec, pt_intensity_vec


def estimate_background(pt_intensity_dict, bg_pt_list):
    """
    Estimate background value by average the integrated counts of some Pt.
    :param pt_intensity_dict:
    :param bg_pt_list: Pt. for the first N and last M Pt.
    :return:
    """
    # Check
    assert isinstance(pt_intensity_dict, dict), "Peak (Pt) intensities {0} must be given by dictionary but not {1}." "".format(
        pt_intensity_dict, type(pt_intensity_dict)
    )
    assert (isinstance(bg_pt_list, tuple) or isinstance(bg_pt_list, list)) and len(
        bg_pt_list
    ) > 0, "background points {0} must be a 2-element tuple or list but not a {1}.".format(bg_pt_list, type(bg_pt_list))

    # from bg_pt_list
    bg_sum = 0.0
    background_points = list()
    pt_list = sorted(pt_intensity_dict.keys())
    left_bgs = pt_list[0 : bg_pt_list[0]]
    background_points.extend(left_bgs)
    right_bgs = pt_list[-bg_pt_list[1] :]
    background_points.extend(right_bgs)

    for bg_pt in background_points:
        assert bg_pt in pt_intensity_dict, "Pt. %d is not calculated." % bg_pt
        bg_sum += pt_intensity_dict[bg_pt]

    avg_bg = float(bg_sum) / len(bg_pt_list)

    return avg_bg


def find_gaussian_start_values_by_observation(vec_x, vec_y):
    """
    find out the starting values of a gaussian + linear background with observation
    :param vec_x:
    :param vec_y:
    :return: must be the same order as gaussian linear background function as x0, sigma, a, b
    """
    # assume that it is a quasi-ideal Gaussian
    # find out the maximum Y with x
    max_y_index = vec_y.argmax()

    x0 = vec_x[max_y_index]
    max_y = vec_y[max_y_index]
    est_background = 0.5 * (vec_y[0] + vec_y[-1])
    est_sigma = (vec_x[-1] - vec_x[0]) * 0.1
    est_a = max(1.0, max_y - est_background)

    return [x0, est_sigma, est_a, est_background]


def fit_gaussian_linear_background_mtd(matrix_ws_name):
    """
    fit Gaussian with linear background by calling Mantid's FitPeaks
    :param matrix_ws_name:
    :return: 2-tuple: dictionary for fit result (key = ws index, value = dictionary),
                      model workspace name
    """
    # check input workspace
    check_string("MatrixWorkspace name", matrix_ws_name)

    if not AnalysisDataService.doesExist(matrix_ws_name):
        raise RuntimeError("Workspace {} does not exist in Mantid ADS.".format(matrix_ws_name))
    else:
        peak_ws = AnalysisDataService.retrieve(matrix_ws_name)

    # fit peaks
    out_ws_name = "{}_fit_positions".format(matrix_ws_name)
    model_ws_name = "{}_model".format(matrix_ws_name)
    fit_param_table_name = "{}_params_table".format(matrix_ws_name)

    # estimated peak center
    vec_x = peak_ws.readX(0)
    estimated_peak_center = numpy.mean(vec_x)

    mantidsimple.FitPeaks(
        InputWorkspace=matrix_ws_name,
        OutputWorkspace=out_ws_name,
        PeakCenters=estimated_peak_center,
        FitWindowBoundaryList="{}, {}".format(vec_x[0], vec_x[-1]),
        FitFromRight=False,
        Minimizer="Levenberg-MarquardtMD",
        HighBackground=False,
        ConstrainPeakPositions=False,
        FittedPeaksWorkspace=model_ws_name,
        OutputPeakParametersWorkspace=fit_param_table_name,
    )

    # Convert the table workspace to a standard dictionary
    fit_param_dict = convert_fit_parameter_table_to_dict(fit_param_table_name)

    return fit_param_dict, model_ws_name


def convert_fit_parameter_table_to_dict(table_name):
    """

    :param table_name:
    :return:
    """
    # table workspace
    assert isinstance(table_name, str), "blabla"
    table_ws = AnalysisDataService.retrieve(table_name)

    # create dictionary
    fit_param_dict = dict()
    params_list = table_ws.getColumnNames()
    #  ['wsindex', 'peakindex', 'Height', 'PeakCentre', 'Sigma', 'A0', 'A1', 'chi2']

    # go through all lines
    num_rows = table_ws.rowCount()
    for irow in range(num_rows):
        value_dict = dict()
        ws_index = None
        for iparam, par_name in enumerate(params_list):
            value_i = table_ws.cell(irow, iparam)
            if par_name == "wsindex":
                ws_index = int(value_i)
            else:
                value_dict[par_name] = float(value_i)
        # END-FOR (column)
        assert ws_index is not None
        fit_param_dict[ws_index] = value_dict
    # END-FOR (row)

    return fit_param_dict


def fit_gaussian_linear_background(vec_x, vec_y, vec_e, start_value_list=None, find_start_value_by_fit=False):
    """
    Fit a curve with Gaussian + linear background
    The starting value can be
    1. specified by caller
    2. guessed by fitting a pure gaussian to the data
    3. guessed by observing the data
    :param vec_x:
    :param vec_y:
    :param vec_e:
    :param start_value_list: if not None, then it must have 4 elements:  x0, sigma, A, and b (for background)
    :param find_start_value_by_fit: if it is True, then fit the curve with a Gaussian without background
    :return: 3-tuple (1) float as error, (2) list/tuple as x0, sigma, a, b , (3) 4 x 4 covariance matrix
    """
    # check input
    assert isinstance(vec_x, numpy.ndarray), "Input vec_x must be a numpy.ndarray but not a {0}.".format(vec_x)
    assert isinstance(vec_y, numpy.ndarray), "Input vec_y must be a numpy.ndarray but not a {0}.".format(vec_y)
    assert isinstance(vec_e, numpy.ndarray), "Input vec_e must be a numpy.ndarray but not a {0}.".format(vec_e)

    # starting value
    if isinstance(start_value_list, list):
        assert len(start_value_list) == 4, "If specified, there must be 4 values: a, x0, sigma and b but not {0}." "".format(
            start_value_list
        )
    elif find_start_value_by_fit:
        # find out the starting value by fit a Gaussian without background
        fit1_coeff, fit1_cov_matrix = curve_fit(gaussian, vec_x, vec_y)
        start_x0, start_sigma, start_a = fit1_coeff
        # get result
        start_value_list = [start_x0, start_sigma, start_a, 0.0]

    else:
        # guess starting value via observation
        start_value_list = find_gaussian_start_values_by_observation(vec_x, vec_y)
    # END-IF-ELSE

    # do second round fit
    assert isinstance(start_value_list, list) and len(start_value_list) == 4, "Starting value list must have 4 elements"

    fit2_coeff, fit2_cov_matrix = curve_fit(gaussian_linear_background, vec_x, vec_y, sigma=vec_e, p0=start_value_list)
    # take sigma=vec_e out as it increases unstable

    # calculate the model
    x0, sigma, a, b = fit2_coeff
    model_vec_y = gaussian_linear_background(vec_x, x0, sigma, a, b)

    cost = calculate_penalty(model_vec_y, vec_y)

    return cost, fit2_coeff, fit2_cov_matrix


def fit_motor_intensity_model(motor_pos_dict, integrated_pt_dict):
    """
    construct a data as motor position vs counts, and do the fit with Gaussian + flat background
    :param motor_pos_dict:
    :param integrated_pt_dict:
    :return: 3-tuple: dictionary for fitted parameter, dictionary for fitting error, covariance matrix
    """
    # check inputs
    assert isinstance(motor_pos_dict, dict), "Input motor position {0} must be a dictionary but not a {1}." "".format(
        motor_pos_dict, type(motor_pos_dict)
    )
    assert isinstance(integrated_pt_dict, dict), "Input integrated Pt. intensity {0} must be a dictionary but not a " "{1}.".format(
        integrated_pt_dict, type(integrated_pt_dict)
    )

    # construct the data
    list_motor_pos = list()
    list_intensity = list()

    pt_list = motor_pos_dict.keys()
    pt_list.sort()

    for pt in pt_list:
        if pt not in integrated_pt_dict:
            raise RuntimeError("Pt. {0} does not exist in integrated intensity dictionary {1}" "".format(pt, integrated_pt_dict))
        list_motor_pos.append(motor_pos_dict[pt])
        list_intensity.append(integrated_pt_dict[pt])
    # END-FOR

    vec_x = numpy.array(list_motor_pos)
    vec_y = numpy.array(list_intensity)
    # try to avoid negative Y value
    vec_e = numpy.ndarray(shape=(len(vec_x),), dtype="float")
    for index in range(len(vec_y)):
        if vec_y[index] > 1.0:
            vec_e[index] = numpy.sqrt(vec_y[index])
        else:
            vec_e[index] = 1.0
    # END-FOR

    # fit
    gauss_error, gauss_parameters, cov_matrix = fit_gaussian_linear_background(vec_x, vec_y, vec_e)

    # function parameters (in order): x0, sigma, a, b
    # construct parameter dictionary and error dictionary
    gauss_parameter_dict = dict()
    gauss_error_dict = dict()

    gauss_parameter_dict["x0"] = gauss_parameters[0]
    gauss_parameter_dict["s"] = gauss_parameters[1]
    gauss_parameter_dict["A"] = gauss_parameters[2]
    gauss_parameter_dict["B"] = gauss_parameters[3]

    if str(cov_matrix).count("inf") > 0:
        # gaussian fit fails
        cov_matrix = None
    else:
        # good
        assert isinstance(cov_matrix, numpy.ndarray), "Covariance matrix must be a numpy array"
        # calculate fitting error/standard deviation
        g_error_array = numpy.sqrt(numpy.diag(cov_matrix))
        # print('[DB...BAT] Gaussian fit error (type {0}): {1}'.format(type(g_error_array), g_error_array))

        gauss_error_dict["x0"] = g_error_array[0]
        gauss_error_dict["s"] = g_error_array[1]
        gauss_error_dict["A"] = g_error_array[2]
        gauss_error_dict["B"] = g_error_array[3]

        gauss_error_dict["x02"] = cov_matrix[0, 0]
        gauss_error_dict["s2"] = cov_matrix[1, 1]
        gauss_error_dict["A2"] = cov_matrix[2, 2]
        gauss_error_dict["B2"] = cov_matrix[3, 3]
        gauss_error_dict["s_A"] = cov_matrix[1, 2]
        gauss_error_dict["A_s"] = cov_matrix[2, 1]
    # END-FOR

    return gauss_parameter_dict, gauss_error_dict, cov_matrix


def get_motor_step_for_intensity(motor_pos_dict):
    """
    get the motor step for each measurement Pts.
    if it is the first or last Pt. then use the difference between this Pt and its nearest Pts as motor step
    else use 1/2 as the motor step to its previous one and 1/2 as the motor step to its following one.
    :param motor_pos_dict:
    :return: dictionary of motor steps for calculating intensity
    """
    # check
    assert isinstance(motor_pos_dict, dict), "Input motor position must in dictionary."

    # get Pt list
    pt_list = motor_pos_dict.keys()
    pt_list.sort()
    if len(pt_list) < 2:
        raise RuntimeError("Motor position dictionary has too few Pt (FYI: Motor positions: {0}" "".format(motor_pos_dict))

    # get step dictionary
    motor_step_dict = dict()

    for i_pt in range(len(pt_list)):
        if i_pt == 0:
            # first motor position
            motor_step = motor_pos_dict[pt_list[1]] - motor_pos_dict[pt_list[0]]
        elif i_pt == len(pt_list) - 1:
            # last motor position
            motor_step = motor_pos_dict[pt_list[-1]] - motor_pos_dict[pt_list[-2]]
        else:
            # regular
            motor_step = 0.5 * (motor_pos_dict[pt_list[i_pt + 1]] - motor_pos_dict[pt_list[i_pt - 1]])
        pt = pt_list[i_pt]
        motor_step_dict[pt] = motor_step

    return motor_step_dict


def get_moving_motor_information(spice_table_name):
    """

    :param spice_table_name:
    :return:
    """
    table = AnalysisDataService.retrieve(spice_table_name)

    col_names = table.getColumnNames()
    pt_index = col_names.index("Pt.")
    omega_index = col_names.index("omega")
    chi_index = col_names.index("chi")
    phi_index = col_names.index("phi")

    col_tup_dict = {"omega": omega_index, "phi": phi_index, "chi": chi_index}

    std_list = list()
    motor_vector_dict = dict()
    for motor in col_tup_dict:
        motor_index = col_tup_dict[motor]
        motor_vector = numpy.array(table.column(motor_index))
        motor_vector_dict[motor] = motor_vector
        std_list.append((motor_vector.std(), motor))
    std_list.sort()
    moving_motor = std_list[-1][1]
    pt_list = table.column(pt_index)

    motor_pos_dict = dict()
    for i_m in range(len(pt_list)):
        motor_pos_dict[pt_list[i_m]] = motor_vector_dict[moving_motor][i_m]

    return moving_motor, motor_pos_dict


def gaussian_linear_background(x, x0, sigma, a, b):
    """
    Gaussian + linear background: y = a * exp( -(x-x0)**2/2*sigma**2 ) + b
    :param x:
    :param x0:
    :param sigma:
    :param a: maximum value
    :param b: linear background
    :return:
    """
    # gaussian + linear background
    return a * numpy.exp(-((x - x0) ** 2) / (2.0 * sigma**2)) + b


def gaussian(x, a, b, c):
    # pure gaussian
    return c * numpy.exp(-((x - a) ** 2) / (2.0 * b * b))


def gaussian_peak_intensity(parameter_dict, error_dict):
    """
    calculate peak intensity as a Gaussian
    the equation to calculate Gaussian from -infinity to +infinity is
    I = A\times s\times\\sqrt{2\\pi}
    :param parameter_dict:
    :param error_dict:
    :return:
    """
    # check input
    assert isinstance(parameter_dict, dict), "Parameters {0} must be given as a dictionary but not a {1}." "".format(
        parameter_dict, type(parameter_dict)
    )
    assert isinstance(error_dict, dict), "Errors {0} must be given as a dictionary but not a {1}." "".format(error_dict, type(error_dict))

    # get the parameters from the dictionary
    try:
        gauss_a = parameter_dict["A"]
        gauss_sigma = parameter_dict["s"]
    except KeyError as key_err:
        raise RuntimeError(
            'Parameter dictionary must have "A", "s" (for sigma) but now only {0}. Error message: {1}' "".format(
                parameter_dict.keys(), key_err
            )
        )

    # I = A\times s\times\sqrt{2 pi}
    peak_intensity = gauss_a * gauss_sigma * numpy.sqrt(2.0 * numpy.pi)

    # calculate error
    # \sigma_I^2 = 2\pi (A^2\cdot \sigma_s^2 + \sigma_A^2\cdot s^2 + 2\cdot A\cdot s\cdot \sigma_{As})
    try:
        error_a_sq = error_dict["A2"]
        error_s_sq = error_dict["s2"]
        error_a_s = error_dict["A_s"]
    except KeyError as key_err:
        raise RuntimeError(
            'Error dictionary must have "A2", "s2", "A_s" but not only found {0}. FYI: {1}' "".format(error_dict.keys(), key_err)
        )
    intensity_error = numpy.sqrt(
        2 / numpy.pi * (gauss_a**2 * error_s_sq + error_a_sq * gauss_sigma**2 + 2 * gauss_a * gauss_sigma * error_a_s)
    )

    return peak_intensity, intensity_error


def get_finer_grid(vec_x, factor):
    """
    insert values to a vector (grid) to make it finer
    :param vec_x:
    :param factor:
    :return:
    """
    assert isinstance(factor, int), "Insertion factor {0} must be an integer but not a {1}".format(factor, type(factor))

    orig_size = len(vec_x)
    new_list = list()
    for i in range(orig_size - 1):
        d_x = vec_x[i + 1] - vec_x[i]
        for j in range(factor):
            temp_x = vec_x[i] + d_x * float(j) / float(factor)
            new_list.append(temp_x)
        # END-FOR
    # END-FOR

    # don't forget the last
    new_list.append(vec_x[-1])

    new_vector = numpy.array(new_list)

    return new_vector


def integrate_single_scan_peak(
    merged_scan_workspace_name,
    integrated_peak_ws_name,
    peak_radius,
    peak_centre,
    merge_peaks=True,
    normalization="",
    mask_ws_name=None,
    scale_factor=1.0,
):
    """Integrate the peak in a single scan with merged Pt.
    :param merged_scan_workspace_name: MDEventWorkspace with merged Pts.
    :param integrated_peak_ws_name: output PeaksWorkspace for integrated peak
    :param peak_radius:
    :param peak_centre:  a float radius or None for not using
    :param merge_peaks: If selected, merged all the Pts can return 1 integrated peak's value;
                        otherwise, integrate peak for each Pt.
    :param normalization: normalization set up (by time or ...)
    :param mask_ws_name: mask workspace name or None
    :param scale_factor: integrated peaks' scaling factor
    :return: dictionary of Pts.
    """
    # check
    # assert isinstance(exp, int)
    # assert isinstance(scan, int)
    assert isinstance(peak_radius, float) or peak_radius is None, "Peak radius {0} must be of type float but not " "{1}.".format(
        peak_radius, type(peak_radius)
    )
    assert len(peak_centre) == 3, "Peak center {0} of type {1} must have 3 elements but not {2}." "".format(
        peak_centre, type(peak_centre), len(peak_centre)
    )
    assert isinstance(merge_peaks, bool), "Flag to merge peak must be a boolean but not {0}.".format(type(merge_peaks))

    try:
        peak_centre_str = "%f, %f, %f" % (peak_centre[0], peak_centre[1], peak_centre[2])
    except IndexError:
        raise RuntimeError("Peak center {0} must have 3 elements.".format(peak_centre))
    except ValueError:
        raise RuntimeError("Peak center {0} must have floats.".format(peak_centre))

    # normalization
    norm_by_mon = False
    norm_by_time = False
    if normalization == "time":
        norm_by_time = True
    elif normalization == "monitor":
        norm_by_mon = True

    # integrate peak of a scan
    print("[DB....BAT] Peak integration scale factor: {0}".format(scale_factor))
    mantidsimple.IntegratePeaksCWSD(
        InputWorkspace=merged_scan_workspace_name,
        OutputWorkspace=integrated_peak_ws_name,
        PeakRadius=peak_radius,
        PeakCentre=peak_centre_str,
        MergePeaks=merge_peaks,
        NormalizeByMonitor=norm_by_mon,
        NormalizeByTime=norm_by_time,
        MaskWorkspace=mask_ws_name,
        ScaleFactor=scale_factor,
    )

    # process the output workspace
    pt_dict = dict()
    out_peak_ws = AnalysisDataService.retrieve(integrated_peak_ws_name)
    num_peaks = out_peak_ws.rowCount()

    for i_peak in range(num_peaks):
        peak_i = out_peak_ws.getPeak(i_peak)
        run_number_i = peak_i.getRunNumber() % 1000
        intensity_i = peak_i.getIntensity()
        pt_dict[run_number_i] = intensity_i
    # END-FOR

    # # store the data into peak info
    # if (exp, scan) not in self._myPeakInfoDict:
    #     raise RuntimeError('Exp %d Scan %d is not recorded in PeakInfo-Dict' % (exp, scan))
    # self._myPeakInfoDict[(exp, scan)].set_pt_intensity(pt_dict)

    return True, pt_dict


# TEST NOW3 - API changed!
def integrate_peak_full_version(
    scan_md_ws_name,
    spice_table_name,
    output_peak_ws_name,
    peak_center,
    mask_workspace_name,
    norm_type,
    intensity_scale_factor,
    background_pt_tuple,
):
    """
    Integrate peak with the full version including
    1. simple summation
    2. simple summation with gaussian fit
    3. integrate with fitted gaussian
    :return: dictionary: peak integration result
    """

    def create_peak_integration_dict():
        """
        create a standard dictionary for recording peak integration result
        keys are
         - simple intensity
         - simple error
         - simple background
         - intensity 2
         - error 2
         - gauss intensity
         - gauss error
         - gauss background
         - gauss parameters
         - motor positions: numpy array of motor positions
         - pt intensities: numpy array of integrated intensities per Pt.
        :return:
        """
        info_dict = {
            "simple intensity": 0.0,
            "simple error": 0.0,
            "simple background": 0.0,
            "intensity 2": 0.0,
            "error 2": 0.0,
            "pt_range": "",
            "gauss intensity": 0.0,
            "gauss error": 0.0,
            "gauss background": 0.0,
            "gauss parameters": None,
            "gauss errors": None,  # details can be found in (this module) fit_motor_intensity_model()
            "motor positions": None,
            "pt intensities": None,
            "covariance matrix": None,
        }

        return info_dict

    # END-DEF: create_peak_integration_dict()

    # integrate the peak in MD workspace
    try:
        status, ret_obj = integrate_single_scan_peak(
            merged_scan_workspace_name=scan_md_ws_name,
            integrated_peak_ws_name=output_peak_ws_name,
            peak_radius=1.0,
            peak_centre=peak_center,
            merge_peaks=False,
            mask_ws_name=mask_workspace_name,
            normalization=norm_type,
            scale_factor=intensity_scale_factor,
        )
    except RuntimeError as run_err:
        raise RuntimeError("Failed to integrate peak at {0} due to {1}".format(scan_md_ws_name, run_err))
    except Exception as run_err:
        raise RuntimeError("Failed (2) to integrate peak at {0} due to {1}".format(scan_md_ws_name, run_err))

    # result due to error
    if status is False:
        error_message = ret_obj
        raise RuntimeError("Unable to integrate peak of workspace {0} due to {1}." "".format(scan_md_ws_name, error_message))
    else:
        # process result
        integrated_pt_dict = ret_obj
        assert isinstance(integrated_pt_dict, dict), "Returned masked Pt dict must be a dictionary"

    # create output dictionary
    peak_int_dict = create_peak_integration_dict()

    # get moving motor information. candidates are 2theta, phi and chi
    motor, motor_pos_dict = get_moving_motor_information(spice_table_name)

    # check motor position dictionary and integrated per Pt. peak intensity
    motor_pos_vec, pt_intensity_vec = convert_motor_pos_intensity(integrated_pt_dict, motor_pos_dict)
    peak_int_dict["motor positions"] = motor_pos_vec
    peak_int_dict["pt intensities"] = pt_intensity_vec
    peak_int_dict["mask"] = mask_workspace_name

    # get motor step per pt.
    try:
        motor_step_dict = get_motor_step_for_intensity(motor_pos_dict)
    except RuntimeError as run_err:
        raise RuntimeError("Unable to integrate workspace {0} due to {1}.".format(scan_md_ws_name, run_err))

    # calculate the intensity with background removed and correct intensity by background value
    averaged_background = estimate_background(integrated_pt_dict, background_pt_tuple)
    simple_intensity, simple_intensity_error, pt_range = simple_integrate_peak(integrated_pt_dict, averaged_background, motor_step_dict)
    peak_int_dict["simple background"] = averaged_background
    peak_int_dict["simple intensity"] = simple_intensity
    peak_int_dict["simple error"] = simple_intensity_error
    peak_int_dict["simple background"] = averaged_background

    # fit gaussian + flat background
    parameters, gauss_error_dict, covariance_matrix = fit_motor_intensity_model(motor_pos_dict, integrated_pt_dict)
    peak_int_dict["gauss parameters"] = parameters
    peak_int_dict["gauss errors"] = gauss_error_dict
    peak_int_dict["covariance matrix"] = covariance_matrix

    if covariance_matrix is None or parameters["B"] < 0.0:
        # gaussian fit fails or output result is not correct
        peak_int_dict["intensity 2"] = None
        peak_int_dict["error 2"] = None

        peak_int_dict["gauss intensity"] = None
        peak_int_dict["gauss error"] = None

    else:
        # calculate intensity with method 2
        motor_pos_center = parameters["x0"]
        motor_pos_sigma = parameters["s"]
        intensity_m2, error_m2, pt_range = simple_integrate_peak(
            integrated_pt_dict,
            parameters["B"],
            motor_step_dict,
            peak_center=motor_pos_center,
            peak_sigma=motor_pos_sigma,
            motor_pos_dict=motor_pos_dict,
            sigma_range=2.0,
        )

        peak_int_dict["intensity 2"] = intensity_m2
        peak_int_dict["error 2"] = error_m2
        peak_int_dict["pt_range"] = pt_range

        # calculate gaussian (method 3)
        intensity_gauss, intensity_gauss_error = gaussian_peak_intensity(parameters, gauss_error_dict)
        peak_int_dict["gauss intensity"] = intensity_gauss
        peak_int_dict["gauss error"] = intensity_gauss_error
    # END-IF-ELSE

    return peak_int_dict


def read_peak_integration_table_csv(peak_file_name):
    """
    read a csv file saved from the peak integration information table
    :param peak_file_name:
    :return: a dictionary of peak integration result information in STRING form
    """
    # check input
    assert isinstance(peak_file_name, str), "Peak integration table file {0} must be a string but not a {1}." "".format(
        peak_file_name, type(peak_file_name)
    )

    if os.path.exists(peak_file_name) is False:
        raise RuntimeError("Peak integration information file {0} does not exist.".format(peak_file_name))

    # read
    scan_peak_dict = dict()
    with open(peak_file_name, "r") as csv_file:
        reader = csv.reader(csv_file, delimiter="\t", quotechar="#")
        title_list = None
        for index, row in enumerate(reader):
            if index == 0:
                # title
                title_list = row
            else:
                # value
                peak_dict = dict()
                for term_index, value in enumerate(row):
                    peak_dict[title_list[term_index]] = value
                scan_number = int(peak_dict["Scan"])
                scan_peak_dict[scan_number] = peak_dict
            # END-IF-ELSE
        # END-FOR
    # END-WITH

    return scan_peak_dict


def simple_integrate_peak(
    pt_intensity_dict, bg_value, motor_step_dict, peak_center=None, peak_sigma=None, motor_pos_dict=None, sigma_range=2.0
):
    """
    A simple approach to integrate peak in a cuboid with background removed.
    :param pt_intensity_dict:
    :param bg_value:
    :param motor_step_dict:
    :param peak_center:
    :param peak_sigma:
    :param motor_pos_dict:
    :param sigma_range:
    :return:
    """
    # check
    assert isinstance(pt_intensity_dict, dict), "Pt. intensities {0} should be a dictionary but not a {1}." "".format(
        pt_intensity_dict, type(pt_intensity_dict)
    )
    assert isinstance(bg_value, float) and bg_value >= 0.0, "Background value {0} must be a non-negative float." "".format(bg_value)
    assert isinstance(motor_step_dict, dict), "Motor steps {0} must be given as a dictionary of Pt but not a {1}." "".format(
        motor_step_dict, type(motor_step_dict)
    )

    if peak_center is not None:
        assert peak_sigma is not None and motor_pos_dict is not None and sigma_range is not None, "Must be specified"

    pt_list = pt_intensity_dict.keys()
    pt_list.sort()

    # loop over Pt. to sum for peak's intensity
    sum_intensity = 0.0
    used_pt_list = list()

    # raw intensity
    sum_raw_int = 0.0

    motor_step = 0.0
    for pt in pt_list:
        # check the motor position if required
        if peak_center is not None:
            motor_pos = motor_pos_dict[pt]
            if abs(motor_pos - peak_center) > sigma_range * peak_sigma:
                # peak is out of range
                continue
        # END-IF

        intensity = pt_intensity_dict[pt]
        motor_step_i = motor_step_dict[pt]
        sum_intensity += (intensity - bg_value) * motor_step_i
        motor_step = motor_step_i

        if 0:
            pass
            # error_2 += numpy.sqrt(intensity) * motor_step_i
        else:
            sum_raw_int += intensity

        used_pt_list.append(pt)
    # END-FOR

    # error = sqrt(sum I_i) * delta
    error_2 = numpy.sqrt(sum_raw_int) * motor_step

    # convert the Pt to list
    if len(used_pt_list) > 0:
        used_pt_list.sort()
        pt_list_range = "{0} - {1}".format(used_pt_list[0], used_pt_list[-1])
    else:
        pt_list_range = "N/A"

    return sum_intensity, error_2, pt_list_range
