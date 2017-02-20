from __future__ import (absolute_import, print_function, division)
from helper import Helper

def modes():
    return ['reflect', 'constant', 'nearest', 'mirror', 'wrap']

def execute(data, size, mode, cores=None, chunksize=None, h=None):
    """
    Execute the Median filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: The size of the kernel
    :param mode: The mode with which to handle the edges

    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: the data after being processed with the filter

    Full reference:
    https://docs.scipy.org/doc/scipy-0.16.1/reference/generated/scipy.ndimage.filters.median_filter.html

    """
    h = Helper.empty_init() if h is None else h
    h.check_data_stack(data)

    if size and size > 1:
        from parallel import utility as pu
        if pu.multiprocessing_available():
            data = _execute_par(data, size, mode, cores, chunksize, h)
        else:
            data = _execute_seq(data, size, mode, h)

    else:
        h.tomo_print_note("NOT applying noise filter / median.")

    h.check_data_stack(data)
    return data


def _execute_seq(data, size, mode, h=None):
    """
    Sequential version of the Median Filter using scipy.ndimage

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: Size of the median filter kernel
    :param mode: Mode for the borders of the median filter. Options available in script -h command
    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: Returns the processed data
    """
    h = Helper.empty_init() if h is None else h
    scipy_ndimage = import_scipy_ndimage()
    h.pstart(
        "Starting median filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    h.prog_init(data.shape[0], "Median filter")
    for idx in range(0, data.shape[0]):
        data[idx] = scipy_ndimage.median_filter(data[idx], size, mode=mode)
        h.prog_update()
    h.prog_close()

    h.pstop(
        "Finished median filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))
    return data


def _execute_par(data, size, mode, cores=None, chunksize=None, h=None):
    """
    Parallel version of the Median Filter using scipy.ndimage

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: Size of the median filter kernel
    :param mode: Mode for the borders of the median filter. Options available in script -h command
    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: Returns the processed data
    """

    h = Helper.empty_init() if h is None else h
    scipy_ndimage = import_scipy_ndimage()

    from parallel import shared_mem as psm
    f = psm.create_partial(
        scipy_ndimage.median_filter,
        fwd_func=psm.return_fwd_func,
        size=size,
        mode=mode)

    h.pstart(
        "Starting PARALLEL median filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    data = psm.execute(data, f, cores, chunksize, "Median Filter", h)

    h.pstop(
        "Finished PARALLEL median filter, with pixel data type: {0}, filter size/width: {1}.".
        format(data.dtype, size))

    return data


def import_scipy_ndimage():
    """
    Tries to import scipy so that the median filter can be applied
    :return:
    """

    try:
        import scipy.ndimage as scipy_ndimage
    except ImportError:
        raise ImportError(
            "Could not find the subpackage scipy.ndimage, required for image pre-/post-processing"
        )

    return scipy_ndimage
