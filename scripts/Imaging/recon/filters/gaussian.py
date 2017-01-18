from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper


def execute(data, size, mode, order, h=None):
    """

    :param data: The data that will be processed with this filter
    :param size: Size of the filter kernel
    :param mode: Mode for the borders of the filter. Options available in script -h command
    :param order: An order of 0 corresponds to convolution with a Gaussian kernel.
            An order of 1, 2, or 3 corresponds to convolution with the first, second or third derivatives of a Gaussian.
            Higher order derivatives are not implemented.
    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: Returns the processed data
    """
    from recon.filters.median_filter import import_scipy_ndimage
    h = Helper.empty_init() if h is None else h

    if size and size > 1:
        scipy_ndimage = import_scipy_ndimage()
        h.pstart(
            "Starting  gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
            format(data.dtype, size))

        for idx in range(0, data.shape[0]):
            data[idx] = scipy_ndimage.gaussian_filter(
                data[idx], size, mode=mode, order=order)

        h.pstop(
            "Finished  gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
            format(data.dtype, size))

    else:
        h.tomo_print_note("NOT applying gaussian filter.")

    return data
