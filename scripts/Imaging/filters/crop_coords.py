from __future__ import (absolute_import, division, print_function)
import numpy as np
from helper import Helper


def execute(sample, region_of_interest, flat, dark, h=None):
    """
    Execute the Circular Mask filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param region_of_interest: The region of interest that will be cropped
    :param flat: The average flat image to be cropped
    :param dark: The average dark image to be cropped
    :param h: Helper class, if not provided will be initialised with empty constructor

    :return: the data after being processed with the filter
    """
    if sample is not None and flat is None and dark is not None:
        return execute_volume(sample, region_of_interest, h), None, None
    else:
        return execute_volume(sample, region_of_interest, h), \
               execute_image(flat, region_of_interest, h), \
               execute_image(dark, region_of_interest, h)


def _crop_coords_sanity_checks(coords, data_image, expected_data_shape=3):
    # if nothing is provided make the user aware
    if not isinstance(coords, list) or 4 != len(coords):
        raise ValueError(
            "Error in crop (region of interest) parameter (expecting a list with four integers) "
            "[left, top, right, bottom]. Got: {0} with {1}".format(
                coords, type(coords)))

    elif not all(isinstance(crd, int) for crd in coords):
        raise ValueError(
            "Cannot use non-integer coordinates to crop images. Got "
            "these coordinates: {0}".format(coords))

    elif not isinstance(data_image, np.ndarray) or expected_data_shape != len(
            data_image.shape):
        raise ValueError(
            "Wrong data volume when trying to crop: {0}".format(data_image))


def execute_image(data, region_of_interest, h=None):
    """

    :param data :: image as a 2d numpy array
    :param region_of_interest: coordinates which will be cropped and returned for further processing
    :param h: Helper class, if not provided will be initialised with empty constructor

    :returns :: cropped data (stack of images)

    """
    h = Helper.empty_init() if h is None else h

    if region_of_interest:
        h.pstart("Starting image cropping with coordinates: {0}. ...".format(
            region_of_interest))

        data = _crop_image(data, region_of_interest)

        h.pstop(
            "Finished image cropping with pixel data type: {0}, resulting shape: {1}.".
            format(data.dtype, data.shape))

    else:
        h.tomo_print_note(
            "NOT applying cropping to region of interest on single image, because no --region-of-interest coordinates were given."
        )

    return data


def _crop_image(data_image, coords):
    """
    Crops a data volume by a rectangle defined by two corner
    coordinates. Crops along the z axis (outermost numpy array index)

    :param data_image :: image data to be cropped
    :param coords :: coordinates of the corners that define a rectangle box (crop to this
    box, as when cropping to the regions of interest).
    :returns :: cropped image
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
        return data_image[top:bottom, left:right]


def execute_volume(data, crop_coords, h=None):
    """
    Crop stack of images to a region (region of interest or similar), image by image

    :param data :: stack of images as a 3d numpy array
    :param crop_coords: coordinates which will be cropped and returned for further processing
    :param h: Helper class, if not provided will be initialised with empty constructor

    :returns :: cropped data (stack of images)
    """
    h.check_data_stack(data)

    # list with left, top, right, bottom
    if crop_coords:
        h.pstart("Starting data volume cropping with coordinates: {0}. ...".
                 format(crop_coords))

        data = _crop_volume(data, crop_coords)

        h.pstop(
            "Finished data volume cropping with pixel data type: {0}, resulting shape: {1}.".
            format(data.dtype, data.shape))

    else:
        h.tomo_print_note(
            "NOT applying cropping to region of interest on data volume, because no --region-of-interest coordinates were given."
        )

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
