from __future__ import (absolute_import, division, print_function)
from helper import Helper

import numpy as np


def supported_formats():
    try:
        import pyfits
        fits_available = True
    except ImportError:
        # In Anaconda python, the pyfits package is in a different place, and this is what you frequently
        # find on windows.
        try:
            import astropy.io.fits as pyfits
            fits_available = True

        except ImportError:
            fits_available = False

    try:
        import h5py
        h5nxs_available = True
    except ImportError:
        h5nxs_available = False

    try:
        from skimage import io as skio
        skio_available = True
    except ImportError:
        skio_available = False

    avail_list = \
        (['fits', 'fit'] if fits_available else []) + \
        (['nxs'] if h5nxs_available else []) + \
        (['tif', 'tiff', 'png', 'jpg'] if skio_available else [])

    return avail_list


def load_data(config, h=None):
    """
    Load data by reading the provided configuration file for paths.
    This is intended to be used internally within the scripts.

    :param config: The full reconstruction config
    :param h: Helper class, if not provided will be initialised with the config

    :return: the loaded data as a tuple (sample, flat, dark)
    """
    h = Helper(config) if h is None else h

    h.pstart("Loading data...")
    input_path = config.func.input_path
    input_path_flat = config.func.input_path_flat
    input_path_dark = config.func.input_path_dark
    img_format = config.func.in_format
    data_dtype = config.func.data_dtype
    cores = config.func.cores
    chunksize = config.func.chunksize
    parallel_load = config.func.parallel_load

    sample, flat, dark = load(input_path, input_path_flat, input_path_dark,
                              img_format, data_dtype, cores, chunksize,
                              parallel_load, h)

    h.pstop("Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
        sample.shape, sample.dtype))

    h.check_data_stack(sample)

    return sample, flat, dark


def load(input_path=None,
         input_path_flat=None,
         input_path_dark=None,
         img_format=None,
         dtype=np.float32,
         cores=None,
         chunksize=None,
         parallel_load=False,
         h=None,
         file_names=None):
    """
    Loads a stack, including sample, white and dark images.


    :param input_path: Path for the input data folder
    :param input_path_flat: Optional: Path for the input Flat images folder
    :param input_path_dark: Optional: Path for the input Dark images folder
    :param img_format: Default:'fits', format for the input images
    :param dtype: Default:np.float32, data type for the input images
    :param cores: Default:1, cores to be used if parallel_load is True
    :param chunksize: chunk of work per worker
    :param parallel_load: Default: False, if set to true the loading of the data will be done in parallel.
            This could be faster depending on the IO system. For local HDD runs the recommended setting is False
    :param h: Helper class, if not provided will be initialised with empty constructor
    :return: stack of images as a 3-elements tuple: numpy array with sample images, white image, and dark image.
    """

    h = Helper.empty_init() if h is None else h
    if img_format is None:
        # assume only images in directory, inb4 loading text files
        img_format = '*'

    if file_names is None:
        input_file_names = get_file_names(input_path, img_format)
    else:
        input_file_names = file_names

    if img_format in ['fits', 'fit']:
        from imgdata import img_loader
        sample, flat, dark = img_loader.execute(
            fitsread, input_file_names, input_path_flat, input_path_dark,
            img_format, dtype, cores, chunksize, parallel_load, h)
    elif img_format in ['nxs']:
        from imgdata import nxs_loader
        sample, flat, dark = nxs_loader.execute(input_file_names[0],
                                                img_format, dtype, cores,
                                                chunksize, parallel_load, h)
    else:
        from imgdata import img_loader
        sample, flat, dark = img_loader.execute(
            imread, input_file_names, input_path_flat, input_path_dark,
            img_format, dtype, cores, chunksize, parallel_load, h)

    Helper.check_data_stack(sample)

    return sample, flat, dark


def fitsread(filename):
    """
    Read one image and return it as a 2d numpy array

    :param filename :: name of the image file, can be relative or absolute path
    :param img_format: format of the image ('fits')
    """
    pyfits = import_pyfits()
    image = pyfits.open(filename)
    if len(image) < 1:
        raise RuntimeError(
            "Could not load at least one FITS image/table file from: {0}".
            format(filename))

    # get the image data
    return image[0].data


def nxsread(filename):
    import h5py
    nexus = h5py.File(filename, 'r')
    data = nexus["entry1/tomo_entry/instrument/detector/data"]
    return data


def imread(filename):
    skio = import_skimage_io()
    return skio.imread(filename)


def import_pyfits():
    try:
        import pyfits
    except ImportError:
        # In Anaconda python, the pyfits package is in a different place, and this is what you frequently
        # find on windows.
        try:
            import astropy.io.fits as pyfits
        except ImportError:
            raise ImportError(
                "Cannot find the package 'pyfits' which is required to read/write FITS image files"
            )

    return pyfits


def import_skimage_io():
    """
    To import skimage io only when it is/can be used
    """
    try:
        from skimage import io as skio
        # tifffile works better on local, but not available on scarf
        # no plugin will use the default python imaging library (PIL)
        # This behaviour might need to be changed when switching to python 3
        # skio.use_plugin('freeimage')
    except ImportError as exc:
        raise ImportError(
            "Could not find the package skimage, its subpackage "
            "io and the pluging freeimage which are required to support "
            "several image formats. Error details: {0}".format(exc))
    return skio


def get_file_names(path, img_format, prefix=''):
    """
    Get all file names in a directory with a specific format.
    :param path: The path to be checked.
    :param img_format: The image format used as a postfix after the .
    :param prefix: A specific prefix for the images
    :return: All the file names, sorted by ascending
    """
    import os
    import glob

    path = os.path.abspath(os.path.expanduser(path))

    files_match = glob.glob(
        os.path.join(path, "{0}*.{1}".format(prefix, img_format)))

    if len(files_match) <= 0:
        raise RuntimeError(
            "Could not find any image files in {0} with extension: {1}".format(
                path, img_format))

    # this is a necessary step, otherwise the file order is not guaranteed to be sequential and we could get randomly
    # ordered stack of images which would produce nonsense
    files_match.sort(key=_alphanum_key_split)

    return files_match


def get_folder_names(path):
    """
    Get all folder names in a specific path.
    :param path: The path to be checked.
    :return: All the folder names, sorted by ascending
    """
    import os

    path = os.path.abspath(os.path.expanduser(path))

    folders = next(os.walk(path))[1]

    if len(folders) <= 0:
        raise RuntimeError("Could not find any folders in {0}".format(path))

    # this is a necessary step, otherwise the file order is not guaranteed to be sequential and we could get randomly
    # ordered stack of images which would produce nonsense
    folders.sort(key=_alphanum_key_split)

    return folders


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
    return [
        int(c) if c.isdigit() else c
        for c in alpha_num_split_re.split(path_str)
    ]


def parallel_move_data(input_data, output_data):
    """
    Forwarded function for parallel loading of data
    :param input_data: shared_data
    :param output_data: second_shared_data
    """
    output_data[:] = input_data[:]


def do_stack_load_seq(data, new_data, img_shape, name, h):
    """
    Sequential version of loading the data.
    This performs faster locally, but parallel performs faster on SCARF

    :param data: shared array of data
    :param new_data:
    :param img_shape:
    :param name:
    :param h: Helper class, if not provided will be initialised with empty constructor
    :return: the loaded data
    """
    h.prog_init(img_shape[0], name)
    for i in range(img_shape[0]):
        data[i] = new_data[i]
        h.prog_update()
    h.prog_close()
    return data


def do_stack_load_par(data, new_data, cores, chunksize, name, h):
    from parallel import two_shared_mem as ptsm
    f = ptsm.create_partial(
        parallel_move_data, fwd_function=ptsm.inplace_fwd_func)
    ptsm.execute(new_data, data, f, cores, chunksize, name, h=h)
    return data


def load_stack(load_func,
               file_name,
               dtype,
               name,
               cores=None,
               chunksize=None,
               parallel_load=False,
               h=None):
    """
    Load a single image FILE that is expected to be a stack of images.

    Parallel execution can be slower depending on the storage system.

    ! On HDD I've found it's about 50% SLOWER, thus not recommended!

    :param file_name :: list of image file paths given as strings
    :param load_func :: file name extension if fixed (to set the expected image format)
    :param dtype :: data type for the output numpy array
    :param cores: Default:1, cores to be used if parallel_load is True
    :param chunksize: chunk of work per worker
    :param parallel_load: Default: False, if set to true the loading of the data will be done in parallel.
            This could be faster depending on the IO system. For local HDD runs the recommended setting is False
    :param h: Helper class, if not provided will be initialised with empty constructor
    :return: stack of images as a 3-elements tuple: numpy array with sample images, white image, and dark image.
    """
    # create shared array
    from parallel import utility as pu
    new_data = load_func(file_name)
    img_shape = new_data.shape
    data = pu.create_shared_array(img_shape, dtype=dtype)
    if parallel_load:
        return do_stack_load_par(data, new_data, cores, chunksize, name, h)
    else:
        return do_stack_load_seq(data, new_data, img_shape, name, h)
