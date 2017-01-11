from __future__ import (absolute_import, division, print_function)


def execute(data, config, norm_flat_img, norm_dark_img=0):
    """
    Normalize by flat and dark images

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

    if not config.normalize_flat_dark:
        h.tomo_print(
            " * Note: NOT applying normalization by flat/dark images.")
        return data

    if isinstance(norm_flat_img, np.ndarray):
        if 2 != len(norm_flat_img.shape) or norm_flat_img.shape != data.shape[1:]:
            raise ValueError(
                "Incorrect shape of the flat image ({0}) which should match the "
                "shape of the sample images ({1})".format(
                    norm_flat_img.shape, data[0].shape))

        h.pstart(
            " * Starting normalization by flat/dark images, pixel data type: {0}...".format(data.dtype))

        norm_divide = norm_flat_img - norm_dark_img

        # prevent divide-by-zero issues
        norm_divide[norm_divide == 0] = 1e-6

        for idx in range(0, data.shape[0]):
            data[idx, :, :] = np.true_divide(
                data[idx, :, :] - norm_dark_img, norm_divide)
        # true_divide produces float64, we assume that precision not needed (definitely not
        # for 16-bit depth output images as we usually have).
        h.pstop(
            " * Finished normalization by flat/dark images, pixel data type: {0}.".format(data.dtype))
    else:
        h.tomo_print(
            " * Note: cannot apply normalization by flat/dark images because no valid flat image has been "
            "provided in the inputs. Flat image given: {0}".format(norm_flat_img))

    h.check_data_stack(data)

    return data
