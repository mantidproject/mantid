from __future__ import (absolute_import, division, print_function)
import numpy as np
from recon.helper import Helper


def execute(data, circular_mask_ratio, h=None):
    h = Helper.empty_init() if h is None else h

    if circular_mask_ratio and 0 < circular_mask_ratio < 1:
        from recon.tools import tool_importer
        tomopy = tool_importer.do_importing('tomopy')

        h.pstart("Starting circular mask...")
        tomopy.circ_mask(arr=data, axis=0, ratio=circular_mask_ratio, val=0.)
        h.pstart("Finished applying circular mask.")
    else:
        h.tomo_print_note(
            "Not applying circular mask, no --circular-mask was specified.")

    return data


def execute_custom(data_vol, ratio=1.0, mask_out_val=0.0):
    """
    Applies a circular mask on a 3D volume. The mask is applied along the z axis (first
    dimension of the numpy shape)

    @param data_vol :: 3D data volume
    @param ratio :: radius of the mask relative to the radius of the smallest from the
    x and y dimensions/edges
    @param mask_out_val :: value to use when masking out pixels outside of the mask radius

    Returns :: masked volume
    """
    if not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError(
            "Wrong data volume when trying to apply a circular mask: {0}".
            format(data_vol))

    edge_z, edge_y, edge_x = data_vol.shape
    mask_in = _calc_mask(edge_y, edge_x, ratio)
    for idx in range(edge_z):
        data_vol[idx, ~mask_in] = mask_out_val

    return data_vol


def _calc_mask(ydim, xdim, ratio):
    """
    Prepare a mask object.

    :param ydim :: size/length of the y dimension (image rows)
    :param xdim :: size/length of the x dimension (innermost, image columns)
    :param ratio :: ratio in [0,1] relative to the smaller dimension

    Returns :: mask as a numpy array of boolean values (in/out-side mask)
    """
    radius_y = ydim / 2.0
    radius_x = xdim / 2.0
    if ydim < xdim:
        small_radius2 = radius_y * radius_y
    else:
        small_radius2 = radius_x * radius_x

    y_mask, x_mask = np.ogrid[0.5 - radius_y:0.5 + radius_y, 0.5 - radius_x:0.5
                              + radius_x]

    small_radius2 *= ratio * ratio
    return (y_mask * y_mask + x_mask * x_mask) < (small_radius2)
