# Utility methods to do peak integration
import numpy
import scipy
from scipy.optimize import curve_fit


def calculate_peak_intensity_gauss(gauss_a, gauss_sigma, error_a_sq=None, error_sigma_sq=None, error_q_sigma=None):
    """
    calcukalte the peak intensity, which is the area under the peak
    if sigma == 1, then the integral is sqrt(pi);
    then the value is sqrt(pi) * e^{-1/(2.*sigma**2)}
    :param gauss_a:
    :param gauss_sigma:
    :return:
    """
    integral = numpy.sqrt(2. * numpy.pi) * gauss_a * gauss_sigma

    if error_a_sq is not None:
        # TODO/ISSUE/NOW - Implement error to return
        pass

    else:
        error = numpy.sqrt(integral)

    return integral, error


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
    est_sigma = len(vec_x) * 0.5
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
    :return: 3-tuple (1) float as error, (2) list/tuple as x0, sigma, a, b , (3) 1-D array as model Y
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
    fit2_coeff, fit2_cov_matrix = curve_fit(gaussian_linear_background, vec_x, vec_y, p0=start_value_list)
    # take sigma=vec_e,  out as it increases unstable

    # calculate the model
    x0, sigma, a, b = fit2_coeff
    # TODO/ISSUE/NOW - make modelX and modelY for more fine grids
    model_vec_y = gaussian_linear_background(vec_x, x0, sigma, a, b)

    print 'Covariance matrix: ', fit2_cov_matrix

    # calculate error as (model Y - obs Y)**2/wi
    diff_y = model_vec_y - vec_y
    diff_y2 = numpy.ndarray(shape=(len(diff_y),), dtype='float')
    error = numpy.power(diff_y, 2, out=diff_y2)/len(diff_y)

    return error, fit2_coeff, model_vec_y


