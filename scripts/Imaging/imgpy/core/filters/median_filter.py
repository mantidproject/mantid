from __future__ import (absolute_import, print_function, division)
import helper as h
import scipy.ndimage as scipy_ndimage
from core.parallel import shared_mem as psm
from core.parallel import utility as pu


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
    :param size: Size of the kernel
    :param mode: The mode with which to handle the endges. One of [reflect, constant, nearest, mirror, wrap].
    :param cores: The number of cores that will be used to process the data.
    :param chunksize: The number of chunks that each worker will receive.

    :return: Returns the processed data

    Full reference:
    https://docs.scipy.org/doc/scipy-0.16.1/reference/generated/scipy.ndimage.filters.median_filter.html

    python main.py -i /some/data --pre-median-size 3
    python main.py -i /some/data --pre-median-size 3 --pre-median-mode 'nearest'
    python main.py -i /some/data --pre-median-size 3 --pre-median-mode 'nearest'
    
    python main.py -i /some/data --post-median-size 3
    python main.py -i /some/data --post-median-size 3 --post-median-mode 'nearest'
    python main.py -i /some/data --post-median-size 3 --post-median-mode 'nearest'
    """
    h.check_data_stack(data)

    if size and size > 1:
        if pu.multiprocessing_available():
            data = _execute_par(data, size, mode, cores, chunksize)
        else:
            data = _execute_seq(data, size, mode)

    h.check_data_stack(data)
    return data


def _execute_seq(data, size, mode):
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
    # create the partial function to forward the parameters
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
