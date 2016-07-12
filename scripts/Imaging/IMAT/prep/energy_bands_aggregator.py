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

import glob
import os
import re
import sys
import time

try:
    from skimage import io as skio
except ImportError:
    raise ImportError("Cannot find the package 'skimage.io' which is required to read/write image files in "
                      "TIFF and other formats")

try:
    from skimage import exposure
except ImportError:
    raise ImportError("Unable to import package skimage.exposure which is required to write output image "
                      "files with correct range of values")

# Ideally, use freeimage plugin. That provides good support for tiff, png, and more
# Backup solution: tifffile required for tiff format; skimage provides basic png, etc. support.
try:
    skio.use_plugin('freeimage')
except RuntimeError:
    FREEIMG_ERR_MSG = "Could not find the plugin 'freeimage' in skimage."
    # Because tifffile is going to be imported later on (in skio.imsave()) - check that it can be imported
    if not __package__:
        try:
            # it is unused here - it will be used by the tiff skio plugin
            #pylint: disable=unused-import
            import tifffile
            _USING_PLUGIN_TIFFFILE = True
        except ImportError:
            raise ImportError("Cannot find the package 'tifffile' which is required to read/write TIFF image "
                              "files." + FREEIMG_ERR_MSG)
    else:
        try:
            #pylint: disable=no-name-in-module
            from . import tifffile
            _USING_PLUGIN_TIFFFILE = True
        except ImportError:
            try:
                import tifffile
                _USING_PLUGIN_TIFFFILE = True
            except ImportError:
                raise ImportError("Cannot find the package 'tifffile' in the system or together with this "
                                  "module. It is required to read/write TIFF image files. " + FREEIMG_ERR_MSG)


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
    import numpy as np
except ImportError:
    raise ImportError("Cannot find the package 'numpy' which is required to calculate aggregated images")

# alternative for when other plugins are not available. It can be distributed
# and copied on remote machines where there are no other options
_USING_PLUGIN_TIFFFILE = False

#pylint: disable=too-many-instance-attributes
class EnergyBandsAggregator(object):
    """
    Combines energy bands, producing stacks of images with one image per projection angle from
    energy-banded stacks of images (in which there can be thousands of images per projection angle).

    Usage example:
    eb_agg = Energy_Bands_Aggregator()
    eb_agg.agg_angles("~/test/LARMOR/test_few_angles/", "output_stack_all_bands")

    where the input directory (first argument) is expected to  have several directories like 'angle0',
    'angle1'... 'angle100', etc.
    """

    default_out_path = None
    supported_aggs = None
    supported_out_formats = None
    default_out_format = None

    def __init__(self, out_format='tiff', out_type='uint16'):
        """
        @param out_format :: format for the output image files. The list of supported format
        is given in in the attribute supported_out_formats. Default: tiff

        @param out_type :: pixel type for the output image files. Default: 2 bytes integer
        """
        # a default output path name for when the user doesn't provide any
        self.default_out_path = 'out_agg_stack'
        # types of aggregation across energy bands
        self.supported_aggs = ['sum', 'average']
        # format of the output images.
        self.supported_out_formats = ['tiff', 'png']
        # because tiff is the most common choice of third party tools
        self.default_out_format = 'tiff'
        # the default one
        self._out_format = out_format

        # plain sum by default
        self._default_agg_method = self.supported_aggs[0]
        # default pixel type of output files. Only this one supported for now.
        self._default_output_type = out_type
        # to handle the image index when doing incremental calculations
        self.__img_idx = 1

    def _write_image(self, img_data, filename, img_format=None, dtype=None):
        """
        Output image data to a file, in a given image format.
        Assumes that the output directory exists (must be checked before).

        @param img_data :: image data in the usual numpy representation
        @param filename :: file name, including directory and extension
        @param img_format :: image file format
        @param dtype :: can be used to force a pixel type, otherwise the type
                        of the input data is used

        Returns :: name of the file saved
        """
        if not img_format:
            img_format = self.default_out_format
        filename = filename + '.' + img_format

        if dtype and img_data.dtype != dtype:
            img_data = np.array(img_data, dtype=dtype)

        if img_format == 'tiff' and _USING_PLUGIN_TIFFFILE:
            img_data = exposure.rescale_intensity(img_data, out_range='uint16')
            skio.imsave(filename, img_data, plugin='tifffile')
        else:
            img_data = exposure.rescale_intensity(img_data, out_range='uint16')
            skio.imsave(filename, img_data)

        return filename

    def _make_dirs_if_needed(self, dirname):
        """
        Makes sure that the directory needed to save the file exists, or creates it

        @param dirname :: (output) directory to check
        """
        absname = os.path.abspath(dirname)
        if not os.path.exists(absname):
            os.makedirs(absname)

    def _alphanum_key_split(self, path_str):
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

    def _agg_img(self, acc, img_data, agg_method=None, index=1):
        """
        Adds in data coming from a new image/hdu. Expects a numpy array of shape (N1, N2) where
        N1 and N2 are the number of rows and columns of the images.

        @param acc :: aggregated value accumulated so far
        @param img_data :: new image to add in
        @param agg_method :: whether to sum, average, etc. See options supported.
        @param index :: image index used when calculating incremental statistics like the average

        Returns :: result from aggregating (sum, average, etc.) the new image
        """
        if agg_method is None:
            agg_method = self._default_agg_method

        if 'sum' == agg_method:
            acc = np.add(acc, img_data)
        elif 'average' == agg_method:
            acc = np.add((index-1)*acc/index, img_data/index)
            __img_idx += 1

        return acc

    #pylint: disable=too-many-arguments
    def agg_indiv_angle(self, path_proj, band_indices=None, imgExt='fits', agg_method=None,
                        verbose=True, too_verbose=False):
        """
        Aggregates (sum, average, etc.) all the energy bands selected for one projection angle and
        returns it. The aggregation is done sequentially through the potentially thousands of energy
        band images available for one projection angle. This requires memory for just two images at once.
        It aggregates between given minimum and maximum indices.

        @param path_proj :: path to the angle/projection subdirectory (ex.: '~/test/IMAT/test1/angle10').

        @param band_indices :: a typle with minimum and maximum indices of the energy bands/images
        to aggregate. Empty implies all bands. Their validity must have been checked before/elsewhere.

        @param imgExt :: extension of image files. Only fits supported for now as this is the format
                         that we get from the intrument/cameras.

        @param agg_method :: whether to sum, average, etc. See options supported.

        Returns :: an aggregated image as a 2-dimensional numpy array with size number of image
                   rows x columns.
        """
        img_files = glob.glob(os.path.join(path_proj,  "*[0-9]." + imgExt))
        if len(img_files) <= 0:
            raise RuntimeError("No image files found in " + path_proj)

        imgs = pyfits.open(img_files[0])
        if len(imgs) < 1:
            raise RuntimeError(
                "Could not load at least one image from path: {0}".format(path_proj))

        data_dtype = imgs[0].data.dtype
        # from fits files we usually get this, just change it to uint16
        if '>i2' == data_dtype:
            data_dtype = np.uint16
        accum = np.zeros((imgs[0].shape[0], imgs[0].shape[1]), dtype=data_dtype)

        # methods that require incremental calculations should remember to do this
        if 'average' == agg_method:
            __img_idx = 1

        # filter, keep only the files between min and max indices given
        if band_indices:
            img_files = [f for idx,f in enumerate(img_files) if
                         idx >= band_indices[0] and idx <= band_indices[1] ]

        for ifile in img_files:
            if too_verbose:
                sys.stdout.write('.')

            hdu = None
            try:
                hdu = pyfits.open(ifile)
            except IOError as exc:
                print "Got I/O exception trying to open and load {0}: {1}. Ignoring and going on.".format(
                    ifile, str(exc))
                continue

            accum = self._agg_img(accum, hdu[0].data, agg_method=agg_method)

        if too_verbose:
            sys.stdout.write('\n')

        if verbose:
            print "Aggregated {0} images. Stats of result image. "\
                "Max : {1}, min: {2}, per-pixel-avg: {3:.3f}, all-sum: {4}".format(
                    len(img_files), np.amax(accum), np.amin(accum), np.average(accum), np.sum(accum))

        return accum

    def _check_inputs_to_agg_angles(self, in_path, output_path, out_format, band_indices):
        """ Checks relevant user inputs"""
        if not in_path:
            raise ValueError("The input path cannot be empty")

        if None == output_path:
            output_path = self.default_out_path

        if out_format:
            self._out_format = out_format
        if not isinstance(self._out_format, str) or not self._out_format in self.supported_out_formats:
            raise ValueError("Only the following output formats are supported: {0}. Format requested: {1}".
                             format(self.supported_out_formats, self._out_format))

        if band_indices:
            if 2 != len(band_indices) or not isinstance(band_indices[0], int) or\
               not isinstance(band_indices[1], int):
                raise ValueError("Wrong min-max energy band indices given: {0}".format(band_indices))
            if band_indices[0] > band_indices[1]:
                raise ValueError("The minimum energy band index must be lower than the maximum index")

    #pylint: disable=too-many-arguments
    def agg_angles(self, in_path, output_path=None, band_indices=None,
                   agg_method='sum', out_format=None, angle_subdir_prefix='angle',
                   verbose=True, too_verbose=False, out_files_name_prefix='angle_agg_', zero_pad=6):
        """
        Aggregate a stack, processing all the angles (projections) found. Produces an output stack with
        one single image per projection into an output directory. Does not store all the images in
        memory, only one at a time.

        @param in_path :: path to the multi-energy-band (energy selective) stack of images. It is
        expected to have several directories with the prefix 'angle' and additional alphanumeric
        characters (example: 'angle0', 'angle1', etc.)

        @param band_indices :: a typle with minimum and maximum indices of the energy bands/images
        to aggregate. Empty implies all bands. This method checks the validity of the values (if) given.

        @param output_path :: where to write the output image files

        @param angle_subdir_prefix :: prefix for the individual angle subdirectories. For example,
        'angle', as used for IMAT data. Directories that do not match this are ignored.

        @param verbose :: write progress info and additional messages to the standard output

        @param too_verbose :: write more detailed progress information, for testing purposes

        @param zero_pad :: width (number of digits) to pad the index number to, in the output file
        names
        """
        self._check_inputs_to_agg_angles(in_path, output_path, out_format, band_indices)

        # in case they use '~' or similar
        in_path = os.path.expanduser(in_path)

        # This is the big loop through every angle's of the order of 1000s individual energy bands
        if verbose:
            print "Looking for projection angles in {0}".format(in_path)

        angle_subdirs = glob.glob(os.path.join(in_path, "{0}*".format(angle_subdir_prefix)))
        angle_subdirs.sort(key=self._alphanum_key_split)

        print "Found {0} projection (angle) subdirectories.".format(len(angle_subdirs))

        # Prepare output directory
        self._make_dirs_if_needed(output_path)

        start = time.time()
        for idx, adir in enumerate(angle_subdirs):
            print "Processing projection angle subdirectory: {0}".format(adir)

            # produce aggregated image
            img_data = self.agg_indiv_angle(adir, band_indices, agg_method=agg_method,
                                            verbose=verbose, too_verbose=too_verbose)

            # write into output image
            if self._default_output_type != img_data.dtype:
                img_data = img_data.astype(self._default_output_type)

            out_name = os.path.join(output_path, out_files_name_prefix + str(idx).zfill(zero_pad))

            out_name = self._write_image(img_data=img_data, filename=out_name,
                                         img_format=self._out_format, dtype=img_data.dtype)

            if verbose:
                print "Output image written in: {0}".format(out_name)
                tnow = time.time()
                print "Time elapsed: {0:.3f}".format(tnow - start)
