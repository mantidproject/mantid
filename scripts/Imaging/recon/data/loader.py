from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper

import numpy as np


# TODO write performance test, execution and memory usage!
def supported_formats():
    try:
        import pyfits
        fits_available = True
    except ImportError:
        fits_available = False

    try:
        import h5py
        h5nxs_available = True
    except ImportError:
        h5nxs_available = False

    avail_list = \
        (['fits', 'fit'] if fits_available else []) + \
        (['nxs'] if h5nxs_available else [])

    return avail_list


def load_stack_config(config):
    """
    Loads a stack, including sample, white and dark images.

    :config: The full reconstruction config

    :returns :: stack of images as a 3-elements tuple: numpy array with sample images, white image, and dark image.
    """
    input_path = config.func.input_path
    input_path_flat = config.func.input_path_flat
    input_path_dark = config.func.input_path_dark
    img_format = config.func.in_format
    data_dtype = config.func.data_dtype
    h = config.helper

    if img_format in ['fits', 'fit']:
        sample, flat, dark = load_stack(
            input_path, input_path_flat, input_path_dark, img_format, data_dtype, h)

    elif img_format in ['nxs']:
        data_file = _get_stack_file_names(input_path, img_format)

        # It is assumed that all images have the same size and properties as the
        # first.
        # read in .nxs file
        sample, flat, dark = _read_nxs(data_file[0])

    from recon.helper import Helper
    Helper.check_data_stack(sample)

    return sample, flat, dark


def load_stack(sample_path, flat_file_path=None, dark_file_path=None,
               img_format='fits', argument_data_dtype=np.float32, h=None):
    """
    Reads a stack of images into memory, assuming dark and flat images
    are in separate directories.

    If several files are found in the same directory (for example you
    give image0001.fits and there's also image0002.fits,
    image0003.fits) these will also be loaded as the usual convention
    in ImageJ and related imaging tools, using the last digits to sort
    the images in the stack.

    :param sample_path :: path to sample images. Can be a file or directory
    :param flat_file_path :: (optional) path to open beam / flat image(s). Can be a file or directory
    :param dark_file_path :: (optional) path to dark field image(s). Can be a file or directory
    :param img_format :: file extension (typically 'tiff', 'tif', 'fits', or 'fit' (not including the dot)
    :param argument_data_dtype: the type in which the data will be loaded, could be float16, float32, float64, uint16

    :return :: 3 numpy arrays: input data volume (3d), average of flatt images (2d),
               average of dark images(2d)
    """

    sample_file_names = _get_stack_file_names(
        sample_path, img_format)

    # It is assumed that all images have the same size and properties as the
    # first.
    try:
        first_sample_img = _read_img(sample_file_names[0], img_format)
    except RuntimeError as exc:
        raise RuntimeError(
            "Could not load at least one image file from: {0}. Details: {1}".
            format(sample_path, str(exc)))

    # data_dtype = first_sample_img.dtype
    # usual type in fits with 16-bit pixel depth
    # '>i2' uint16
    # '>f4' float32
    # (type letter i,f,etc, number is *8, eg f4 = float8*4 = float32
    # force float32 on everything
    data_dtype = argument_data_dtype
    img_shape = first_sample_img.shape

    sample_data = _load_sample_data(
        first_sample_img, sample_file_names, img_shape, img_format, data_dtype, h)

    # this removes the image number dimension, if we loaded a stack of images
    img_shape = img_shape[1:] if len(img_shape) > 2 else img_shape
    flat_avg = _load_and_avg_data(
        flat_file_path, img_shape, img_format, data_dtype, h, "Flat")
    dark_avg = _load_and_avg_data(
        dark_file_path, img_shape, img_format, data_dtype, h, "Dark")

    return sample_data, flat_avg, dark_avg


def _load_and_avg_data(file_path, img_shape, img_format, data_dtype, h=None, prog_prefix=None):
    if file_path is not None:
        file_names = _get_stack_file_names(
            file_path, img_format)

        data = _read_listed_files(
            file_names, img_shape, img_format, data_dtype, h, prog_prefix)
        avg = get_data_average(data)
    else:
        avg = None
    return avg


def _load_sample_data(first_sample_img, sample_file_names, img_shape, img_format, data_dtype, h=None):
    # determine what the loaded data was
    if len(img_shape) == 2:  # the loaded file was a single image
        sample_data = _read_listed_files(
            sample_file_names, img_shape, img_format, data_dtype, h, "Sample")
    elif len(img_shape) == 3:  # the loaded file was a stack of fits images
        sample_data = first_sample_img
    else:
        raise ValueError("Data loaded has invalid shape: {0}", img_shape)

    return sample_data


def _read_listed_files(files, img_shape, img_format, dtype, h=None, loop_name=None):
    """
    Read several images in a row into a 3d numpy array. Useful when reading all the sample
    images, or all the flat or dark images.

    Tried an multiparallel version of this with Python 2.7 multithreading library.
    Each type -> Pool, processes and threads, and none gave any improvement
    over linear loading, it was usually up to 50% slower with MP loading.

    The reason is that the loading is IO Bound, not CPU bound, thus
    multiple threads or processes accessing the IO doesn't provide any benefit.

    :param files :: list of image file paths given as strings
    :param img_shape :: shape of every image, assumes they all have the same shape
    :param img_format :: file name extension if fixed (to set the expected image format)
    :param dtype :: data type for the output numpy array

    Returns:: a 3d data volume with the size of the first (outermost) dimension equal
    to the number of files, and the sizes of the second and third dimensions equal to
    the sizes given in the input img_shape
    """

    h = Helper.empty_init() if h is None else h

    # Zeroing here to make sure that we can allocate the memory.
    # If it's not possible better crash here than later
    from parallel import shared_mem as psm
    data = psm.create_shared_array(
        (len(files), img_shape[0], img_shape[1]), dtype=dtype)
    # data = np.zeros((len(files), img_shape[
    #     0], img_shape[1]), dtype=dtype)

    # FIXME TODO PERFORMANCE CRITICAL
    # TODO Performance Tests
    h.prog_init(len(files), desc=loop_name)
    for idx, in_file in enumerate(files):
        try:
            data[idx, :, :] = _read_img(in_file, img_format)
            h.prog_update(1)
        except ValueError as exc:
            raise ValueError(
                "An image has different width and/or height dimensions! All images must have the same dimensions. "
                "Error message: " + str(exc))
        except IOError as exc:
            raise RuntimeError(
                "Could not load file {0}. Error details: {1}".format(in_file, str(exc)))
    h.prog_close()
    return data


def _read_img(filename, img_format=None):
    """
    Read one image and return it as a 2d numpy array

    :param filename :: name of the image file, can be relative or absolute path
    :param img_format :: extension and effectively format to use ('tiff', 'fits')
    """
    # currently the only image type is FITS, but there will be tiff
    return _read_fits(filename)


def _read_fits(filename):
    pyfits = import_pyfits()
    image = pyfits.open(filename)
    if len(image) < 1:
        raise RuntimeError(
            "Could not load at least one FITS image/table file from: {0}".format(filename))

    # get the image data
    return image[0].data


def _read_nxs(filename):
    import h5py
    nexus = h5py.File(filename, 'r')
    data = nexus["entry1/tomo_entry/instrument/detector/data"]
    dark = data[-1, :, :]

    flat = data[-2, :, :]

    return data[:-2, :, :], flat, dark


def import_pyfits():
    """
    To import pyfits optionally only when it is/can be used
    """
    try:
        import pyfits
    except ImportError:
        # In Anaconda python, the pyfits package is in a different place, and this is what you frequently
        # find on windows.
        try:
            import astropy.io.fits as pyfits
        except ImportError:
            raise ImportError(
                "Cannot find the package 'pyfits' which is required to read/write FITS image files")

    return pyfits


def import_skimage_io():
    """
    To import skimage io only when it is/can be used
    """
    try:
        from skimage import io as skio
        skio.use_plugin('freeimage')
    except ImportError as exc:
        raise ImportError("Could not find the package skimage, its subpackage "
                          "io and the pluging freeimage which are required to support "
                          "several image formats. Error details: {0}".format(exc))
    return skio


def get_data_average(data):
    avg = np.mean(data, axis=0)
    return avg


def _get_stack_file_names(path, img_format):
    import os
    import glob

    path = os.path.expanduser(path)

    files_match = glob.glob(os.path.join(
        path, "{0}*.{1}".format('', img_format)))

    if len(files_match) <= 0:
        raise RuntimeError("Could not find any image files in {0} with extension: {1}".
                           format(path, img_format))

    # this is a necessary step, otherwise the file order is not guaranteed to be sequential and we could get randomly
    # ordered stack of images which would produce nonsense
    files_match.sort(key=_alphanum_key_split)

    return files_match


def _alphanum_key_split(path_str):
    """
    From a string to a list of alphabetic and numeric elements. Intended to
    be used for sequence number/natural sorting. In list.sort() the
    key can be a list, so here we split the alpha/numeric fields into
    a list. For example (in the final order after sort() would be applied):

    "angle4" -> ["angle", 4]
    "angle31" -> ["angle", 31]
    "angle42" -> ["angle", 42]
    "angle101" -> ["angle", 101]

    Several variants compared here:
    https://dave.st.germa.in/blog/2007/12/11/exception-handling-slow/
    """
    import re
    alpha_num_split_re = re.compile('([0-9]+)')
    return [int(c) if c.isdigit() else c for c in alpha_num_split_re.split(path_str)]
