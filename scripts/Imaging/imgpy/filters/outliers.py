from __future__ import (absolute_import, division, print_function)
import helper as h


def cli_register(parser):
    parser.add_argument(
        "--pre-outliers",
        required=False,
        type=float,
        help="Crop bright pixels.")

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


def execute(data, outliers_threshold, outliers_radius, cores=None):
    """
    Execute the Outliers filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param outliers_threshold: The threshold related to the pixel value that will be clipped
    :param outliers_radius: Which pixels will be clipped: dark, bright or both

    :return: the data after being processed with the filter
    """

    if outliers_threshold and outliers_radius and outliers_threshold > 0 and outliers_radius > 0:
        h.pstart("Applying outliers with threshold: {0} and radius {1}".format(
            outliers_threshold, outliers_radius))

        from tools import importer
        tomopy = importer.do_importing('tomopy')

        data[:] = tomopy.misc.corr.remove_outlier(
            data, outliers_threshold, outliers_radius, ncore=cores)

        h.pstop("Finished outliers step, with pixel data type: {0}.".format(
            data.dtype))

    return data
