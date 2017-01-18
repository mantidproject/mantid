from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper


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


def execute(data, size, mode, h=None):
    """

    :param data: The data that will be processed with this filter
    :param size: Size of the median filter kernel
    :param mode: Mode for the borders of the median filter. Options available in script -h command
    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: Returns the processed data
    """
    h = Helper.empty_init() if h is None else h
    if size and size > 1:
        scipy_ndimage = import_scipy_ndimage()
        h.pstart(
            "Starting noise filter / median, with pixel data type: {0}, filter size/width: {1}.".
            format(data.dtype, size))

        for idx in range(0, data.shape[0]):
            data[idx] = scipy_ndimage.median_filter(
                data[idx], size, mode=mode)

        h.pstop(
            "Finished noise filter / median, with pixel data type: {0}, filter size/width: {1}.".
            format(data.dtype, size))

    else:
        h.tomo_print_note("NOT applying noise filter / median.")

    return data
