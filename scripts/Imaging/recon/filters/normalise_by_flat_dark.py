from __future__ import (absolute_import, division, print_function)


def execute(data, config, norm_flat_img, norm_dark_img=0):
    """
    Normalise by flat and dark images

    :param data :: image stack as a 3d numpy array
    :param config :: pre-processing configuration
    :param norm_flat_img :: flat (open beam) image to use in normalization
    :param norm_dark_img :: dark image to use in normalization

    :returns :: filtered data (stack of images)
    """
    from recon.helper import Helper
    import numpy as np

    h = Helper(config)
    h.check_data_stack(data)

    if isinstance(norm_flat_img, np.ndarray):
        if 2 != len(norm_flat_img.shape):
            raise ValueError(
                "Incorrect shape of the flat image ({0}) which should match the "
                "shape of the sample images ({1})".format(
                    norm_flat_img.shape, data[0].shape))

        h.pstart(
            "Starting normalization by flat/dark images, pixel data type: {0}...".format(data.dtype))

        norm_divide = np.subtract(norm_flat_img, norm_dark_img)

        # prevent divide-by-zero issues
        norm_divide[norm_divide == 0] = 1e-6
        clip_min = config.pre.clip_min
        clip_max = config.pre.clip_max

        # this divide gives bad results
        for idx in range(0, data.shape[0]):
            data[idx, :, :] = np.clip(np.true_divide(
                data[idx, :, :] - norm_dark_img, norm_divide), clip_min, clip_max)

        h.pstop(
            "Finished normalization by flat/dark images, pixel data type: {0}.".format(data.dtype))
    else:
        h.tomo_print_note(
            "Cannot apply normalization by flat/dark images because no valid flat image has been "
            "provided with -iflat/--input-path-flat")

    h.check_data_stack(data)

    return data
