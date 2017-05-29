from __future__ import (absolute_import, division, print_function)
from helper import Helper

def modes():
    return ['reflect', 'constant', 'nearest', 'mirror', 'wrap']


def execute(data, size, mode, order, cores=None, chunksize=None, h=None):
    """
    Execute the Gaussian filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: The size of the kernel
    :param mode: The mode with which to handle the endges
    :param order: The order of the filter along each axis is given as a sequence of integers, or as a single number.
                  An order of 0 corresponds to convolution with a Gaussian kernel.
                  An order of 1, 2, or 3 corresponds to convolution with the first, second or third
                  derivatives of a Gaussian. Higher order derivatives are not implemented

    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: the data after being processed with the filter

    Full reference:
    https://docs.scipy.org/doc/scipy-0.16.1/reference/generated/scipy.ndimage.filters.gaussian_filter.html

    """
    h = Helper.empty_init() if h is None else h
    h.check_data_stack(data)

    if size and size > 1:
        from parallel import utility as pu
        if pu.multiprocessing_available():
            data = _execute_par(data, size, mode, order, cores, chunksize, h)
        else:
            data = _execute_seq(data, size, mode, order, h)

    else:
        h.tomo_print_note("NOT applying gaussian filter.")

    h.check_data_stack(data)
    return data


def _execute_seq(data, size, mode, order, h=None):
    """
    Sequential CPU version of the Gaussian filter
    """

    from filters.median_filter import import_scipy_ndimage

    scipy_ndimage = import_scipy_ndimage()
    h.pstart(
        "Starting  gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    h.prog_init(data.shape[0], "Gaussian")
    for idx in range(0, data.shape[0]):
        data[idx] = scipy_ndimage.gaussian_filter(
            data[idx], size, mode=mode, order=order)
        h.prog_update()

    h.prog_close()

    h.pstop(
        "Finished  gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    return data


def _execute_par(data, size, mode, order, cores=None, chunksize=None, h=None):
    """
    Parallel CPU version of the Gaussian filter
    """
    from filters.median_filter import import_scipy_ndimage

    scipy_ndimage = import_scipy_ndimage()

    from parallel import shared_mem as psm

    f = psm.create_partial(
        scipy_ndimage.gaussian_filter,
        fwd_func=psm.return_fwd_func,
        sigma=size,
        mode=mode,
        order=order)

    h.pstart(
        "Starting PARALLEL gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))
    data = psm.execute(data, f, cores, chunksize, "Gaussian", h)

    h.pstop(
        "Finished  gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    return data
