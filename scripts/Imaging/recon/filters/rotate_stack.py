from __future__ import (absolute_import, division, print_function)
import numpy as np
from recon.helper import Helper


def _rotate_image_inplace(data, rotation=None):
    # rot90 rotates counterclockwise; config.pre.rotation rotates clockwise
    data[:, :] = np.rot90(data[:, :], rotation)


def _rotate_image(data, rotation=None):
    # rot90 rotates counterclockwise; config.pre.rotation rotates clockwise
    return np.rot90(data[:, :], rotation)


def execute(data, rotation, flat=None, dark=None, cores=8, chunksize=None, h=None):
    """
    Rotates a stack (sample, flat and dark images).

    This function must be used on the whole picture, which is a square.

    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    :param data :: stack of sample images
    :param rotation: The rotation to be performed
    :param flat :: flat images average
    :param dark :: dark images average
    :param cores :: cores for parallel execution
    :param chunksize :: chunk for each worker
    :param h: Helper class, if not provided will be initialised with empty constructor

    :return :: rotated images
    """
    h = Helper.empty_init() if h is None else h
    h.check_data_stack(data)
    if rotation:
        clockwise_rotations = 4 - rotation

        from parallel import utility as pu
        if pu.multiprocessing_available():
            _execute_par(data, clockwise_rotations, cores, chunksize, h)
        else:
            _execute_seq(data, clockwise_rotations, h)

        if flat is not None:
            flat = _rotate_image(flat, clockwise_rotations)
        if dark is not None:
            dark = _rotate_image(dark, clockwise_rotations)
    else:
        h.tomo_print_note(
            "NOT rotating the input images, because no valid -r/--rotation was specified.")

    h.check_data_stack(data)
    return data, flat, dark


def _execute_seq(data, rotation, h=None):
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
    h.pstart(
        "Starting rotation step ({0} degrees clockwise), with pixel data type: {1}...".
        format(rotation * 90, data.dtype))

    img_count = data.shape[0]
    h.prog_init(img_count, "Rotating stack")
    for idx in range(0, img_count):
        data[idx] = _rotate_image(data[idx], rotation)
        h.prog_update(1)

    h.prog_close()

    h.pstop("Finished rotation step ({0} degrees clockwise), with pixel data type: {1}."
            .format(rotation * 90, data.dtype))

    return data


def _execute_par(data, rotation, cores=8, chunksize=None, h=None):

    h.pstart(
        "Starting PARALLEL rotation step ({0} degrees clockwise), with pixel data type: {1}...".
        format(rotation * 90, data.dtype))

    from parallel import shared_mem as psm

    f = psm.create_partial(_rotate_image_inplace,
                           fwd_function=psm.inplace_fwd_func, rotation=rotation)

    data = psm.execute(data, f, cores=cores,
                       chunksize=chunksize, name="Rotation", h=h)

    h.pstop("Finished PARALLEL rotation step ({0} degrees clockwise), with pixel data type: {1}."
            .format(rotation * 90, data.dtype))

    return data
