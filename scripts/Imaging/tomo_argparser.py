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


def grab_full_config():
    import argparse

    parser = argparse.ArgumentParser(
        description='Run tomographic reconstruction via third party tools')

    # this sets up the arguments in the parser, with defaults from the Config file
    functional_args = FunctionalConfig()
    parser = functional_args.setup_parser(parser)

    pre_args = PreProcConfig()
    parser = pre_args.setup_parser(parser)

    post_args = PostProcConfig()
    parser = post_args.setup_parser(parser)

    # parse the real arguments
    args = parser.parse_args()

    # update the configs
    functional_args.update(args)
    pre_args.update(args)
    post_args.update(args)
    # combine all of them together
    return ReconstructionConfig(functional_args, pre_args, post_args)


def _grab_functional_args(functional_config, args):

    functional_config.debug = args.debug

    if args.debug_port is not None:
        functional_config.debug_port = args.debug_port

    # grab paths
    functional_config.input_dir = args.input_path
    functional_config.input_dir_flat = args.input_path_flat
    functional_config.input_dir_dark = args.input_path_dark
    functional_config.output_dir = args.output_path

    if args.in_img_format:
        functional_config.in_img_format = args.in_img_format

    if args.out_img_format:
        functional_config.out_img_format = args.out_img_format

    if args.cor:
        functional_config.cor = int(args.cor)

    if args.find_cor:
        functional_config.find_cor = args.find_cor

    if args.verbosity:
        functional_config.verbosity = args.verbosity

    # grab tools options
    functional_config.tool = args.tool
    functional_config.algorithm = args.algorithm

    if args.num_iter:
        if isinstance(args.num_iter, str) and not args.num_iter.isdigit():
            raise RuntimeError(
                "The number of iterations must be an integer")
        functional_config.num_iter = int(args.num_iter)

    return functional_config


def _grab_preproc_args(pre_config, args):
    """
    Get pre-processing options from the command line (through an argument parser)

    :return A pre-processing config object set up according to the user inputs in the command line
    """

    import ast

    if args.max_angle:
        pre_config.max_angle = float(args.max_angle)

    if args.rotation:
        pre_config.rotation = int(args.rotation)

    if args.air_region:
        coords = ast.literal_eval(args.air_region)
        pre_config.normalize_air_region = [int(val) for val in coords]

    if args.air_region:
        coords = ast.literal_eval(args.air_region)
        pre_config.normalize_air_region = [int(val) for val in coords]

    if args.region_of_interest:
        roi_coords = ast.literal_eval(args.region_of_interest)
        pre_config.region_of_interest = [int(val) for val in roi_coords]

    if args.crop_before_normalize:
        pre_config.crop_before_normalize = args.crop_before_normalize

    if args.median_filter_mode:
        if args.median_filter_mode is not None:
            pre_config.median_filter_mode = args.median_filter_mode

    if args.mcp_corrections:
        pre_config.mcp_corrections = args.mcp_corrections

    if args.median_filter_size:
        if isinstance(args.median_filter_size,
                      str) and not args.median_filter_size.isdigit():
            raise RuntimeError(
                "The median filter size/width must be an integer")
        pre_config.median_filter_size = args.median_filter_size

    if 'wf' == args.remove_stripes:
        pre_config.stripe_removal_method = 'wavelet-fourier'

    return pre_config


def _grab_postproc_args(post_config, args):
    """
        Get post-processing (on the reconstructed volume) options from the command line
        (through an argument parser)

        @param parser :: arguments parsed already, set up with post-processing options

        Returns:: a post-processing object set up according to the user inputs in the command line
        """
    if args.circular_mask:
        post_config.circular_mask = float(args.circular_mask)

    if args.cut_off:
        post_config.cut_off_level = float(args.cut_off)

    if args.out_median_filter:
        post_config.median_filter_size = float(args.out_median_filter)

    return post_config
