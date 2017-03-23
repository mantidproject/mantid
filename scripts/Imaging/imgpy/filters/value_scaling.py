from __future__ import (absolute_import, division, print_function)
import numpy as np


def cli_register(parser):
    # this in an internal filter, doesn't have any external commands
    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def _calc_avg(data,
              roi_sums,
              roi_top=None,
              roi_left=None,
              roi_right=None,
              roi_bottom=None):
    return data[roi_top:roi_bottom, roi_left:roi_right].mean()


def create_factors(data, roi, cores=None, chunksize=None):
    """
    Calculate the scale factors as the mean of the ROI
    :param data: The data stack from which the scale factors will be calculated
    :return: The scale factor for each image.
    """
    from parallel import utility as pu
    img_num = data.shape[0]
    scale_factors = pu.create_shared_array((img_num, 1, 1))

    # turn into a 1D array, from the 3D that is returned
    scale_factors = scale_factors.reshape(img_num)

    # calculate the scale factor from original image
    from parallel import two_shared_mem as ptsm
    calc_sums_partial = ptsm.create_partial(
        _calc_avg,
        fwd_function=ptsm.fwd_func_return_to_second,
        roi_left=roi[0] if roi else 0,
        roi_top=roi[1] if roi else 0,
        roi_right=roi[2] if roi else data[0].shape[1] - 1,
        roi_bottom=roi[3] if roi else data[0].shape[0] - 1)

    data, scale_factors = ptsm.execute(data, scale_factors, calc_sums_partial,
                                       cores, chunksize,
                                       "Calculating scale factor sums")

    return scale_factors


def _scale_inplace(data, scale):
    np.multiply(data, scale, out=data[:])


def apply_factor(data, scale_factors, cores=None, chunksize=None):
    """
    This will apply the scale factors to the data stack.

    :param data: the data stack to which the scale factors will be applied.
    :param scale_factors: The scale factors to be applied
    """
    from parallel import two_shared_mem as ptsm
    # scale up the data
    scale_up_partial = ptsm.create_partial(
        _scale_inplace, fwd_function=ptsm.inplace_fwd_func_second_2d)

    # scale up all images by the mean sum of all of them, this will keep the contrast the same as from the region of interest
    data, scale_factors = ptsm.execute(data, [scale_factors.mean()],
                                       scale_up_partial, cores, chunksize,
                                       "Applying scale factor")

    return data
