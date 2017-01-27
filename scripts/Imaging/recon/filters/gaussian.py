from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper

__module__ = 'gaussian_p'


def execute(data, size, mode, order, cores=1, chunksize=None, h=None):
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


def _execute_par(data, size, mode, order, cores=1, chunksize=None, h=None):
    """
    Parallel version of the gaussian filter.

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

    scipy_ndimage = import_scipy_ndimage()

    from parallel import shared_mem as psm

    f = psm.create_partial(scipy_ndimage.gaussian_filter, fwd_function=psm.fwd_func,
                           sigma=size, mode=mode, order=order)

    h.pstart(
        "Starting PARALLEL gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))
    data = psm.execute(data, f, cores, chunksize, "Gaussian", h)

    h.pstop(
        "Finished  gaussian filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    return data
