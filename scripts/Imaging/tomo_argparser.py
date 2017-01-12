from __future__ import (absolute_import, division, print_function)


# Copyright &copy; 2017-2018 ISIS Rutherford Appleton Laboratory, NScD
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
# Author: Dimitar Tasev, Mantid Development Team
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>
from recon.configs.functional_config import FunctionalConfig
from recon.configs.postproc_config import PostProcConfig
from recon.configs.preproc_config import PreProcConfig
from recon.configs.recon_config import ReconstructionConfig


class ArgumentParser(object):

    def __init__(self):
        import argparse

        parser = argparse.ArgumentParser(
            description='Run tomographic reconstruction via third party tools')

        self._setup_functional_args(parser)
        self._setup_preproc_args(parser)
        self._setup_postproc_args(parser)

        self._parser = parser
        self._args = None

    @staticmethod
    def _setup_functional_args(parser):
        """
        Setup the functional arguments for the script
        :param parser: The parser which is set up
        """
        grp_req = parser.add_argument_group('Mandatory/required options')

        grp_req.add_argument(
            "-i", "--input-path", required=True, type=str, help="Input directory")

        grp_req.add_argument(
            "-o",
            "--output-path",
            required=True,
            type=str,
            help="Where to write the output slice images (reconstructed volume)")

        grp_req.add_argument(
            "-c",
            "--cor",
            required=False,
            type=float,
            help="Provide a pre-calculated centre of rotation. If one is not provided it will be automatically "
                 "calculated "
        )

        grp_req.add_argument(
            "-f",
            "--find-cor",
            action='store_true',
            required=False,
            help="Find the center of rotation (in pixels). rotation around y axis is assumed"
        )

        grp_recon = parser.add_argument_group('Reconstruction options')

        grp_recon.add_argument(
            "-d",
            "--debug",
            required=False,
            action='store_true',
            help='Run debug to specified port, if no port is specified, it will default to 59003')

        grp_recon.add_argument(
            "-p",
            "--debug-port",
            required=False,
            type=int,
            help='Port on which a debugger is listening, if no port is specified, it will default to 59003')

        grp_recon.add_argument(
            "-t",
            "--tool",
            required=False,
            type=str,
            help="Tomographic reconstruction tool to use")

        grp_recon.add_argument(
            "-a",
            "--algorithm",
            required=False,
            type=str,
            help="Reconstruction algorithm (tool dependent)")

        grp_recon.add_argument(
            "-n",
            "--num-iter",
            required=False,
            type=int,
            help="Number of iterations (only valid for iterative methods "
                 "(example: SIRT, ART, etc.).")

        grp_recon.add_argument(
            "--max-angle",
            required=False,
            type=float,
            help="Maximum angle (of the last projection), assuming first angle=0, and "
                 "uniform angle increment for every projection (note: this "
                 "is overriden by the angles found in the input FITS headers)")

    @staticmethod
    def _setup_preproc_args(parser):
        """
        Setup the pre-processing arguments for the script
        :param parser: The parser which is set up
        """
        grp_pre = parser.add_argument_group(
            'Pre-processing of input raw images/projections')

        grp_pre.add_argument(
            "--input-path-flat",
            required=False,
            default=None,
            type=str,
            help="Input directory for flat images")

        grp_pre.add_argument(
            "--input-path-dark",
            required=False,
            default=None,
            type=str,
            help="Input directory for flat images")

        img_formats = ['tiff', 'fits', 'tif', 'fit', 'png']
        grp_pre.add_argument(
            "--in-img-format",
            required=False,
            default='fits',
            type=str,
            help="Format/file extension expected for the input images. Supported: {0}".
            format(img_formats))

        grp_pre.add_argument(
            "--out-img-format",
            required=False,
            default='tiff',
            type=str,
            help="Format/file extension expected for the input images. Supported: {0}".
            format(img_formats))

        grp_pre.add_argument(
            "--region-of-interest",
            required=False,
            type=str,
            help="Region of interest (crop original "
                 "images to these coordinates, given as comma separated values: x1,y1,x2,y2. If not "
                 "given, the whole images are used.")

        grp_pre.add_argument(
            "--air-region",
            required=False,
            type=str,
            help="Air region /region for normalization. "
                 "If not provided, the normalization against beam intensity fluctuations in this "
                 "region will not be performed")

        grp_pre.add_argument(
            "--median-filter-size",
            type=int,
            required=False,
            help="Size/width of the median filter (pre-processing")

        grp_pre.add_argument(
            "--remove-stripes",
            default='wf',
            required=False,
            type=str,
            help="Methods supported: 'wf' (Wavelet-Fourier)")

        grp_pre.add_argument(
            "--rotation",
            required=False,
            type=int,
            help="Rotate images by 90 degrees a number of "
                 "times. The rotation is clockwise unless a negative number is given which indicates "
                 "rotation counterclocwise")

        grp_pre.add_argument(
            "--scale-down",
            required=False,
            type=int,
            help="Scale down factor, to reduce the size of "
                 "the images for faster (lower-resolution) reconstruction. For example a factor of 2 "
                 "reduces 1kx1k images to 512x512 images (combining blocks of 2x2 pixels into a single "
                 "pixel. The output pixels are calculated as the average of the input pixel blocks."
        )

        grp_pre.add_argument(
            "--mcp-corrections",
            default='yes',
            required=False,
            type=str,
            help="Perform corrections specific to images taken with the MCP detector"
        )

    @staticmethod
    def _setup_postproc_args(parser):
        """
        Setup the post-processing arguments for the script
        :param parser: The parser which is set up
        """
        grp_post = parser.add_argument_group(
            'Post-processing of the reconstructed volume')

        grp_post.add_argument(
            "--circular-mask",
            required=False,
            type=float,
            default=0.94,
            help="Radius of the circular mask to apply on the reconstructed volume. "
                 "It is given in [0,1] relative to the size of the smaller dimension/edge "
                 "of the slices. Empty or zero implies no masking.")

        grp_post.add_argument(
            "--cut-off",
            required=False,
            type=float,
            help="Cut off level (percentage) for reconstructed "
                 "volume. pixels below this percentage with respect to maximum intensity in the stack "
                 "will be set to the minimum value.")

        grp_post.add_argument(
            "--out-median-filter",
            required=False,
            type=float,
            help="Apply median filter (2d) on reconstructed volume with the given window size."
        )

        parser.add_argument(
            "-v",
            "--verbose",
            action="count",
            default=1,
            help="Verbosity level. Default: 1. "
                 "User zero to supress outputs.")

    def parse_args(self):
        """
        Prepares the arguments for retrieval via grab_options
        """
        self._args = self._parser.parse_args()

    def grab_full_config(self):
        functional_args = self._grab_functional_args()
        preproc_args = self._grab_preproc_args()
        postproc_args = self._grab_postproc_args()

        # combine all of them together
        recon_config = ReconstructionConfig(
            functional_args, preproc_args, postproc_args)

        return recon_config

    def _grab_functional_args(self):
        functional_config = FunctionalConfig()

        functional_config.debug = self._args.debug

        if self._args.debug_port is not None:
            functional_config.debug_port = self._args.debug_port
        else:
            functional_config.debug_port = 59003


        # grab paths
        functional_config.input_dir = self._args.input_path
        functional_config.input_dir_flat = self._args.input_path_flat
        functional_config.input_dir_dark = self._args.input_path_dark
        functional_config.output_dir = self._args.output_path

        if self._args.cor:
            functional_config.cor = int(self._args.cor)

        if self._args.find_cor:
            functional_config.find_cor = self._args.find_cor

        # grab tools options
        functional_config.tool = self._args.tool
        functional_config.algorithm = self._args.algorithm

        if self._args.num_iter:
            if isinstance(self._args.num_iter, str) and not self._args.num_iter.isdigit():
                raise RuntimeError(
                    "The number of iterations must be an integer")
            functional_config.num_iter = int(self._args.num_iter)

        return functional_config

    def _grab_preproc_args(self):
        """
        Get pre-processing options from the command line (through an argument parser)

        :return A pre-processing config object set up according to the user inputs in the command line
        """

        import ast

        pre_config = PreProcConfig()

        if self._args.in_img_format:
            pre_config.in_img_format = self._args.in_img_format

        if self._args.out_img_format:
            pre_config.out_img_format = self._args.out_img_format

        if self._args.max_angle:
            pre_config.max_angle = float(self._args.max_angle)

        if self._args.rotation:
            pre_config.rotation = int(self._args.rotation)

        if self._args.air_region:
            coords = ast.literal_eval(self._args.air_region)
            pre_config.normalize_air_region = [int(val) for val in coords]

        if self._args.air_region:
            coords = ast.literal_eval(self._args.air_region)
            pre_config.normalize_air_region = [int(val) for val in coords]

        if self._args.region_of_interest:
            roi_coords = ast.literal_eval(self._args.region_of_interest)
            pre_config.crop_coords = [int(val) for val in roi_coords]

        if 'yes' == self._args.mcp_corrections:
            pre_config.mcp_corrections = True

        if self._args.median_filter_size:
            if isinstance(self._args.median_filter_size,
                          str) and not self._args.median_filter_size.isdigit():
                raise RuntimeError(
                    "The median filter size/width must be an integer")
            pre_config.median_filter_size = self._args.median_filter_size

        if 'wf' == self._args.remove_stripes:
            pre_config.stripe_removal_method = 'wavelet-fourier'

        return pre_config

    def _grab_postproc_args(self):
        """
            Get post-processing (on the reconstructed volume) options from the command line
            (through an argument parser)

            @param parser :: arguments parsed already, set up with post-processing options

            Returns:: a post-processing object set up according to the user inputs in the command line
            """
        config = PostProcConfig()

        if self._args.circular_mask:
            config.circular_mask = float(self._args.circular_mask)

        if self._args.cut_off:
            config.cut_off_level = float(self._args.cut_off)

        if self._args.out_median_filter:
            config.median_filter_size = float(self._args.out_median_filter)

        return config
