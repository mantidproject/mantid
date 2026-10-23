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
from mantidqtinterfaces.HFIR_4Circle_Reduction.fourcircle_utility import check_string


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
    assert isinstance(motor_pos_array, numpy.ndarray), "Motor positions {0} must be given as a numpy array but not a {1}.".format(
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
        assert isinstance(error_sigma_sq, float), "Error(sigma)**2 must be a float but not a {0}.".format(type(error_sigma_sq))
        assert isinstance(error_a_sigma, float), "Error(a,sigma) must be a float but not a {0}.".format(type(error_a_sigma))
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
    assert isinstance(model_vec_y, numpy.ndarray), "Model vec Y cannot be {0}.".format(type(model_vec_y))
    assert isinstance(exp_vec_y, numpy.ndarray), "Experimental vec Y cannot be {0}.".format(type(exp_vec_y))
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
    vec_x = peak_ws.x(0)
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
        assert len(start_value_list) == 4, "If specified, there must be 4 values: a, x0, sigma and b but not {0}.".format(start_value_list)
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


def read_peak_integration_table_csv(peak_file_name):
    """
    read a csv file saved from the peak integration information table
    :param peak_file_name:
    :return: a dictionary of peak integration result information in STRING form
    """
    # check input
    assert isinstance(peak_file_name, str), "Peak integration table file {0} must be a string but not a {1}.".format(
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
