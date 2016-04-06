# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>

import numpy as np

def scale_down(data_vol, block_size, method='average'):
    """
    Downscale to for example shrink 1Kx1K images to 512x512

    @param data_vol :: 3d volume to downscale
    @param block_size :: make block_size X block_size blocks to downscale
    @param method :: either 'average' (default) or 'sum' to calculate average or sum of blocks

    Returns :: downscaled volume, with the dimensions implied by block_size, and the
    same data type as the input data volume.
    """
    if not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError("Wrong data volume when trying to crop (expected a 3d numpy array): {0}".
                         format(data_vol))
    if block_size > data_vol.shape[1] or block_size > data_vol.shape[2]:
        raise ValueError("Block size too large when trying to crop data volume. Block size: {0}, "
                         "data dimensions: {1}".format(block_size, data_vol.shape))

    if 0 != np.mod(data_vol.shape[1], block_size) or 0 != np.mod(data_vol.shape[2], block_size):
        raise ValueError("The block size ({0}) must be an exact integer divisor of the sizes of the "
                         "x and y dimensions ({1} and {2} of the input data volume".
                         format(data_vol.shape[2], data_vol.shape[1], block_size))

    supported_methods = ['average', 'sum']
    if method.lower() not in supported_methods:
        raise ValueError("The method to combine pixels in blocks must be one of {0}. Got unknown "
                         "value: {1}".format(supported_methods, method))

    rescaled_vol = np.zeros((data_vol.shape[0], data_vol.shape[1]//block_size,
                             data_vol.shape[2]//block_size), dtype=data_vol.dtype)
    # for block averages in every slice/image along the vertical/z axis
    tmp_shape = rescaled_vol.shape[1], block_size, rescaled_vol.shape[2], block_size
    for vert_slice in range(len(rescaled_vol)):
        vsl = data_vol[vert_slice, :, :]
        if 'average' == method:
            rescaled_vol[vert_slice, :, :] = vsl.reshape(tmp_shape).mean(-1).mean(1)
        elif 'sum' == method:
            rescaled_vol[vert_slice, :, :] = vsl.reshape(tmp_shape).mean(-1).mean(1)

    return rescaled_vol

def crop_vol(data_vol, coords):
    """
    Crops a data volume by a rectangle defined by two corner
    coordinates. Crops along the z axis (outermost numpy array index)

    @param data_vol :: 3D data volume
    @param coords :: coordinates of the corners that define a rectangle box (crop to this
    box, as when cropping to the regions of interest). Given as list [x1, y1, x2, y2]

    Returns :: cropped data volume
    """
    cropped_data = None
    if not isinstance(coords, list) or 4 != len(coords):
        raise ValueError("Wrong coordinates object when trying to crop: {0}".format(coords))
    elif not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError("Wrong data volume when trying to crop: {0}".format(data_vol))
    elif not any(coords) or coords[1] > coords[3] or coords[0] > coords[2]:
        # skip if for example: 0, 0, 0, 0 (empty selection)
        return data_vol
    elif not all(isinstance(crd, int) for crd in coords):
        raise ValueError("Cannot use non-integer coordinates to crop images. Got "
                         "these coordinates: {0}".format(coords))
    else:
        cropped_data = data_vol[:, coords[1]:(coords[3]+1), coords[0]:(coords[2]+1)]

    return cropped_data

def remove_stripes_ring_artifacts(data_vol, method='wavelet-fourier'):
    """
    Removal of stripes in sinograms / ring artifacts in reconstructed
    volume.

    This is an unimplemented stub at the moment.
    As first step it should implement one methods: the combined wavelet-Fourier method
    (Muench et al. 2009, Opt Express, 17(10), 8567-91), as implemented also in tomopy.

    @param data_vol :: stack of projection images as 3d data (dimensions z, y, x), with
    z different projections angles, and y and x the rows and columns of individual images.

    @param method :: 'wf': Wavelet-Fourier based method

    Returns :: filtered data hopefully without stripes which should dramatically decrease
    ring artifacts after reconstruction and the effect of these on post-processing tasks
    such as segmentation of the reconstructed 3d data volume.
    """
    supported_methods = ['wavelet-fourier']

    if not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError("Wrong data volume when trying to filter stripes/ring artifacts: {0}".
                         format(data_vol))

    if method.lower() not in supported_methods:
        raise ValueError("The method to remove stripes and ring artifacts must be one of {0}. "
                         "Got unknown value: {1}".format(supported_methods, method))

    try:
        import tomopy
        stripped_vol = tomopy.prep.stripe.remove_stripe_fw(data_vol)
    except ImportError:
        stripped_vol = remove_sino_stripes_rings_wf(data_vol)

    return stripped_vol

def remove_sino_stripes_rings_wf(data_vol, wv_levels=None):
    if not wv_levels:
        max_len = np.max(data_vol.shape)
        wv_levels = int(np.ceil(np.log2(max_len)))

    from . import filters_adv
    return filters_adv.remove_sino_stripes_rings_wf(data_vol, wv_levels)

def circular_mask(data_vol, ratio=1.0, mask_out_val=0.0):
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
        raise ValueError("Wrong data volume when trying to apply a circular mask: {0}".
                         format(data_vol))

    edge_z, edge_y, edge_x = data_vol.shape
    mask_in = _calc_mask(edge_y, edge_x, ratio)
    for idx in range(edge_z):
        data_vol[idx, ~mask_in] = mask_out_val

    return data_vol

def _calc_mask(ydim, xdim, ratio):
    """
    Prepare a mask object.

    @param ydim :: size/length of the y dimension (image rows)
    @param xdim :: size/length of the x dimension (innermost, image columns)
    @param ratio :: ratio in [0,1] relative to the smaller dimension

    Returns :: mask as a numpy array of boolean values (in/out-side mask)
    """
    radius_y = ydim/2.0
    radius_x = xdim/2.0
    if ydim < xdim:
        small_radius2 = radius_y * radius_y
    else:
        small_radius2 = radius_x * radius_x

    y_mask, x_mask = np.ogrid[0.5 - radius_y:0.5 + radius_y,
                              0.5 - radius_x:0.5 + radius_x]

    small_radius2 *= ratio * ratio
    return (y_mask*y_mask + x_mask*x_mask) < (small_radius2)
