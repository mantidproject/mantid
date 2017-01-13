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

        self._functional_args = FunctionalConfig()
        # this sets up the arguments in the parser, with the defaults from the Config file
        parser = self._functional_args.setup_parser(parser)

        self._preproc_args = PreProcConfig()
        # this sets up the arguments in the parser, with the defaults from the Config file
        parser = self._preproc_args.setup_parser(parser)

        self._postproc_args = PostProcConfig()
        # this sets up the arguments in the parser, with the defaults from the Config file
        parser = self._postproc_args.setup_parser(parser)

        self._setup_postproc_args(parser)

        self._parser = parser
        self._args = None

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

    def parse_args(self):
        """
        Prepares the arguments for retrieval via grab_options
        """
        self._args = self._parser.parse_args()

    def grab_full_config(self):
        # combine all of them together
        return ReconstructionConfig(
            self._functional_args, self._preproc_args, self._postproc_args)

    def _grab_functional_args(self):

        functional_config.debug = self._args.debug

        if self._args.debug_port is not None:
            functional_config.debug_port = self._args.debug_port

        # grab paths
        functional_config.input_dir = self._args.input_path
        functional_config.input_dir_flat = self._args.input_path_flat
        functional_config.input_dir_dark = self._args.input_path_dark
        functional_config.output_dir = self._args.output_path

        if self._args.in_img_format:
            functional_config.in_img_format = self._args.in_img_format

        if self._args.out_img_format:
            functional_config.out_img_format = self._args.out_img_format

        if self._args.cor:
            functional_config.cor = int(self._args.cor)

        if self._args.find_cor:
            functional_config.find_cor = self._args.find_cor

        if self._args.verbosity:
            functional_config.verbosity = self._args.verbosity

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

        if self._args.crop_before_normalize:
            pre_config.crop_before_normalize = self._args.crop_before_normalize

        if self._args.median_filter_mode:
            if self._args.median_filter_mode is not None:
                pre_config.median_filter_mode = self._args.median_filter_mode

        if self._args.mcp_corrections:
            pre_config.mcp_corrections = self._args.mcp_corrections

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
        if self._args.circular_mask:
            config.circular_mask = float(self._args.circular_mask)

        if self._args.cut_off:
            config.cut_off_level = float(self._args.cut_off)

        if self._args.out_median_filter:
            config.median_filter_size = float(self._args.out_median_filter)

        return config
