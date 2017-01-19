from __future__ import (absolute_import, division, print_function)
import numpy as np
from recon.helper import Helper


def _rotate_image(data, rotation):
    # rot90 rotates counterclockwise; config.pre.rotation rotates clockwise
    data[:, :] = np.rot90(data[:, :], rotation)
    return data


def _rotate_stack(data, rotation):
    """
    WARNING: ONLY WORKS FOR SQUARE IMAGES
    Rotate every image of a stack

    :param data :: image stack as a 3d numpy array
    :param rotation :: rotation for the image

    Returns :: rotated data (stack of images)
    """

    for idx in range(0, data.shape[0]):
        _rotate_image(data[idx], rotation)

    return data


def execute(data, rotation, flat=None, dark=None, h=None):
    """
    Rotates a stack (sample, flat and dark images).
    This function is usually used on the whole picture, which is a square.
    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    :param data :: stack of sample images
    :param rotation: The rotation to be performed
    :param flat :: flat images average
    :param dark :: dark images average
    :param h: Helper class, if not provided will be initialised with empty constructor

    Returns :: rotated images
    """

    h = Helper.empty_init() if h is None else h
    h.check_data_stack(data)

    if rotation:

        # numpy.rot90 rotates counterclockwise
        # this reverses it, so we rotate clockwise
        clockwise_rotations = 4 - rotation

        h.pstart(
            "Starting rotation step ({0} degrees clockwise), with pixel data type: {1}...".
            format(clockwise_rotations * 90, data.dtype))

        data = _rotate_stack(data, clockwise_rotations)
        if flat is not None:
            flat = _rotate_image(flat, clockwise_rotations)
        if dark is not None:
            dark = _rotate_image(dark, clockwise_rotations)

        h.pstop("Finished rotation step ({0} degrees clockwise), with pixel data type: {1}."
                .format(clockwise_rotations * 90, data.dtype))

        h.check_data_stack(data)
    else:
        h.tomo_print_note(
            "NOT rotating the input images, because no valid -r/--rotation was specified.")

    return data, flat, dark
