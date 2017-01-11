from __future__ import (absolute_import, division, print_function)


def _rotate_imgs(data, rotation):
    """
    NOTE: ONLY WORKS FOR SQUARE IMAGES
    Rotate every image of a stack

    :param data :: image stack as a 3d numpy array
    :param rotation :: rotation for the image

    Returns :: rotated data (stack of images)
    """

    import numpy as np

    for idx in range(0, data.shape[0]):
        # rot90 rotates counterclockwise; config.pre.rotation rotates clockwise
        counterclock_rotations = 4 - rotation
        data[idx, :, :] = np.rot90(data[idx, :, :], counterclock_rotations)

    return data


def execute(data, config, white=None, dark=None):
    """
    Rotates a stack (sample, white and dark images).
    This function is usually used on the whole picture, which is a square.
    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    :param data :: stack of sample images
    :param config :: pre-processing configuration
    :param white :: stack of white images
    :param dark :: stack of dark images

    Returns :: rotated images
    """
    from recon.configs.recon_config import ReconstructionConfig
    if not config or not isinstance(config, ReconstructionConfig):
        raise ValueError(
            "Cannot rotate images without a valid pre-processing configuration"
        )

    from recon.helper import Helper
    h = Helper(config)
    h.check_data_stack(data)

    if not config.pre.rotation or config.pre.rotation < 0:
        h.tomo_print(" * Note: NOT rotating the input images.")
        return data, white, dark

    rotation = config.pre.rotation
    h.pstart(
        " * Starting rotation step ({0} degrees clockwise), with pixel data type: {1}...".
        format(rotation * 90, data.dtype))

    data = _rotate_imgs(data, rotation)
    if white:
        white = _rotate_imgs(white, rotation)
    if dark:
        dark = _rotate_imgs(dark, rotation)

    h.pstop(" * Finished rotation step ({0} degrees clockwise), with pixel data type: {1}."
            .format(rotation * 90, data.dtype))

    h.check_data_stack(data)

    return data, white, dark
