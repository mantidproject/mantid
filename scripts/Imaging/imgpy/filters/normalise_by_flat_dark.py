from __future__ import (absolute_import, division, print_function)
import numpy as np
import helper as h


def cli_register(parser):
    # this doesn't have anything to add, 
    # the options are added in the funcitonal config, 
    # which should be moved to here TODO
    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def _apply_normalise_inplace(data, norm_divide, clip_min=None, clip_max=None):
    data[:] = np.clip(np.true_divide(data, norm_divide), clip_min, clip_max)


def _apply_normalise(data,
                     dark=None,
                     norm_divide=None,
                     clip_min=None,
                     clip_max=None):
    return np.clip(
        np.true_divide(data - dark, norm_divide), clip_min, clip_max)


def execute(data,
            norm_flat_img=None,
            norm_dark_img=None,
            clip_min=0,
            clip_max=3,
            roi=None,
            cores=None,
            chunksize=None):
    h.check_data_stack(data)

    if norm_flat_img is not None and norm_dark_img is not None and isinstance(
            norm_flat_img, np.ndarray):
        if 2 != len(norm_flat_img.shape) or 2 != len(norm_dark_img.shape):
            raise ValueError(
                "Incorrect shape of the flat image ({0}) which should match the "
                "shape of the sample images ({1})".format(norm_flat_img.shape,
                                                          data[0].shape))

        from parallel import utility as pu
        if pu.multiprocessing_available():
            _execute_par(data, norm_flat_img, norm_dark_img, clip_min,
                         clip_max, roi, cores, chunksize)
        else:
            _execute_seq(data, norm_flat_img, norm_dark_img, clip_min,
                         clip_max, roi)

    else:
        # I think this might be the only "filter not applied" message that 
        #  is useful, so I've left it here for now
        h.tomo_print_note(
            "Not applying normalization by flat/dark images because no valid flat and dark images have been "
            "provided with -F/--input-path-flat and -D/--input-path-dark.")

    h.check_data_stack(data)
    return data


def _execute_par(data,
                 norm_flat_img=None,
                 norm_dark_img=None,
                 clip_min=0,
                 clip_max=5,
                 roi=None,
                 cores=None,
                 chunksize=None):
    """
    Normalise by flat and dark images

    :param data :: image stack as a 3d numpy array
    :param norm_flat_img :: flat (open beam) image to use in normalization
    :param norm_dark_img :: dark image to use in normalization
    :param clip_min: Pixel values found below this value will be clipped to equal this value
    :param clip_max: Pixel values found above this value will be clipped to equal this value
    :param cores:
    :param chunksize:

    :returns :: filtered data (stack of images)
    """
    import numpy as np
    h.pstart(
        "Starting PARALLEL normalization by flat/dark images, pixel data type: {0}...".
        format(data.dtype))

    from parallel import utility as pu
    norm_divide = pu.create_shared_array((1, data.shape[1], data.shape[2]))
    norm_divide[:] = norm_divide.reshape(data.shape[1], data.shape[2])

    # subtract dark from flat and copy into shared array
    norm_divide[:] = np.subtract(norm_flat_img, norm_dark_img)

    # prevent divide-by-zero issues
    norm_divide[norm_divide == 0] = 1e-6

    # subtract the dark from all images
    np.subtract(data[:], norm_dark_img, out=data[:])

    from parallel import two_shared_mem as ptsm

    f = ptsm.create_partial(
        _apply_normalise_inplace,
        fwd_function=ptsm.inplace_fwd_func_second_2d,
        clip_min=clip_min,
        clip_max=clip_max)

    data, norm_divide = ptsm.execute(data, norm_divide, f, cores, chunksize,
                                     "Norm by Flat/Dark")

    h.pstop(
        "Finished PARALLEL normalization by flat/dark images, pixel data type: {0}.".
        format(data.dtype))

    return data


def _execute_seq(data,
                 norm_flat_img=None,
                 norm_dark_img=None,
                 clip_min=0,
                 clip_max=1.5,
                 roi=None):
    """
    Normalise by flat and dark images

    :param data :: image stack as a 3d numpy array
    :param norm_flat_img :: flat (open beam) image to use in normalization
    :param norm_dark_img :: dark image to use in normalization
    :param clip_min: Pixel values found below this value will be clipped to equal this value
    :param clip_max: Pixel values found above this value will be clipped to equal this value

    :returns :: filtered data (stack of images)
    """

    if roi:
        raise NotImplementedError(
            "The sequential execution with ROI for scaling is not implemented")

    import numpy as np
    h.pstart(
        "Starting normalization by flat/dark images, pixel data type: {0}...".
        format(data.dtype))

    norm_divide = np.subtract(norm_flat_img, norm_dark_img)

    # prevent divide-by-zero issues
    norm_divide[norm_divide == 0] = 1e-6

    # this divide gives bad results
    h.prog_init(data.shape[0], "Norm by Flat/Dark")
    for idx in range(0, data.shape[0]):
        data[idx, :, :] = np.clip(
            np.true_divide(data[idx, :, :] - norm_dark_img, norm_divide),
            clip_min, clip_max)
        h.prog_update(1)

    h.prog_close()
    h.pstop(
        "Finished normalization by flat/dark images, pixel data type: {0}.".
        format(data.dtype))

    return data
