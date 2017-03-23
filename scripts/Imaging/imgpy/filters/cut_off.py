from __future__ import (absolute_import, division, print_function)


def cli_register(parser):
    parser.add_argument(
        "--cut-off",
        required=False,
        type=float,
        default=None,
        help="Default: %(default)s\n"
        "Cut off values above threshold relative to the max pixels.")

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def execute(data, threshold):
    """
    Execute the Cut off filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param threshold: The threshold related to the minimum pixel value that will be clipped

    :return: the data after being processed with the filter
    """
    import helper as h

    if threshold and threshold > 0.0:
        import numpy as np
        h.pstart("Applying cut-off with level: {0}".format(threshold))
        dmin = np.amin(data)
        dmax = np.amax(data)
        rel_cut_off = dmin + threshold * (dmax - dmin)

        data = np.minimum(data, rel_cut_off)

        h.pstop("Finished cut-off step, with pixel data type: {0}.".format(
            data.dtype))

    return data
