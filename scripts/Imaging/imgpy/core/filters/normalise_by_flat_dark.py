from __future__ import (absolute_import, division, print_function)
import numpy as np
import helper as h
from core.parallel import utility as pu
from core.parallel import two_shared_mem as ptsm


def cli_register(parser):
    # this doesn't have anything to add,
    # the options are added in the funcitonal config,
    # which should be moved to here TODO
    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def _apply_normalise_inplace(data, norm_divide):
    data[:] = np.true_divide(data, norm_divide)


def _apply_normalise(data, dark=None, norm_divide=None):
    return np.true_divide(data - dark, norm_divide)


def execute(data, flat=None, dark=None, cores=None, chunksize=None):
    """
    Normalise by flat and dark images

    :param data: image stack as a 3d numpy array
    :param flat: flat (open beam) image to use in normalization
    :param dark: dark image to use in normalization
    :param cores: The number of cores that will be used to process the data.
    :param chunksize: The number of chunks that each worker will receive.

    :returns: filtered data (stack of images)

    Example command line:
    python main.py -i /some/data -F /some/flat/images -D /some/dark/images
    python main.py -i /some/data --input-path-flat /some/flat/images --input-path-flat /some/dark/images
    """
    h.check_data_stack(data)

    if flat is not None and dark is not None and isinstance(
            flat, np.ndarray) and isinstance(dark, np.ndarray):
        if 2 != len(flat.shape) or 2 != len(dark.shape):
            raise ValueError(
                "Incorrect shape of the flat image ({0}) or dark image ({1}) \
                which should match the shape of the sample images ({2})"
                .format(flat.shape, dark.shape, data[0].shape))

        if pu.multiprocessing_available():
            _execute_par(data, flat, dark, cores, chunksize)
        else:
            _execute_seq(data, flat, dark)

    h.check_data_stack(data)
    return data


def _execute_par(data, flat=None, dark=None, cores=None, chunksize=None):
    h.pstart("Starting PARALLEL normalization by flat/dark images.")

    norm_divide = pu.create_shared_array((1, data.shape[1], data.shape[2]))
    norm_divide = norm_divide.reshape(data.shape[1], data.shape[2])

    # subtract dark from flat and copy into shared array with [:]
    norm_divide[:] = np.subtract(flat, dark)

    # prevent divide-by-zero issues, and negative pixels make no sense
    norm_divide[norm_divide == 0] = 1e-9

    # subtract the dark from all images,
    # specify out to do in place, otherwise the data is copied
    np.subtract(data[:], dark, out=data[:])

    f = ptsm.create_partial(
        _apply_normalise_inplace, fwd_function=ptsm.inplace_fwd_func_second_2d)

    data, norm_divide = ptsm.execute(data, norm_divide, f, cores, chunksize,
                                     "Norm by Flat/Dark")

    # This clipping is important, if removed some images will have pixels
    # with big negative values -25626262 which throws off contrast adjustments.
    # This will crop those negative pixels out, and set them to nearly zero
    # The negative values will also get scaled back after this in
    # value_scaling which will increase their values futher!
    np.clip(data, 1e-9, 3, data)
    h.pstop("Finished PARALLEL normalization by flat/dark images.")

    return data


def _execute_seq(data, flat=None, dark=None):
    h.pstart("Starting normalization by flat/dark images.")

    norm_divide = np.subtract(flat, dark)

    # prevent divide-by-zero issues
    norm_divide[norm_divide <= 0] = 1e-6

    # this divide gives bad results
    h.prog_init(data.shape[0], "Norm by Flat/Dark")
    for idx in range(0, data.shape[0]):
        data[idx, :, :] = np.true_divide(data[idx, :, :] - dark, norm_divide)
        h.prog_update()

    h.prog_close()
    h.pstop("Finished normalization by flat/dark images.")

    return data
