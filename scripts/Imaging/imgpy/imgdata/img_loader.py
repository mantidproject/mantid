from __future__ import (absolute_import, division, print_function)

import numpy as np
import helper as h
"""
This module handles the loading of FIT, FITS, TIF, TIFF
"""


def execute(load_func, input_file_names, input_path_flat, input_path_dark,
            img_format, data_dtype, cores, chunksize, parallel_load):
    """
    Reads a stack of images into memory, assuming dark and flat images
    are in separate directories.

    If several files are found in the same directory (for example you
    give image0001.fits and there's also image0002.fits,
    image0003.fits) these will also be loaded as the usual convention
    in ImageJ and related imaging tools, using the last digits to sort
    the images in the stack.

    Usual type in fits is 16-bit pixel depth, data type is denoted with:
        '>i2' - uint16
        '>f2' - float16
        '>f4' - float32

    :param load_func :: function to be used to load the files
    :param input_file_names :: path to sample images. Can be a file or directory
    :param input_path_flat :: (optional) path to open beam / flat image(s). Can be a file or directory
    :param input_path_dark :: (optional) path to dark field image(s). Can be a file or directory
    :param img_format :: file extension (typically 'tiff', 'tif', 'fits', or 'fit' (not including the dot)
    :param data_dtype: the type in which the data will be loaded, could be float16, float32, float64, uint16
    :param cores: Cores to be used for parallel loading
    :param chunksize: Chunk of work that each worker will receive

    :return :: 3 numpy arrays: input data volume (3D), average of flatt images (2D),
               average of dark images(2D)
    """

    # Assumed that all images have the same size and properties as the first.
    first_sample_img = load_func(input_file_names[0])

    # get the shape of all images
    img_shape = first_sample_img.shape

    sample_data = _load_sample_data(load_func, input_file_names, img_shape,
                                    data_dtype, cores, chunksize,
                                    parallel_load)

    # this removes the image number dimension, if we loaded a stack of images
    img_shape = img_shape[1:] if len(img_shape) > 2 else img_shape

    flat_avg = _load_and_avg_data(load_func, input_path_flat, img_shape,
                                  img_format, data_dtype, "Flat", cores,
                                  chunksize, parallel_load)

    dark_avg = _load_and_avg_data(load_func, input_path_dark, img_shape,
                                  img_format, data_dtype, "Dark", cores,
                                  chunksize, parallel_load)

    return sample_data, flat_avg, dark_avg


def _load_sample_data(load_func,
                      input_file_names,
                      img_shape,
                      data_dtype,
                      cores=None,
                      chunksize=None,
                      parallel_load=False):
    from imgdata.loader import load_stack

    # determine what the loaded data was
    if len(img_shape) == 2:  # the loaded file was a single image
        sample_data = _load_files(load_func, input_file_names, img_shape,
                                  data_dtype, "Sample", cores, chunksize,
                                  parallel_load)
    elif len(img_shape) == 3:  # the loaded file was a stack of fits images
        sample_data = load_stack(load_func, input_file_names[0], data_dtype,
                                 "Sample", cores, chunksize, parallel_load)
    else:
        raise ValueError("Data loaded has invalid shape: {0}", img_shape)

    return sample_data


def _load_and_avg_data(load_func,
                       file_path,
                       img_shape,
                       img_format,
                       data_dtype,
                       prog_prefix=None,
                       cores=None,
                       chunksize=None,
                       parallel_load=False):
    if file_path is not None:
        from imgdata.loader import get_file_names
        file_names = get_file_names(file_path, img_format)

        data = _load_files(load_func, file_names, img_shape, data_dtype,
                           prog_prefix, cores, chunksize, parallel_load)
        return get_data_average(data)


def _do_files_load_seq(data, load_func, files, img_shape, name):
    h.prog_init(len(files), desc=name)
    for idx, in_file in enumerate(files):
        try:
            data[idx, :, :] = load_func(in_file)[:]
            h.prog_update()
        except ValueError as exc:
            raise ValueError(
                "An image has different width and/or height dimensions! All images must have the same dimensions. "
                "Expected dimensions: {0} Error message: {1}".format(img_shape,
                                                                     exc))
        except IOError as exc:
            raise RuntimeError("Could not load file {0}. Error details: {1}".
                               format(in_file, exc))
    h.prog_close()

    return data


def _par_inplace_load_fwd_func(data, filename, load_func=None):
    data[:] = load_func(filename)


def _do_files_load_par(data, load_func, files, cores, chunksize, name):
    from parallel import two_shared_mem as ptsm
    f = ptsm.create_partial(
        _par_inplace_load_fwd_func, ptsm.inplace_fwd_func, load_func=load_func)
    ptsm.execute(data, files, f, cores, chunksize, name)
    return data


def _load_files(load_func,
                files,
                img_shape,
                dtype,
                name=None,
                cores=None,
                chunksize=None,
                parallel_load=False):
    """
    Reads image files in a row into a 3d numpy array. Useful when reading all the sample
    images, or all the flat or dark images.

    Tried an multiparallel version of this with Python 2.7 multithreading library.
    Each type -> Pool, processes and threads, and none gave any improvement
    over linear loading, it was usually up to 50% slower with MP loading.

    The reason is that the loading is IO Bound, not CPU bound, thus
    multiple threads or processes accessing the IO doesn't provide any benefit.

    :param files :: list of image file paths given as strings
    :param img_shape :: shape of every image, assumes they all have the same shape
    :param load_func :: file name extension if fixed (to set the expected image format)
    :param dtype :: data type for the output numpy array

    Returns:: a 3d data volume with the size of the first (outermost) dimension equal
    to the number of files, and the sizes of the second and third dimensions equal to
    the sizes given in the input img_shape
    """

    # Zeroing here to make sure that we can allocate the memory.
    # If it's not possible better crash here than later.
    from parallel import utility as pu
    data = pu.create_shared_array(
        (len(files), img_shape[0], img_shape[1]), dtype=dtype)

    if parallel_load:
        return _do_files_load_par(data, load_func, files, cores, chunksize,
                                  name)
    else:
        return _do_files_load_seq(data, load_func, files, img_shape, name)


def get_data_average(data):
    return np.mean(data, axis=0)
