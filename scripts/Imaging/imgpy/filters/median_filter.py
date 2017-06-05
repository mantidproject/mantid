from __future__ import (absolute_import, print_function, division)

import helper as h


def cli_register(parser):
    default_size = None
    size_help = "Apply median filter (2d) on reconstructed volume with the given kernel size."
    mode_help = "Default: %(default)s\n"\
        "Mode of median filter which determines how the array borders are handled."

    parser.add_argument(
        "--pre-median-size",
        type=int,
        required=False,
        default=default_size,
        help=size_help)

    parser.add_argument(
        "--pre-median-mode",
        type=str,
        required=False,
        default=modes()[0],
        choices=modes(),
        help=mode_help)

    parser.add_argument(
        "--post-median-size",
        required=False,
        type=float,
        default=default_size,
        help=size_help)

    parser.add_argument(
        "--post-median-mode",
        type=str,
        required=False,
        default=modes()[0],
        choices=modes(),
        help=mode_help)

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def modes():
    return ['reflect', 'constant', 'nearest', 'mirror', 'wrap']


def execute(data, size, mode, cores=None, chunksize=None):
    """
    Execute the Median filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: The size of the kernel
    :param mode: The mode with which to handle the edges

    :return: the data after being processed with the filter

    Full reference:
    https://docs.scipy.org/doc/scipy-0.16.1/reference/generated/scipy.ndimage.filters.median_filter.html

    """
    h.check_data_stack(data)

    if size and size > 1:
        from parallel import utility as pu
        if pu.multiprocessing_available():
            data = _execute_par(data, size, mode, cores, chunksize)
        else:
            data = _execute_seq(data, size, mode)

    h.check_data_stack(data)
    return data


def _execute_seq(data, size, mode):
    """
    Sequential version of the Median Filter using scipy.ndimage

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: Size of the median filter kernel
    :param mode: Mode for the borders of the median filter.

    :return: Returns the processed data
    """
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


def _execute_par(data, size, mode, cores=None, chunksize=None):
    """
    Parallel version of the Median Filter using scipy.ndimage

    :param data: The sample image data as a 3D numpy.ndarray
    :param size: Size of the median filter kernel
    :param mode: Mode for the borders of the median filter.

    :return: Returns the processed data
    """

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

    data = psm.execute(data, f, cores, chunksize, "Median Filter")

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
