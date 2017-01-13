from __future__ import (absolute_import, division, print_function)
import numpy as np


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


def execute(data, config, flat=None, dark=None):
    """
    Rotates a stack (sample, flat and dark images).
    This function is usually used on the whole picture, which is a square.
    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    :param data :: stack of sample images
    :param config :: pre-processing configuration
    :param flat :: stack of flat images
    :param dark :: stack of dark images

    Returns :: rotated images
    """

    from recon.helper import Helper
    h = Helper(config)
    h.check_data_stack(data)

    if not config.pre.rotation or config.pre.rotation < 0:
        h.tomo_print_note(
            "NOT rotating the input images, because no valid -r/--rotation was specified.")
        return data, flat, dark

    rotation = config.pre.rotation
    counterclock_rotations = 4 - rotation

    h.pstart(
        " * Starting rotation step ({0} degrees clockwise), with pixel data type: {1}...".
        format(counterclock_rotations * 90, data.dtype))

    data = _rotate_stack(data, counterclock_rotations)
    if flat is not None:
        flat = _rotate_image(flat, counterclock_rotations)
    if dark is not None:
        dark = _rotate_image(dark, counterclock_rotations)

    h.pstop(" * Finished rotation step ({0} degrees clockwise), with pixel data type: {1}."
            .format(counterclock_rotations * 90, data.dtype))

    h.check_data_stack(data)

    return data, flat, dark
