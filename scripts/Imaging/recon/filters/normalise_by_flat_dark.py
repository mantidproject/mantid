from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper
import numpy as np


def _apply_normalise_inplace(data, norm_divide, clip_min=None, clip_max=None):
    data[:] = np.clip(np.true_divide(
        data, norm_divide), clip_min, clip_max)


def _apply_normalise(data, dark=None, norm_divide=None, clip_min=None, clip_max=None):
    return np.clip(np.true_divide(
        data - dark, norm_divide), clip_min, clip_max)


def execute(data, norm_flat_img=None, norm_dark_img=None, clip_min=0, clip_max=1.5, cores=1, chunksize=None, h=None):
    h = Helper.empty_init() if h is None else h
    h.check_data_stack(data)

    if norm_flat_img is not None and norm_dark_img is not None and isinstance(norm_flat_img, np.ndarray):
        if 2 != len(norm_flat_img.shape):
            raise ValueError(
                "Incorrect shape of the flat image ({0}) which should match the "
                "shape of the sample images ({1})".format(
                    norm_flat_img.shape, data[0].shape))

        from parallel import utility as pu
        if pu.multiprocessing_available():
            _execute_par(data, norm_flat_img, norm_dark_img,
                         clip_min, clip_max, cores, chunksize, h)
        else:
            _execute_seq(data, norm_flat_img, norm_dark_img,
                         clip_min, clip_max, h)

    else:
        h.tomo_print_note(
            "Cannot apply normalization by flat/dark images because no valid flat image has been "
            "provided with -F/--input-path-flat and -D/--input-path-dark.")

    h.check_data_stack(data)
    return data


def _execute_par(data, norm_flat_img=None, norm_dark_img=None, clip_min=0, clip_max=1.5, cores=1, chunksize=None, h=None):
    """
    Normalise by flat and dark images

    :param data :: image stack as a 3d numpy array
    :param norm_flat_img :: flat (open beam) image to use in normalization
    :param norm_dark_img :: dark image to use in normalization
    :param clip_min: Pixel values found below this value will be clipped to equal this value
    :param clip_max: Pixel values found above this value will be clipped to equal this value
    :param cores:
    :param chunksize:
    :param h: Helper class, if not provided will be initialised with empty constructor


    :returns :: filtered data (stack of images)
    """
    import numpy as np

    h.pstart(
        "Starting PARALLEL normalization by flat/dark images, pixel data type: {0}...".format(data.dtype))

    from parallel import utility as pu
    norm_divide = pu.create_shared_array((1, data.shape[1], data.shape[2]))
    norm_divide[:] = norm_divide.reshape(data.shape[1], data.shape[2])
    norm_divide[:] = np.subtract(norm_flat_img, norm_dark_img)
    # prevent divide-by-zero issues
    norm_divide[norm_divide == 0] = 1e-6

    np.subtract(data[:], norm_dark_img, out=data[:])

    from parallel import two_shared_mem as ptsm
    f = ptsm.create_partial(_apply_normalise_inplace, fwd_function=ptsm.inplace_fwd_func_second_2d,
                            clip_min=clip_min, clip_max=clip_max)

    Helper.debug_print_memory_usage_linux("Before execution")

    data, norm_divide = ptsm.execute(data, norm_divide, f, cores, chunksize, "Norm by Flat/Dark", h=h)
    Helper.debug_print_memory_usage_linux("after execution")
    h.pstop(
        "Finished PARALLEL normalization by flat/dark images, pixel data type: {0}.".format(data.dtype))

    return data


def _execute_seq(data, norm_flat_img=None, norm_dark_img=None, clip_min=0, clip_max=1.5, h=None):
    """
    Normalise by flat and dark images

    :param data :: image stack as a 3d numpy array
    :param norm_flat_img :: flat (open beam) image to use in normalization
    :param norm_dark_img :: dark image to use in normalization
    :param clip_min: Pixel values found below this value will be clipped to equal this value
    :param clip_max: Pixel values found above this value will be clipped to equal this value
    :param h: Helper class, if not provided will be initialised with empty constructor


    :returns :: filtered data (stack of images)
    """
    import numpy as np

    h.pstart(
        "Starting normalization by flat/dark images, pixel data type: {0}...".format(data.dtype))

    norm_divide = np.subtract(norm_flat_img, norm_dark_img)

    # prevent divide-by-zero issues
    norm_divide[norm_divide == 0] = 1e-6

    # this divide gives bad results
    h.prog_init(data.shape[0], "Norm by Flat/Dark")
    for idx in range(0, data.shape[0]):
        data[idx, :, :] = np.clip(np.true_divide(
            data[idx, :, :] - norm_dark_img, norm_divide), clip_min, clip_max)
        h.prog_update(1)

    h.prog_close()
    h.pstop(
        "Finished normalization by flat/dark images, pixel data type: {0}.".format(data.dtype))

    return data
