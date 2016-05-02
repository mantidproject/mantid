# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>
"""
Input/output functionality for stacks of images. For example: load
a stack of images in different formats (.tif, FITS). Includes handling
of paths, sample, flat, and dark images, file name prefixes, etc.

"""

import glob
import os
import re

import numpy as np

__AGG_IMG_IDX = 0

def _import_pyfits():
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
            raise ImportError("Cannot find the package 'pyfits' which is required to read/write FITS image files")

    return pyfits

def _import_skimage_io():
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

def make_dirs_if_needed(dirname):
    """
    Makes sure that the directory needed (for example to save a file)
    exists, otherwise creates it.

    @param dirname :: (output) directory to check

    """
    absname = os.path.abspath(dirname)
    if not os.path.exists(absname):
        os.makedirs(absname)

#pylint: disable=too-many-arguments
def write_image(img_data, min_pix, max_pix, filename, img_format=None, dtype=None,
                rescale_intensity=False):
    """
    Output image data, given as a numpy array, to a file, in a given image format.
    Assumes that the output directory exists (must be checked before). The pixel
    values are rescaled in the range [min_pix, max_pix] which would normally be set
    to the minimum/maximum values found in a stack of images.

    @param img_data :: image data in the usual numpy representation

    @param min_pix :: minimum reference value to rescale data (may be local to an
    image or global for a stack of images)
    @param max_pix :: maximum reference value to rescale data (may be local to an
    image or global for a stack of images)

    @param filename :: file name, including directory and extension
    @param img_format :: image file format
    @param dtype :: can be used to force a pixel type, otherwise the type
    of the input data is used

    Returns:: name of the file saved
    """
    if not img_format:
        img_format = 'png'
    filename = filename + '.' + img_format

    # The special case dtype = 'uint8' could be handled with bytescale:
    # img_data = scipy.misc.bytescale(img_data)

    # from bigger to smaller type, example: float32 => uint16
    if dtype and img_data.dtype != dtype:
        old_img_data = img_data
        img_data = np.zeros(old_img_data.shape, dtype='float32')
        pix_range = max_pix - float(min_pix)
        scale_factor = (np.iinfo(dtype).max - np.iinfo(dtype).min) / pix_range

        too_verbose = False
        if too_verbose:
            print "pix min: {0}, max: {1}, scale_factor: {2}".format(min_pix, max_pix, scale_factor)
        img_data = scale_factor * (old_img_data - min_pix)
        img_data = img_data.astype(dtype=dtype)

    # this rescale intensity would ignore the range of other images in the stack
    # in addition, it clips if the original values are below/above the destination type limits
    if rescale_intensity:
        try:
            from skimage import exposure
        except ImportError as exc:
            raise ImportError("Could not find the exposure package (in skimage) "
                              "Error details: {0}".format(exc))
        img_data = exposure.rescale_intensity(img_data, out_range=dtype)#'uint16')

    skio = _import_skimage_io()
    # Without this plugin tiff files don't seem to be generated correctly for some
    # bit depths (especially relevant for uint16), but you still need to load the
    # freeimage plugin with use_plugin!
    _USING_PLUGIN_TIFFFILE = True
    if img_format == 'tiff' and _USING_PLUGIN_TIFFFILE:
        # compression option intentionally not used: compress=6, 3rd party tools have
        # issues loading compressed tiff files)
        skio.imsave(filename, img_data, plugin='tifffile')
    else:
        skio.imsave(filename, img_data, plugin='freeimage')

    return filename

def avg_image_files(path, base_path, file_extension=None, agg_method='average'):
    """
    Reads files from a directory, assuming they are images from a
    stack, and calculates the average image.

    @param base_path :: path that can be used as base path if the path to the images
    to average is relative (for example a dark images path relative to the samples
    path)
    """
    path = None
    if os.path.isabs(base_path):
        path = base_path
    else:
        os.path.join(base_path, path)

    path = os.path.expanduser(path)

    img_files = glob.glob(os.path.join(sample_path,
                                       "{0}*.{1}".format(file_prefix, file_extension)))

    if len(img_files) <= 0:
        raise RuntimeError("No image files found in " + path)

    pyfits = _import_pyfits()

    imgs = pyfits.open(img_files[0])
    if len(imgs) < 1:
        raise RuntimeError(
            "Could not load at least one image from path: {0}".format(path_proj))

    data_dtype = imgs[0].data.dtype
    # from fits files we usually get this, just change it to uint16
    if '>i2' == data_dtype:
        data_dtype = np.uint16

    accum = np.zeros((imgs[0].shape[0], imgs[0].shape[1]), dtype=data_dtype)

    if 'average' == agg_method:
        __AGG_IMG_IDX = 1

    for ifile in img_files:
        hdu = None
        try:
            hdu = pyfits.open(ifile)
        except IOError as exc:
            print "Got I/O exception trying to open and load {0}: {1}. Ignoring and going on.".format(
                ifile, str(exc))
            continue

        accum = _agg_img(accum, hdu[0].data, agg_method=agg_method)

    return accum

def _agg_img(acc, img_data, agg_method=None, index=1):
    """
    Adds in (with average, median, etc.) data coming from a new
    image/hdu. Expects a numpy array of shape (N1, N2) where N1 and N2
    are the number of rows and columns of the images.

    @param acc :: aggregated value accumulated so far
    @param img_data :: new image to add in
    @param agg_method :: whether to sum, average, etc. See options supported.
    @param index :: image index used when calculating incremental statistics like the average

    Returns :: result from aggregating (sum, average, etc.) the new image

    """
    if agg_method is None:
        agg_method = 'sum'

    if 'sum' == agg_method:
        acc = np.add(acc, img_data)
    elif 'average' == agg_method:
        acc = np.add((index-1)*acc/index, img_data/index)
        __AGG_IMG_IDX += 1

    return acc

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
    ALPHA_NUM_SPLIT_RE = re.compile('([0-9]+)')
    return [ int(c) if c.isdigit() else c for c in ALPHA_NUM_SPLIT_RE.split(path_str) ]

def _read_img(filename, file_extension=None):
    """
    Read one image and return it as a 2d numpy array

    @param filename :: name of the image file, can be relative or absolute path
    @param file_extension :: extension and effectively format to use ('tiff', 'fits')
    """
    if file_extension in ['fits', 'fit']:
        pyfits = _import_pyfits()
        imgs = pyfits.open(filename)
        if len(imgs) < 1:
            raise RuntimeError(
                "Could not load at least one FITS image/table file from: {0}".format(sample_path))

        # Input fits files always contain a single image
        img_arr = imgs[0].data

    elif file_extension in ['tiff', 'tif', 'png']:
        skio = _import_skimage_io()
        img_arr = skio.imread(filename)

    else:
        raise ValueError("Don't know how to load a file with this extension: {0}".format(file_extension))

    return img_arr

def _read_listed_files(files, slice_img_shape, file_extension, dtype):
    """
    Read several images in a row into a 3d numpy array. Useful when reading all the sample
    images, or all the flat or dark images.

    @param files :: list of image file paths given as strings
    @param slice_img_shape :: shape of every image
    @param file_extension :: file name extension if fixed (to set the expected image format)
    @param dtype :: data type for the output numpy array

    Returns:: a 3d data volume with the size of the first (outermost) dimension equal
    to the number of files, and the sizes of the second and third dimensions equal to
    the sizes given in the input slice_img_shape
    """
    data = np.zeros((len(files), slice_img_shape[0], slice_img_shape[1]), dtype=dtype)
    for idx, in_file in enumerate(files):
        try:
            data[idx, :, :] = _read_img(in_file, file_extension)
        except IOError as exc:
            raise RuntimeError("Could not load file {0} from {1}. Error details: {2}".
                               format(ifile, sample_path, str(exc)))

    return data

def get_flat_dark_stack(field_path, field_prefix, file_prefix, file_extension, img_shape, data_dtype):
    """
    Load the images of the flat/dark/other field and calculate an average of them.

    @param field_path :: path to the images
    @param field_prefix :: prefix for the images of the flat/dark/other field images (filter).
    example: OB, DARK, WHITE, etc.

    @param file_extension :: extension string to look for file names
    @param img_shape :: shape that every image should have
    @param data_dtype :: output data type

    Returns :: numpy array with an average (pixel-by-pixel) of the flat/dark/other field images
    """
    avg = None
    if field_prefix:
        if not file_prefix:
            file_prefix = ''
        files_match = glob.glob(os.path.join(field_path,
                                             "{0}*.{1}".format(field_prefix, file_extension)))
        if len(files_match) <= 0:
            print("Could not find any flat field / open beam image files in: {0}".
                  format(flat_field_prefix))
        else:
            imgs_stack = _read_listed_files(files_match, img_shape, file_extension, data_dtype)
            avg = np.mean(imgs_stack, axis=0)

    return avg

# This could become a command class. There's already many parameters and it's
# likely that there will be more.
def read_stack_of_images(sample_path, flat_field_path=None, dark_field_path=None,
                         file_extension='tiff', file_prefix=None,
                         flat_field_prefix=None, dark_field_prefix=None,
                         verbose=True):
    """
    Reads a stack of images into memory, assuming dark and flat images
    are in separate directories.

    If several files are found in the same directory (for example you
    give image0001.fits and there's also image0002.fits,
    image0003.fits) these will also be loaded as the usual convention
    in ImageJ and related imaging tools, using the last digits to sort
    the images in the stack.

    @param sample_path :: path to sample images. Can be a file or directory

    @param flat_field_path :: (optional) path to open beam / white image(s).
    Can be a file or directory

    @param dark_field_path :: (optional) path to dark field image(s).
    Can be a file or directory

    @param file_extension :: file extension (typically 'tiff', 'tif', 'fits',
    or 'fit' (not including the dot)

    @param file_prefix :: prefix for the image files (example: IMAT00), to filter
    files that may be in the same directories but should not be loaded

    @param flat_field_prefix :: prefix for the flat field image files

    @param dark_field_prefix :: prefix for the dark field image files

    @param verbose :: verbose (some) output

    Returns :: 3 numpy arrays: input data volume (3d), average of flatt images (2d),
    average of dark images(2d)
    """
    SUPPORTED_EXTS = ['tiff', 'tif', 'fits', 'fit', 'png']

    if file_extension not in SUPPORTED_EXTS:
        raise ValueError("File extension not supported: {0}. Supported extensions: {1}".
                         format(file_extension, SUPPORTED_EXTS))

    sample_path = os.path.expanduser(sample_path)

    if verbose:
        print "Loading stack of images from {0}".format(sample_path)

    if not file_prefix:
        file_prefix = ''
    files_match = glob.glob(os.path.join(sample_path,
                                         "{0}*.{1}".format(file_prefix, file_extension)))
    if len(files_match) <= 0:
        raise RuntimeError("Could not find any image files in {0}, with prefix: {1}, extension: {2}".
                           format(sample_path, file_prefix, file_extension))

    files_match.sort(key=_alphanum_key_split)

    if verbose:
        print "Found {0} image files in {1}".format(len(files_match), sample_path)

    # It is assumed that all images have the same size and properties as the first.
    try:
        first_img = _read_img(files_match[0], file_extension)
    except RuntimeError as exc:
        raise RuntimeError(
            "Could not load at least one image file from: {0}. Details: {1}".
            format(sample_path, str(exc)))

    data_dtype = first_img.dtype
    # usual type in fits with 16-bit pixel depth
    if '>i2' == data_dtype:
        data_dtype = np.uint16

    img_shape = first_img.shape
    sample_data = _read_listed_files(files_match, img_shape, file_extension, data_dtype)

    flat_avg = get_flat_dark_stack(flat_field_path, flat_field_prefix,
                                   flat_field_prefix, file_extension,
                                   img_shape, data_dtype)

    dark_avg = get_flat_dark_stack(dark_field_path, flat_field_prefix,
                                   dark_field_prefix, file_extension,
                                   img_shape, data_dtype)

    return sample_data, flat_avg, dark_avg

def save_recon_as_vertical_slices(recon_data, output_dir, img_format='tiff',
                                  name_prefix='out_recon_slice', zero_fill=6):
    """
    Save reconstructed volume (3d) into a series of slices along the Z axis (outermost numpy dimension)

    @param data :: data as images/slices stores in numpy array
    @param output_dir :: where to save the files
    @param name_prefix :: prefix for the names of the images - an index is appended to this prefix
    @param zero_fill :: number of zeros to pad the image/slice index number
    """
    make_dirs_if_needed(output_dir)
    min_pix = np.amin(recon_data)
    max_pix = np.amax(recon_data)
    for idx in range(0, recon_data.shape[0]):
        write_image(recon_data[idx, :, :], min_pix, max_pix,
                    os.path.join(output_dir, name_prefix + str(idx).zfill(zero_fill)),
                    img_format=img_format, dtype='uint16')

def save_recon_as_horizontal_slices(recon_data, out_horiz_dir, img_format='tiff',
                                    name_prefix='out_recon_horiz_slice', zero_fill=6):
    """
    Save reconstructed volume (3d) into a series of slices along the Y axis (second numpy dimension)

    @param data :: data as images/slices stores in numpy array
    @param output_dir :: where to save the files
    @param name_prefix :: prefix for the names of the images throughout the horizontal axis
    @param zero_fill :: number of zeros to pad the image/slice index number. This index is appended to
    the prefix
    """
    make_dirs_if_needed(out_horiz_dir)
    for idx in range(0, recon_data.shape[1]):
        write_image(recon_data[:, idx, :], np.amin(recon_data), np.amax(recon_data),
                    os.path.join(out_horiz_dir, name_prefix + str(idx).zfill(zero_fill)),
                    img_format=img_format, dtype='uint16')

def save_recon_netcdf(recon_data, output_dir, filename='tomo_recon_vol.nc'):
    """
    A netCDF, for compatibility/easier interoperation with other tools

    @param recon_data :: reconstructed data volume. A sequence of images will be saved from this
    @param output_dir :: where the outputs are being saved
    @param filename :: name for the NetCDF file
    """
    try:
        from scipy.io import netcdf_file
    except ImportError as exc:
        print " WARNING: could not save NetCDF file. Import error: {0}".format(exc)

    xsize = recon_data.shape[0]
    ysize = recon_data.shape[1]
    zsize = recon_data.shape[2]

    nc_path = os.path.join(output_dir, filename)
    ncfile = netcdf_file(nc_path, 'w')
    ncfile.createDimension('x', xsize)
    ncfile.createDimension('y', ysize)
    ncfile.createDimension('z', zsize)
    print " Creating netCDF volume data variable"
    dtype = 'int16'
    data = ncfile.createVariable('data', np.dtype(dtype).char, ('x','y','z'))
    print " Data shape: {0}".format(data.shape)
    print " Loading/assigning data..."

    # handle differences in pixel type
    save_data = recon_data
    if recon_data.dtype != dtype:
        save_data = np.zeros(recon_data.shape, dtype='float32')
        max_vol = np.amax(recon_data)
        min_vol = np.amin(recon_data)
        vol_range = max_vol - min_vol
        scale_factor = (np.iinfo(dtype).max - np.iinfo(dtype).min) / vol_range
        save_data = scale_factor * (recon_data - min_vol)
        save_data = save_data.astype(dtype=dtype)

    data[:, :, :] = save_data[0:xsize, 0:ysize, 0:zsize]
    print " Closing netCDF file: {0}".format(nc_path)
    ncfile.close()

def self_save_zipped_scripts(output_path, this_path):
    """
    Self-save a python file and its subpackages.

    @param output_path :: path where to save
    @param this_path :: full path to 'this' file, the file that wants to self-save itself.
    """

    def _zipdir(path, ziph):
        # ziph is zipfile handle
        for root, _, files in os.walk(path):
            for indiv_file in files:
                # Write all files, with the exception of the pyc's
                exclude_extensions = ['pyc']
                if not indiv_file.endswith(tuple(exclude_extensions)):
                    ziph.write(os.path.join(root, indiv_file))

    scripts_path = os.path.dirname(this_path)

    make_dirs_if_needed(output_path)
    print ("Saving myself (reconstruction scripts) from: {0} in: {1}".
           format(scripts_path, os.path.abspath(output_path)))
    import zipfile
    # os.path.join(output_path, ... )
    RECON_SCRIPTS_PKG_NAME = '0.reconstruction_scripts.zip'
    with zipfile.ZipFile(os.path.join(output_path, RECON_SCRIPTS_PKG_NAME), 'w',
                         zipfile.ZIP_DEFLATED) as zip_scripts:
        # To write just this file: zipscr.write(this_path)
        _zipdir(scripts_path, zip_scripts)
