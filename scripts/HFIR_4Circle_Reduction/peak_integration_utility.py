# Utility methods to do peak integration
import numpy
from scipy.optimize import curve_fit
import mantid.simpleapi as mantidsimple
from mantid.api import AnalysisDataService


def calculate_peak_intensity_gauss(gauss_a, gauss_sigma, error_a_sq=None, error_sigma_sq=None,
                                   error_a_sigma=None):
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
    integral = numpy.sqrt(2. * numpy.pi) * gauss_a * gauss_sigma

    if error_a_sq is not None:
        # calculate integral intensity error by propagation
        # check
        assert isinstance(error_a_sq, float), 'Error(a)**2 must be a float but not a {0}.'.format(type(error_a_sq))
        assert isinstance(error_sigma_sq, float), 'Error(sigma)**2 must be a float but not a {0}.' \
                                                  ''.format(type(error_sigma_sq))
        assert isinstance(error_a_sigma, float), 'Error(a,sigma) must be a float but not a {0}.' \
                                                 ''.format(type(error_a_sigma))
        # calculate
        error2 = gauss_a**2 * error_sigma_sq + error_a_sq * gauss_sigma**2 + 2. * gauss_a * gauss_sigma * error_a_sq
        error = numpy.sqrt(error2)
    else:
        error = numpy.sqrt(integral)

    return integral, error


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


def get_moving_motor_information(spice_table_name):
    """

    :param spice_table_name:
    :return:
    """
    table = AnalysisDataService.retrieve(spice_table_name)

    col_names = table.getColumnNames()
    pt_index = col_names.index('Pt.')
    omega_index = col_names.index('omega')
    chi_index = col_names.index('chi')
    phi_index = col_names.index('phi')

    col_tup_dict = {'omega': omega_index, 'phi': phi_index, 'chi': chi_index}

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

    print '[DB] Input x0 = ', x0, ', sigma = ', sigma, ', a = ', a, ', b = ', b
    return a * numpy.exp(-(x - x0) ** 2 / (2. * sigma ** 2)) + b


def gaussian(x, a, b, c):
    # pure gaussian
    return c*numpy.exp(-(x-a)**2/(2. * b * b))


def find_gaussian_start_values_by_observation(vec_x, vec_y):
    """
    find out the starting values of a gaussian + linear background with observation
    :param vec_x:
    :param vec_y:
    :return: must be the same order as gaussian linear background function as x0, sigma, a, b
    """
    # assume that it is a quasi-ideal Gaussian
    print '[DB] Observation: vec x = ', vec_x, '; vec y = ', vec_y

    # find out the maximum Y with x
    max_y_index = vec_y.argmax()
    print '[DB] max Y index = ', max_y_index

    x0 = vec_x[max_y_index]
    max_y = vec_y[max_y_index]
    est_background = 0.5 * (vec_y[0] + vec_y[-1])
    est_sigma = (vec_x[-1] - vec_x[0]) * 0.1
    est_a = max(1.0, max_y - est_background)

    return [x0, est_sigma, est_a, est_background]


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
    assert isinstance(vec_x, numpy.ndarray), 'Input vec_x must be a numpy.ndarray but not a {0}.'.format(vec_x)
    assert isinstance(vec_y, numpy.ndarray), 'Input vec_y must be a numpy.ndarray but not a {0}.'.format(vec_y)
    assert isinstance(vec_e, numpy.ndarray), 'Input vec_e must be a numpy.ndarray but not a {0}.'.format(vec_e)

    print '[DB] Vec X: ', vec_x
    print '[DB] Vec Y: ', vec_y
    print '[DB] Vec e: ', vec_e
    print '[DB] Start values: ', start_value_list
    print '[DB] Find start value by fit: ', find_start_value_by_fit

    # TODO/DEBUG/NOW/ - FROM HERE!

    # starting value
    if isinstance(start_value_list, list):
        assert len(start_value_list) == 4, 'If specified, there must be 4 values: a, x0, sigma and b but not {0}.' \
                                           ''.format(start_value_list)
    elif find_start_value_by_fit:
        # find out the starting value by fit a Gaussian without background
        fit1_coeff, fit1_cov_matrix = curve_fit(gaussian, vec_x, vec_y)
        start_x0, start_sigma, start_a = fit1_coeff
        # get result
        start_value_list = [start_x0, start_sigma, start_a, 0.0]

        print '[DB] Start value by fit: ', start_value_list

    else:
        # guess starting value via observation
        start_value_list = find_gaussian_start_values_by_observation(vec_x, vec_y)

        print '[DB] Start value by observation: ', start_value_list

    # END-IF-ELSE

    """
    [DB] Start values:  None
    [DB] Find start value by fit:  False
    [DB] Start value by observation:  [21, 19.0, 10.5, 100.0]: should be
    """

    # do second round fit
    assert isinstance(start_value_list, list) and len(start_value_list) == 4, 'Starting value list must have 4 elements'
    fit2_coeff, fit2_cov_matrix = curve_fit(gaussian_linear_background, vec_x, vec_y,  sigma=vec_e, p0=start_value_list)
    # take sigma=vec_e,  out as it increases unstable

    # calculate the model
    x0, sigma, a, b = fit2_coeff
    model_vec_y = gaussian_linear_background(vec_x, x0, sigma, a, b)

    print 'Covariance matrix: ', fit2_cov_matrix

    # calculate error as (model Y - obs Y)**2/wi
    # this is a bad way to calculate error!
    # TODO/FIXME/NOW - Need a correct way to calculate the error AND/OR penalty!
    diff_y = model_vec_y - vec_y
    diff_y2 = numpy.ndarray(shape=(len(diff_y),), dtype='float')
    error = numpy.power(diff_y, 2, out=diff_y2)/len(diff_y)

    return error, fit2_coeff, fit2_cov_matrix


def fit_motor_intensity_model(motor_pos_dict, integrated_pt_dict):
    """
    construct a data as motor position vs counts, and do the fit with Gaussian + flat background
    :param motor_pos_dict:
    :param integrated_pt_dict:
    :return:
    """
    # check inputs
    assert isinstance(motor_pos_dict, dict), 'Input motor position {0} must be a dictionary but not a {1}.' \
                                             ''.format(motor_pos_dict, type(motor_pos_dict))
    assert isinstance(integrated_pt_dict, dict), 'Input integrated Pt. intensity {0} must be a dictionary but not a ' \
                                                 '{1}.'.format(integrated_pt_dict, type(integrated_pt_dict))

    # construct the data
    list_motor_pos = list()
    list_intensity = list()

    pt_list = motor_pos_dict.keys()
    pt_list.sort()

    for pt in pt_list:
        if pt not in integrated_pt_dict:
            raise RuntimeError('Pt. {0} does not exist in integrated intensity dictionary {1}'
                               ''.format(pt, integrated_pt_dict))
        list_motor_pos.append(motor_pos_dict[pt])
        list_intensity.append(integrated_pt_dict[pt])
    # END-FOR

    vec_x = numpy.array(list_motor_pos)
    vec_y = numpy.array(list_intensity)
    # try to avoid negative Y value
    vec_e = numpy.ndarray(shape=(len(vec_x),), dtype='float')
    for index in range(len(vec_y)):
        if vec_y[index] > 1.:
            vec_e[index] = numpy.sqrt(vec_y[index])
        else:
            vec_e[index] = 1.
    # END-FOR

    # fit
    # TODO/ISSUE/NOW - Need to find out the return result
    fit_gaussian_linear_background(vec_x, vec_y, vec_e)

    return

def gaussian_peak_intensity(parameter_dict, error_dict):
    """
    calculate peak intensity as a Gaussian
    :param parameter_dict:
    :param error_dict:
    :return:
    """
    # check input
    assert isinstance(parameter_dict, dict), 'Parameters {0} must be given as a dictionary but not a {1}.' \
                                             ''.format(parameter_dict, type(parameter_dict))
    assert isinstance(error_dict, dict), 'Errors {0} must be given as a dictionary but not a {1}.' \
                                         ''.format(error_dict, type(error_dict))

    # get the parameters from the dictionary
    try:
        gauss_a = parameter_dict['A']
        gauss_sigma = parameter_dict['Sigma']
    except KeyError as key_err:
        raise RuntimeError('Parameter dictionary must have "A", "Sigma" but now only {0}. Error message: {1}'
                           ''.format(parameter_dict.keys(), key_err))

    # TODO/ISSUE/NOW - Continue from here to calcualte Gaussian area

    return 1E20

def get_finer_grid(vec_x, factor):
    """
    insert values to a vector (grid) to make it finer
    :param vec_x:
    :param factor:
    :return:
    """
    assert isinstance(factor, int), 'Insertion factor {0} must be an integer but not a {1}'.format(factor, type(factor))

    orig_size = len(vec_x)
    new_list = list()
    for i in range(orig_size-1):
        d_x = vec_x[i+1] - vec_x[i]
        for j in range(factor):
            temp_x = vec_x[i] + d_x * float(j) / float(factor)
            new_list.append(temp_x)
        # END-FOR
    # END-FOR

    # don't forget the last
    new_list.append(vec_x[-1])

    new_vector = numpy.array(new_list)

    return new_vector


def integrate_scan_peaks(#exp, scan,
                         merged_scan_workspace_name, integrated_peak_ws_name,
                         peak_radius, peak_centre,
                         merge_peaks=True, use_mask=False,
                         normalization='', mask_ws_name=None,
                         scale_factor=1):
    """
    # :param exp:
    # :param scan:
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
    # assert isinstance(exp, int)
    # assert isinstance(scan, int)
    assert isinstance(peak_radius, float) or peak_radius is None
    assert len(peak_centre) == 3
    assert isinstance(merge_peaks, bool)

    # get spice file
    # spice_table_name = get_spice_table_name(exp, scan)
    # if AnalysisDataService.doesExist(spice_table_name) is False:
    #     self.download_spice_file(exp, scan, False)
    #     self.load_spice_scan_file(exp, scan)

    # get MD workspace name
    # status, pt_list = self.get_pt_numbers(exp, scan)
    # assert status, str(pt_list)
    # md_ws_name = get_merged_md_name(self._instrumentName, exp, scan, pt_list)

    peak_centre_str = '%f, %f, %f' % (peak_centre[0], peak_centre[1],
                                      peak_centre[2])

    # mask workspace
    # if use_mask:
    #     if mask_ws_name is None:
    #         # get default mask workspace name
    #         mask_ws_name = get_mask_ws_name(exp, scan)
    #     elif not AnalysisDataService.doesExist(mask_ws_name):
    #         # the appointed mask workspace has not been loaded
    #         # then load it from saved mask
    #         self.check_generate_mask_workspace(exp, scan, mask_ws_name)
    #
    #     assert AnalysisDataService.doesExist(mask_ws_name), 'MaskWorkspace %s does not exist.' \
    #                                                         '' % mask_ws_name
    #
    #     integrated_peak_ws_name = get_integrated_peak_ws_name(exp, scan, pt_list, use_mask)
    # else:
    #     mask_ws_name = ''
    #     integrated_peak_ws_name = get_integrated_peak_ws_name(exp, scan, pt_list)

    # normalization
    norm_by_mon = False
    norm_by_time = False
    if normalization == 'time':
        norm_by_time = True
    elif normalization == 'monitor':
        norm_by_mon = True

    # integrate peak of a scan
    mantidsimple.IntegratePeaksCWSD(InputWorkspace=merged_scan_workspace_name,
                                    OutputWorkspace=integrated_peak_ws_name,
                                    PeakRadius=peak_radius,
                                    PeakCentre=peak_centre_str,
                                    MergePeaks=merge_peaks,
                                    NormalizeByMonitor=norm_by_mon,
                                    NormalizeByTime=norm_by_time,
                                    MaskWorkspace=mask_ws_name,
                                    ScaleFactor=scale_factor)

    # process the output workspace
    pt_dict = dict()
    out_peak_ws = AnalysisDataService.retrieve(integrated_peak_ws_name)
    num_peaks = out_peak_ws.rowCount()

    for i_peak in xrange(num_peaks):
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


def integrate_peak_full_version(scan_md_ws_name, spice_table_name, output_peak_ws_name,
                                peak_center, mask_workspace_name, norm_type,
                                intensity_scale_factor, background_pt_list):
    """
    Integrate peak with the full version including
    1. simple summation
    2. simple summation with gaussian fit
    3. integrate with fitted gaussian
    :return:
    """
    # integrate the peak in MD workspace
    # spice_table_name, integrated_peak_ws_name, md_ws_name,
    status, ret_obj = integrate_scan_peaks(merged_scan_workspace_name=scan_md_ws_name,
                                           integrated_peak_ws_name=output_peak_ws_name,
                                           peak_radius=1.0,
                                           peak_centre=peak_center,
                                           merge_peaks=False,
                                           use_mask=True,
                                           mask_ws_name=mask_workspace_name,
                                           normalization=norm_type,
                                           scale_factor=intensity_scale_factor)

    # result due to error
    if status is False:
        error_message = ret_obj
        return False, error_message
    else:
        # process result
        integrated_pt_dict = ret_obj
        assert isinstance(integrated_pt_dict, dict), 'Returned masked Pt dict must be a dictionary'

    # get moving motor information. candidates are 2theta, phi and chi
    motor, motor_pos_dict = get_moving_motor_information(spice_table_name)

    # calculate the intensity with background removed and correct intensity by background value
    averaged_background = estimate_background(integrated_pt_dict, background_pt_list)
    simple_intensity, simple_intensity_error = simple_integrate_peak(integrated_pt_dict, averaged_background,
                                                                     motor_pos_dict)

    # fit gaussian + flat background
    parameters, errors = fit_motor_intensity_model(motor_pos_dict, integrated_pt_dict)

    # calculate intensity with method 2
    intensity_m2, error_m2 = simple_integrate_peak(integrated_pt_dict, parameters['B'])

    # calculate gaussian
    intensity_gauss, intensity_guass_error = gaussian_peak_intensity(parameters, errors)

    return

    #
    #
    #
    # simple_peak_intensity = self.ui.tableWidget_peakIntegration.sum_masked_intensity()
    #     info_text += 'Simple summation of counts masked by {0}'.format(mask_name)
    # self.ui.lineEdit_rawSinglePeakIntensity.setText(str(simple_peak_intensity))
    #
    # info_text += '; Normalized by {0}'.format(norm_type)
    # self.ui.label_ingreateInformation.setText(info_text)
    #
    # # Clear previous line and plot the Pt.
    # self.ui.graphicsView_integratedPeakView.reset()
    # x_array = numpy.array(pt_list)
    # y_array = numpy.array(intensity_list)
    # self.ui.graphicsView_integratedPeakView.plot_raw_data(x_array, y_array)

    return


def calculate_motor_step(motor_pos_array, motor_step_tolerance=0.5):
    """

    :param pt_motor_dict:
    :return:
    """
    assert isinstance(motor_pos_array, numpy.ndarray), 'blabla 2'
    # need to check somewhere: assert len(pt_motor_dict) == len(pt_intensity_dict), 'blabla 3'

    motor_step_vector = motor_pos_array[1:] - motor_pos_array[:-1]

    motor_step = numpy.average(motor_step_vector)
    motor_step_std = motor_step_vector.std()

    if motor_step_std > motor_step_tolerance:
        raise RuntimeError('Step deviation too large. Cannot use average!')

    return motor_step


def simple_integrate_peak(pt_intensity_dict, bg_value, motor_step):
    """
    A simple approach to integrate peak in a cuboid with background removed.
    :param pt_intensity_dict:
    :param bg_value:
    :return:
    """
    # check
    assert isinstance(pt_intensity_dict, dict), 'Pt. intensities {0} should be a dictionary but not a {1}.' \
                                                ''.format(pt_intensity_dict, type(pt_intensity_dict))
    assert isinstance(bg_value, float) and bg_value >= 0., 'Background value {0} must be a non-negative float.' \
                                                           ''.format(bg_value)
    assert isinstance(motor_step, float) and motor_step > 0., 'Motor step {0} must be a positive float but not a' \
                                                              ' {1}.'.format(motor_step, type(motor_step))

    # loop over Pt. to sum for peak's intensity
    sum_intensity = 0.
    error_2 = 0.
    for intensity in pt_intensity_dict.values():
        sum_intensity += (intensity - bg_value) * motor_step
        error_2 += numpy.sqrt(intensity) * motor_step

    return sum_intensity, error_2
