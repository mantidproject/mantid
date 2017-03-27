from __future__ import (absolute_import, division, print_function)
import helper as h


def cli_register(parser):
    parser.add_argument(
        "--ring-removal",
        required=False,
        action='store_true',
        help='Perform Ring Removal on the post processed data.')

    parser.add_argument(
        "--ring-removal-x",
        type=int,
        required=False,
        help='Abscissa location of center of rotation')

    parser.add_argument(
        "--ring-removal-y",
        type=int,
        required=False,
        help='Ordinate location of center of rotation')

    parser.add_argument(
        "--ring-removal-thresh",
        type=float,
        required=False,
        help='Maximum value of an offset due to a ring artifact')

    parser.add_argument(
        "--ring-removal-thresh-max",
        type=float,
        required=False,
        help='Max value for portion of image to filter')

    parser.add_argument(
        "--ring-removal-thresh-min",
        type=float,
        required=False,
        help='Min value for portion of image to filter')

    parser.add_argument(
        "--ring-removal-theta-min",
        type=int,
        required=False,
        help='Minimum angle in degrees (int) to be considered ring artifact')

    parser.add_argument(
        "--ring-removal-rwidth",
        type=int,
        required=False,
        help='Maximum width of the rings to be filtered in pixels')
    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def execute(data,
            run_ring_removal=False,
            center_x=None,
            center_y=None,
            thresh=300.0,
            thresh_max=300.0,
            thresh_min=-100.0,
            theta_min=30,
            rwidth=30,
            cores=None,
            chunksize=None):
    """
    Removal of ring artifacts in reconstructed volume.

    :param data :: stack of projection images as 3d data (dimensions z, y, x), with
    z different projections angles, and y and x the rows and columns of individual images.

    :param run_ring_removal :: 'wf': Wavelet-Fourier based run_ring_removal

    Returns :: filtered data hopefully without stripes which should dramatically decrease
    ring artifacts after reconstruction and the effect of these on post-processing tasks
    such as segmentation of the reconstructed 3d data volume.
    """

    if run_ring_removal:
        h.check_data_stack(data)

        h.pstart("Starting ring removal...")
        import tomopy.misc.corr
        data = tomopy.misc.corr.remove_ring(
            data,
            center_x=center_x,
            center_y=center_y,
            thresh=thresh,
            thresh_max=thresh_max,
            thresh_min=thresh_min,
            theta_min=theta_min,
            rwidth=rwidth,
            ncore=cores,
            nchunk=chunksize)
        h.pstop("Finished ring removal...")

    return data
