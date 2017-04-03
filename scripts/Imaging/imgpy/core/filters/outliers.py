from __future__ import (absolute_import, division, print_function)
import helper as h
from core.tools import importer


def cli_register(parser):
    parser.add_argument(
        "--pre-outliers",
        required=False,
        type=float,
        help="Pixel difference above which to crop bright pixels.")

    parser.add_argument(
        "--pre-outliers-radius",
        required=False,
        type=int,
        help="Radius for the median filter to determine the outlier.")

    parser.add_argument(
        "--post-outliers",
        required=False,
        type=float,
        help="Outliers threshold for reconstructed volume.\n"
        "Pixels below and/or above (depending on mode) this threshold will be clipped."
    )

    parser.add_argument(
        "--post-outliers-radius",
        required=False,
        type=int,
        help="Radius for the median filter to determine the outlier.")

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def modes():
    return ['dark', 'bright', 'both']


def execute(data, pixel_difference, radius, cores=None):
    """
    Execute the Outliers filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param pixel_difference: Pixel difference above which to crop bright pixels
    :param radius: Which pixels will be clipped: dark, bright or both
    :param cores: The number of cores that will be used to process the data.

    :return: the data after being processed with the filter

    Example command line:
    python main.py -i /some/data --pre-outliers 6 --pre-outliers-radius 4
    python main.py -i /some/data --post-outliers 6 --post-outliers-radius 4
    """

    if pixel_difference and radius and pixel_difference > 0 and radius > 0:
        h.pstart("Applying outliers with threshold: {0} and radius {1}".format(
            pixel_difference, radius))

        tomopy = importer.do_importing('tomopy')

        data[:] = tomopy.misc.corr.remove_outlier(
            data, pixel_difference, radius, ncore=cores)

        h.pstop("Finished outliers step, with pixel data type: {0}.".format(
            data.dtype))

    return data
