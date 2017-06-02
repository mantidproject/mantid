from __future__ import (absolute_import, division, print_function)
import helper as h
import scipy.misc
from core.parallel import exclusive_mem as pem
from core.parallel import utility as pu


def cli_register(parser):
    parser.add_argument(
        "--rebin",
        required=False,
        type=float,
        help="Rebin factor by which the images will be rebinned. This could be any positive float number.\n"
        "If not specified no scaling will be done.")

    parser.add_argument(
        "--rebin-mode",
        required=False,
        type=str,
        default=modes()[0],
        choices=modes(),
        help="Default: %(default)s\n"
        "Specify which interpolation mode will be used for the scaling of the image."
    )

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def modes():
    return ['nearest', 'lanczos', 'bilinear', 'bicubic', 'cubic']


def execute(data, rebin_param, mode, cores=None, chunksize=None):
    """
    Execute the Rebin/imresize filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param rebin_param: int, float or tuple
                        int - Percentage of current size.
                        float - Fraction of current size.
                        tuple - Size of the output image.
    :param mode: Interpolation to use for re-sizing ('nearest', 'lanczos', 'bilinear', 'bicubic' or 'cubic').
    :param cores: The number of cores that will be used to process the data.
    :param chunksize: The number of chunks that each worker will receive.

    :return: the data after being processed with the filter

    Full reference:
    https://docs.scipy.org/doc/scipy-0.18.1/reference/generated/scipy.misc.imresize.html

    Example command line:
    python main.py -i /some/data --rebin 0.5 --rebin-mode 'nearest'
    """
    h.check_data_stack(data)
    if rebin_param and 0 < rebin_param:
        if pu.multiprocessing_available():
            data = _execute_par(data, rebin_param, mode, cores, chunksize)
        else:
            data = _execute_seq(data, rebin_param, mode)

    return data


def _execute_par(data, rebin_param, mode, cores=None, chunksize=None):
    resized_data = _create_reshaped_array(data.shape, rebin_param)

    h.pstart("Starting PARALLEL image rebinning.")

    f = pem.create_partial(scipy.misc.imresize, size=rebin_param, interp=mode)
    resized_data = pem.execute(
        data, f, cores, chunksize, "Rebinning", output_data=resized_data)

    h.pstop("Finished PARALLEL image rebinning. New shape: {0}".format(
        resized_data.shape))

    return resized_data


def _execute_seq(data, rebin_param, mode):
    h.pstart("Starting image rebinning.")
    resized_data = _create_reshaped_array(data.shape, rebin_param)
    num_images = resized_data.shape[0]
    h.prog_init(num_images, "Rebinning")
    for idx in range(num_images):
        resized_data[idx] = scipy.misc.imresize(
            data[idx], rebin_param, interp=mode)

        h.prog_update()

    h.prog_close()
    h.pstop(
        "Finished image rebinning. New shape: {0}".format(resized_data.shape))

    return resized_data


def _create_reshaped_array(old_shape, rebin_param):

    num_images = old_shape[0]
    # use SciPy's calculation to find the expected dimensions
    # int to avoid visible deprecation warning
    expected_dimy = int(rebin_param * old_shape[1])
    expected_dimx = int(rebin_param * old_shape[2])

    # allocate memory for images with new dimensions
    return pu.create_shared_array((num_images, expected_dimy, expected_dimx))
