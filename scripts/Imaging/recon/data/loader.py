from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper

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


def load(input_path=None, input_path_flat=None, input_path_dark=None,
         img_format='fits', data_dtype=np.float32, cores=1, chunksize=None, parallel_load=False, h=None):
    """
    Loads a stack, including sample, white and dark images.


    :param input_path: Path for the input data folder
    :param input_path_flat: Optional: Path for the input Flat images folder
    :param input_path_dark: Optional: Path for the input Dark images folder
    :param img_format: Default:'fits', format for the input images
    :param data_dtype: Default:np.float32, data type for the input images
    :param cores: Default:1, cores to be used if parallel_load is True
    :param chunksize: chunk of work per worker
    :param parallel_load: Default: False, if set to true the loading of the data will be done in parallel.
            This could be faster depending on the IO system. For local HDD runs the recommended setting is False
    :param h: Optional helper class. If not provided an empty one will be initialised.
    :returns :: stack of images as a 3-elements tuple: numpy array with sample images, white image, and dark image.
    """

    h = Helper.empty_init() if h is None else h

    if img_format in ['fits', 'fit']:
        sample, flat, dark = _do_img_load(fitsread, input_path, input_path_flat, input_path_dark, img_format,
                                          data_dtype, cores, chunksize, parallel_load, h)
    elif img_format in ['nxs']:
        sample, flat, dark = _do_nxs_load(input_path, img_format, data_dtype, cores, chunksize, parallel_load, h)
    else:
        sample, flat, dark = _do_img_load(imread, input_path, input_path_flat, input_path_dark, img_format, data_dtype,
                                          cores, chunksize, parallel_load, h)

    Helper.check_data_stack(sample)

    return sample, flat, dark


def _do_img_load(load_func, input_path, input_path_flat, input_path_dark, img_format,
                 data_dtype, cores, chunksize, parallel_load, h):
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
    :param input_path :: path to sample images. Can be a file or directory
    :param input_path_flat :: (optional) path to open beam / flat image(s). Can be a file or directory
    :param input_path_dark :: (optional) path to dark field image(s). Can be a file or directory
    :param img_format :: file extension (typically 'tiff', 'tif', 'fits', or 'fit' (not including the dot)
    :param data_dtype: the type in which the data will be loaded, could be float16, float32, float64, uint16
    :param cores: Cores to be used for parallel loading
    :param chunksize: Chunk of work that each worker will receive
    :param h: instance of the helper class

    :return :: 3 numpy arrays: input data volume (3D), average of flatt images (2D),
               average of dark images(2D)
    """

    sample_file_names = get_file_names(input_path, img_format)

    # Assumed that all images have the same size and properties as the first.
    first_sample_img = load_func(sample_file_names[0])

    # get the shape of all images
    img_shape = first_sample_img.shape

    sample_data = _load_sample_data(load_func, sample_file_names, img_shape, data_dtype, cores, chunksize,
                                    parallel_load, h)

    # this removes the image number dimension, if we loaded a stack of images
    img_shape = img_shape[1:] if len(img_shape) > 2 else img_shape

    flat_avg = _load_and_avg_data(load_func, input_path_flat, img_shape, img_format, data_dtype, "Flat", cores,
                                  chunksize, parallel_load, h)

    dark_avg = _load_and_avg_data(load_func, input_path_dark, img_shape, img_format, data_dtype, "Dark", cores,
                                  chunksize, parallel_load, h)

    return sample_data, flat_avg, dark_avg


def _do_nxs_load(input_path, img_format, data_dtype, cores, chunksize, parallel_load, h):
    """
    Do loading from NEXUS .nxs file, this requires special handling of the data, because flat and dark images are
    appended to the array.

    :param input_path: Path for the input data folder
    :param img_format: format for the input images
    :param data_dtype: Default:np.float32, data type for the input images
    :param cores: Default:1, cores to be used if parallel_load is True
    :param chunksize: chunk of work per worker
    :param parallel_load: Default: False, if set to true the loading of the data will be done in parallel.
            This could be faster depending on the IO system. For local HDD runs the recommended setting is False
    :param h: Optional helper class. If not provided an empty one will be initialised.
    :returns :: stack of images as a 3-elements tuple: numpy array with sample images, white image, and dark image.
    """
    data_file = get_file_names(input_path, img_format)

    new_data = nxsread(data_file[0])
    img_shape = new_data.shape

    from parallel import utility as pu
    data = pu.create_shared_array(img_shape, dtype=data_dtype)
    if parallel_load:
        data = _do_stack_load_par(data, new_data, nxsread, data_file[0], cores, chunksize, "NXS Load", h)
    else:
        data = _do_stack_load_seq(data, new_data, nxsread, data_file[0], img_shape, "NXS Load", h)

    return data[:-2, :, :], data[-2, :, :], data[-1, :, :]


def _load_and_avg_data(load_func, file_path, img_shape, img_format, data_dtype, prog_prefix=None, cores=1,
                       chunksize=None,
                       parallel_load=False, h=None):
    if file_path is not None:
        file_names = get_file_names(file_path, img_format)

        data = _load_files(load_func, file_names, img_shape, data_dtype, prog_prefix, cores, chunksize,
                           parallel_load, h)

        return get_data_average(data)


def _load_sample_data(load_func, sample_file_names, img_shape, data_dtype, cores=1, chunksize=None, parallel_load=False,
                      h=None):
    # determine what the loaded data was
    if len(img_shape) == 2:  # the loaded file was a single image
        sample_data = _load_files(load_func, sample_file_names, img_shape, data_dtype, "Sample", cores, chunksize,
                                  parallel_load, h)
    elif len(img_shape) == 3:  # the loaded file was a stack of fits images
        sample_data = _load_stack(load_func, sample_file_names[0], img_shape, data_dtype, "Sample", cores, chunksize,
                                  parallel_load, h)
    else:
        raise ValueError("Data loaded has invalid shape: {0}", img_shape)

    return sample_data


def _do_files_load_seq(data, load_func, files, img_shape, name, h):
    h.prog_init(len(files), desc=name)
    for idx, in_file in enumerate(files):
        try:
            data[idx, :, :] = load_func(in_file)[:]
            h.prog_update(1)
        except ValueError as exc:
            raise ValueError(
                "An image has different width and/or height dimensions! All images must have the same dimensions. "
                "Expected dimensions: {0} Error message: {1}".format(img_shape, exc))
        except IOError as exc:
            raise RuntimeError(
                "Could not load file {0}. Error details: {1}".format(in_file, exc))
    h.prog_close()

    return data


def _par_inplace_load_fwd_func(data, file, load_func=None):
    data[:] = load_func(file)


def _do_files_load_par(data, load_func, files, cores, chunksize, name, h):
    from parallel import two_shared_mem as ptsm
    f = ptsm.create_partial(_par_inplace_load_fwd_func, ptsm.inplace_fwd_func, load_func=load_func)
    ptsm.execute(data, files, f, cores, chunksize, name, h)
    return data


def _load_files(load_func, files, img_shape, dtype, name=None, cores=1, chunksize=None, parallel_load=False, h=None):
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

    h = Helper.empty_init() if h is None else h

    # Zeroing here to make sure that we can allocate the memory.
    # If it's not possible better crash here than later.
    from parallel import utility as pu
    data = pu.create_shared_array(
        (len(files), img_shape[0], img_shape[1]), dtype=dtype)

    if parallel_load:
        return _do_files_load_par(data, load_func, files, cores, chunksize, name, h)
    else:
        return _do_files_load_seq(data, load_func, files, img_shape, name, h)


def _move_data(input_data, output_data):
    """
    Forwarded function for parallel loading of data
    :param input_data: shared_data
    :param output_data: second_shared_data
    :return:
    """
    output_data[:] = input_data[:]


def _load_stack(load_func, file_name, img_shape, dtype, name, cores=1, chunksize=None, parallel_load=False, h=None):
    """
    Load a single image file that is expected to be a stack of images.

    Parallel execution is about 50%-100% slower, with the following code:

    # the function that will move the data
    def _move_data(input_data, output_data):
        output_data[:] = input_data[:]

    f = ptsm.create_partial(_move_data, fwd_function=ptsm.inplace_fwd_func)
    # this will open the file but not read all of it in!
    new_data = imread(file_name[0], img_format)
    # move the data in parallel, this causes 8 processes to try and read the IO at once, thus the slowdown
    ptsm.execute(new_data, data, f, cores=8, chunksize=None, name=name, h=h)

    :param file_name :: list of image file paths given as strings
    :param img_shape :: shape of every image, assumes they all have the same shape
    :param load_func :: file name extension if fixed (to set the expected image format)
    :param dtype :: data type for the output numpy array
    :return:
    """
    # create shared array
    from parallel import utility as pu
    data = pu.create_shared_array(img_shape, dtype=dtype)
    if parallel_load:
        return _do_stack_load_par(data, load_func, file_name, cores, chunksize, name, h)
    else:
        return _do_stack_load_seq(data, load_func, file_name, img_shape, name, h)


def _do_stack_load_seq(data, new_data, load_func, file_name, img_shape, name, h):
    """
    Sequential version of loading the data.
    This performs faster locally, but parallel performs faster on SCARF

    :param data: shared array of data
    :param load_func:
    :param file_name: the name of the stack file
    :param img_shape:
    :param name:
    :param h:
    :return:
    """
    # this will open the file but not read all of it in
    h.prog_init(img_shape[0], name)
    for i in range(img_shape[0]):
        data[i] = new_data[i]
        h.prog_update()
    h.prog_close()
    return data


def _do_stack_load_par(data, new_data, load_func, file_name, cores, chunksize, name, h):
    # this runs faster on SCARF
    from parallel import two_shared_mem as ptsm
    f = ptsm.create_partial(_move_data, fwd_function=ptsm.inplace_fwd_func)
    # move the data in parallel, this causes 8 processes to try and read the
    # IO at once, thus the slowdown
    ptsm.execute(new_data, data, f, cores, chunksize, name, h=h)
    return data


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
            "Could not load at least one FITS image/table file from: {0}".format(filename))

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
    return np.mean(data, axis=0)


def get_file_names(path, img_format):
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
