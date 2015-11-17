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
import numpy as np
import os

try:
    import pyfits
except ImportError:
    # In Anaconda python, the pyfits package is in a different place, and this is what you frequently
    # find on windows.
    try:
        import astropy.io.fits as pyfits
    except ImportError:
        raise ImportError("Cannot find the package 'pyfits' which is required to read/write FITS image files")

try:
    from skimage import io as skio
    from skimage import exposure
except ImportError as exc:
    raise ImportError("Could not find the package skimage and its subpackages "
                      "io and exposure which are required to support several "
                      "image formats. Error details: {0}".format(exc))

import numpy as np
import warnings
import os
import pyfits
import glob

__agg_img_idx = 0

def _make_dirs_if_needed(dirname):
    """
    Makes sure that the directory needed (for example to save a file)
    exists, otherwise creates it.

    @param dirname :: (output) directory to check

    """
    absname = os.path.abspath(dirname)
    print "Absolute path out: ", absname #TODO
    if not os.path.exists(absname):
        os.makedirs(absname)

def _write_image(img_data, min_pix, max_pix, filename, img_format=None, dtype=None, rescale_intensity=False):
    """
    Output image data to a file, in a given image format.
    Assumes that the output directory exists (must be checked before).

    @param img_data :: image data in the usual numpy representation

    @param min_pix ::
    @param max_pix ::

    @param filename :: file name, including directory and extension
    @param img_format :: image file format
    @param dtype :: can be used to force a pixel type, otherwise the type
    of the input data is used

    Returns:: name of the file saved
    """
    if not img_format:
        img_format = 'tiff'
    filename = filename + '.' + img_format

    #dtype = 'uint8'
    #img_data = scipy.misc.bytescale(img_data)

    # from bigger to smaller type, example: float32 => uint16
    if dtype and img_data.dtype != dtype:
        #img_data = np.array(img_data, dtype=dtype)
        #img_data.astype(dtype=dtype)
        old_img_data = img_data
        img_data = np.zeros(old_img_data.shape, dtype='float32')
        pix_range = max_pix - float(min_pix)
        scale_factor = (np.iinfo(dtype).max - np.iinfo(dtype).min) / pix_range

        too_verbose = False
        if too_verbose:
            print "pix min: {0}, max: {1}, scale_factor: {2}".format(min_pix, max_pix, scale_factor)
        img_data = scale_factor * (old_img_data - min_pix)
        img_data = img_data.astype(dtype=dtype)
        #print "re-scaled img_data, min, max: {0}, {1}".format(np.amin(img_data), np.amax(img_data))

    # this rescale intensity would ignore the range of other images in the stack
    # in addition, it clips if the original values are below/above the destination type limits
    if False:# and rescale_intensity:
        img_data = exposure.rescale_intensity(img_data, out_range=dtype)#'uint16')

    _USING_PLUGIN_TIFFFILE = True # TODO
    if img_format == 'tiff' and _USING_PLUGIN_TIFFFILE:
        skio.imsave(filename, img_data, plugin='tifffile')
    else:
        skio.imsave(filename, img_data, plugin='freeimage') # freeimage / plugin='freeimage)

    return filename

# TODO: refactor to try to share code with read_stack (fits/tiff reading)
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
                                        "*.{1}".format(file_prefix, file_extension)))

    if len(img_files) <= 0:
        raise RuntimeError("No image files found in " + path)

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
        __agg_img_idx = 1

    for ifile in img_files:
        hdu = None
        try:
            hdu = pyfits.open(ifile)
        except IOError as exc:
            print "Got I/O exception trying to open and load {0}: {1}. Ignoring and going on.".format(
                ifile, str(exc))
            continue

        accum = self._agg_img(accum, hdu[0].data, agg_method=agg_method)

    return accum

def _agg_img(self, acc, img_data, agg_method=None, index=1):
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
    if None == agg_method:
        agg_method = self._default_agg_method

    if 'sum' == agg_method:
        acc = np.add(acc, img_data)
    elif 'average' == agg_method:
        acc = np.add((index-1)*acc/index, img_data/index)
        __agg_img_idx += 1

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


# TODO: add flat_files_prefix, dark_files_prefix logic
def read_stack_of_images(sample_path, open_beam_path=None, dark_field_path=None,
                         file_extension=None, file_prefix=None, flat_files_prefix=None,
                         dark_files_prefix=None,
                         minIdx=-1, maxIdx=-1):
    """
    Reads a stack of images into memory, assuming dark and flat images
    are in separate directories.

    If several files are found in the same directory (for example you
    give image0001.fits and there's also image0002.fits,
    image0003.fits) these will also be loaded as the usual convention
    in ImageJ and related imaging tools, using the last digits to sort
    the images in the stack.
    
    @param file_extension ::  (not including the dot)

    @param sample_path :: path to sample images. Can be a file or directory
    @param open_beam_path :: (optional) path to open beam / white image(s). Can be a file or directory
    @param dark_field_path :: (optional) path to dark field image(s). Can be a file or directory
    @param minIdx :: enforce this minimum image index, lower indices will be ignored
    @param maxIdx :: enforce this maximum image index, higher indices will be ignored

    Returns :: 3 numpy arrays: input data volume (3d), average of flatt images (2d), average of 
    dark images(2d)
    """
    # TODO: check file_extension
    sample_path = os.path.expanduser(sample_path)

    if verbose:
        print "Loading stack of images from {0}".format(sample_path)

    files_list = glob.glob(os.path.join(sample_path,
                                        "{0}*.{1}".format(file_prefix, file_extension)))
    if len(files) <= 0:
        raise RuntimeError("No image files found in " + sample_path)

    files.sort(key=self._alphanum_key_split)

    if verbose:
        print "Found {0} files".format(len(files))


    imgs = pyfits.open(files[0])
    if len(imgs) < 1:
        raise RuntimeError(
                "Could not load at least one image file from: {0}".format(sample_path))

    data_dtype = imgs[0].data.dtype
    if '>i2' == data_dtype:
        data_dtype = np.uint16

    sample_data = np.zeros((len(files), imgs[0].shape[0], imgs[0].shape[1]), dtype=data_dtype)


    for idx, ifile in enumerate(files):
        hdu = None
        try:
            hdu = pyfits.open(ifile)
            sample_data[idx, :, :] = hdu[0].data
        except IOError as exc:
            raise RuntimeError("Could not load file {0} from {1}".format(ifile, sample_path))

    flat_avg = None
    dark_avg = None # avg_image_files()

    return sample_data, flat_avg, dark_avg
