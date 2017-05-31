from __future__ import (absolute_import, division, print_function)
import numpy as np
import helper as h


def cli_register(parser):
    parser.add_argument(
        "-r",
        "--rotation",
        required=False,
        type=int,
        help="Rotate images by 90 degrees a number of times.\n"
        "The rotation is clockwise unless a negative number is given which indicates "
        "rotation counterclockwise.")

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def _rotate_image_inplace(data, rotation=None):
    # rot90 rotates counterclockwise; config.pre.rotation rotates clockwise
    data[:, :] = np.rot90(data[:, :], rotation)


def _rotate_image(data, rotation=None):
    # rot90 rotates counterclockwise; config.pre.rotation rotates clockwise
    return np.rot90(data[:, :], rotation)


def execute(data, rotation, flat=None, dark=None, cores=None, chunksize=None):
    """
    Rotates a stack (sample, flat and dark images).

    This function only works with square images.

    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    :param data: stack of sample images
    :param rotation: The rotation to be performed
    :param flat: flat images average
    :param dark: dark images average
    :param cores: cores for parallel execution
    :param chunksize: chunk for each worker

    :return: rotated images
    """
    h.check_data_stack(data)
    if rotation:
        clockwise_rotations = 4 - rotation

        from parallel import utility as pu
        if pu.multiprocessing_available():
            _execute_par(data, clockwise_rotations, cores, chunksize)
        else:
            _execute_seq(data, clockwise_rotations)

        if flat is not None:
            flat = _rotate_image(flat, clockwise_rotations)
        if dark is not None:
            dark = _rotate_image(dark, clockwise_rotations)

    h.check_data_stack(data)
    return data, flat, dark


def _execute_seq(data, rotation):
    """
    Rotates a stack (sample, flat and dark images).
    This function is usually used on the whole picture, which is a square.
    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    :param data: stack of sample images
    :param rotation: The rotation to be performed
    :param flat: flat images average
    :param dark: dark images average

    Returns: rotated images
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

    h.pstop(
        "Finished rotation step ({0} degrees clockwise), with pixel data type: {1}."
        .format(rotation * 90, data.dtype))

    return data


def _execute_par(data, rotation, cores=None, chunksize=None):
    h.pstart(
        "Starting PARALLEL rotation step ({0} degrees clockwise), with pixel data type: {1}...".
        format(rotation * 90, data.dtype))

    from parallel import shared_mem as psm

    f = psm.create_partial(
        _rotate_image_inplace,
        fwd_func=psm.inplace_fwd_func,
        rotation=rotation)

    data = psm.execute(
        data, f, cores=cores, chunksize=chunksize, name="Rotation")

    h.pstop(
        "Finished PARALLEL rotation step ({0} degrees clockwise), with pixel data type: {1}."
        .format(rotation * 90, data.dtype))

    return data
