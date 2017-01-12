from __future__ import (absolute_import, division, print_function)
import numpy as np


def _crop_coords_sanity_checks(coords, data_image, expected_data_shape=3):
    # if nothing is provided make the user aware
    if not isinstance(coords, list) or 4 != len(coords):
        raise ValueError(
            "Wrong coordinates object when trying to crop: {0}".format(coords))

    elif not all(isinstance(crd, int) for crd in coords):
        raise ValueError(
            "Cannot use non-integer coordinates to crop images. Got "
            "these coordinates: {0}".format(coords))

    elif not isinstance(data_image, np.ndarray) or expected_data_shape != len(data_image.shape):
        raise ValueError("Wrong data volume when trying to crop: {0}".format(data_image))


def execute_image(data, config):
    from recon.helper import Helper
    h = Helper(config)

    crop_coords = config.pre.crop_coords
    if crop_coords:
        try:
            h.pstart(
                " * Starting image cropping with coordinates: {0}. ...".format(crop_coords))

            data = _crop_image(data, crop_coords)

            h.pstop(" * Finished image cropping with pixel data type: {0}, resulting shape: {1}.".format(data.dtype,
                                                                                                         data.shape))

        except ValueError as exc:
            h.tomo_print(
                "Error in crop (region of interest) parameter (expecting a list with four integers. "
                "Got: {0}. Error details: ".format(crop_coords), exc)
    else:
        h.tomo_print(" * Note: NOT applying cropping to region of interest.")


def _crop_image(data_image, coords):
    """
    Crops a data volume by a rectangle defined by two corner
    coordinates. Crops along the z axis (outermost numpy array index)

    @param data_vol :: 3D data volume
    @param coords :: coordinates of the corners that define a rectangle box (crop to this
    box, as when cropping to the regions of interest).
    Returns :: cropped data volume
    """
    _crop_coords_sanity_checks(coords, data_image, expected_data_shape=2)
    left = coords[0]
    top = coords[1]
    right = coords[2]
    bottom = coords[3]

    if not any(coords) or top > bottom or left > right:
        # skip if for example: 0, 0, 0, 0 (empty selection)
        return data_image
    else:
        return data_image[:, top:bottom, left:right]


def execute_volume(data, config):
    """
    Crop stack of images to a region (region of interest or similar), image by image

    @param data :: stack of images as a 3d numpy array
    @param config :: reconstruction configuration

    Returns :: filtered data (stack of images)
    """
    from recon.helper import Helper
    h = Helper(config)
    h.check_data_stack(data)

    # list with first-x, first-y, second-x, second-y
    crop_coords = config.pre.crop_coords
    if crop_coords:
        try:
            h.pstart(
                " * Starting image cropping with coordinates: {0}. ...".format(crop_coords))

            data = _crop_volume(data, crop_coords)

            h.pstop(" * Finished image cropping with pixel data type: {0}, resulting shape: {1}.".format(data.dtype,
                                                                                                         data.shape))

        except ValueError as exc:
            h.tomo_print(
                "Error in crop (region of interest) parameter (expecting a list with four integers. "
                "Got: {0}. Error details: ".format(crop_coords), exc)
    else:
        h.tomo_print(" * Note: NOT applying cropping to region of interest.")

    h.check_data_stack(data)

    return data


def _crop_volume(data_vol, coords):
    """
    Crops a data volume by a rectangle defined by two corner
    coordinates. Crops along the z axis (outermost numpy array index)

    @param data_vol :: 3D data volume
    @param coords :: coordinates of the corners that define a rectangle box (crop to this
    box, as when cropping to the regions of interest).
    Returns :: cropped data volume
    """
    _crop_coords_sanity_checks(coords, data_vol)

    left = coords[0]
    top = coords[1]
    right = coords[2]
    bottom = coords[3]

    if not any(coords) or top > bottom or left > right:
        # skip if for example: 0, 0, 0, 0 (empty selection)
        return data_vol
    else:
        return data_vol[:, top:bottom, left:right]
