# Utility methods to do peak integration
import numpy
import scipy
from scipy.optimize import curve_fit

def gaussian_linear_background(x, x0, sigma, a, b):
    # gaussian + linear background
    return a * numpy.exp(-(x - x0) ** 2 / (2. * sigma ** 2)) + b


def gaussian(x, a, b, c):
    # pure gaussian
    return c*numpy.exp(-(x-a)**2/(2. * b * b))


def find_gaussian_start_values_by_observation(vec_x, vec_y):
    """
    find out the starting values of a gaussian + linear background with observation
    :param vec_x:
    :param vec_y:
    :return:
    """
    # assume that it is a quasi-ideal Gaussian

    # find out the maximum Y with x
    max_y_index = vec_x.argmax()

    x0 = vec_x[max_y_index]
    max_y = vec_y[max_y_index]
    background =


def fit_gaussian_linear_background(vec_x, vec_y, vec_e, start_value_list=None, find_start_value_by_fit=False):
    """

    :param vec_x:
    :param vec_y:
    :param vec_e:
    :param start_value_list: if not None, then it must have 4 elements:  x0, sigma, A, and b (for background)
    :param find_start_value_by_fit: if it is True, then fit the curve with a Gaussian without background
    :return: 2-tuple,
    """
    # check input
    assert isinstance(vec_x, numpy.ndarray), 'Input vec_x must be a numpy.ndarray but not a {0}.'.format(vec_x)
    assert isinstance(vec_y, numpy.ndarray), 'Input vec_y must be a numpy.ndarray but not a {0}.'.format(vec_y)
    assert isinstance(vec_e, numpy.ndarray), 'Input vec_e must be a numpy.ndarray but not a {0}.'.format(vec_e)

    # starting value
    if isinstance(start_value_list, list):
        assert len(start_value_list) == 4, 'If specified, there must be 4 values: a, x0, sigma and b but not {0}.' \
                                           ''.format(start_value_list)
    elif find_start_value_by_fit:
        # find out the starting value by fit a Gaussian without background
        fit1_coeff, fit1_cov_matrix = curve_fit(gaussian, vec_x, vec_y)
        # get result
        start_value_list = [fit1_coeff[0], fit1_coeff[1], fit1_coeff[2], 0.]

    else:
        # guess starting value via observation
        start_value_list = find_gaussian_start_values_by_observation(vec_x, vec_y)

    # END-IF-ELSE

    # do second round fit
    fit2_coeff, fit2_cov_matrix = curve_fit(gaussian_linear_background, vec_x, vec_y, sigma=vec_e,  p0=start_value_list)

    # calculate the model
    x0, sigma, a, b = fit2_coeff
    model_vec_y = gaussian_linear_background(vec_x, x0, sigma, a, b)

    return fit2_coeff, model_vec_y


