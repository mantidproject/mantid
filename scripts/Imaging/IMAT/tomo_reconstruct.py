# Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
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
Do a tomographic reconstruction, including:
- Pre-processing of input raw images,
- 3d volume reconstruction using a third party tomographic reconstruction tool
- Post-processing of reconstructed volume
- Saving reconstruction results (and pre-processing results, and self-save this script and subpackages)

This command line script and the classes and packages that it uses are prepared so that
they can be run from Mantid, locally (as a process) or remotely (through the tomographic reconstruction
GUI remote job submission, or the remote algorithms).

Example command lines:

ipython -- tomo_reconstruct.py --help

ipython -- scripts/Imaging/IMAT/tomo_reconstruct.py\
 --input-path=../tomography-tests/stack_larmor_metals_summed_all_bands/ --output-path=test_REMOVE_ME\
 --tool tomopy --algorithm gridrec  --cor 123 --max-angle 360 --in-img-format=tiff\
 --region-of-interest='[5, 252, 507, 507]' --rotation=-1

ipython -- scripts/Imaging/IMAT/tomo_reconstruct.py\
 --input-path=../tomography-tests/stack_larmor_metals_summed_all_bands/ --output-path=test_REMOVE_ME\
 --tool tomopy --algorithm sirt --num-iter 10  --cor 123 --max-angle 360 --in-img-format=tiff\
 --out-img-format png --region-of-interest='[5, 252, 507, 507]' --rotation=-1

ipython -- scripts/Imaging/IMAT/tomo_reconstruct.py\
 --input-path=../tomography-tests/stack_larmor_metals_summed_all_bands/ --output-path=test_REMOVE_ME\
 --tool astra --algorithm FP3D_CUDA  --num-iter 10  --cor 123 --max-angle 360 --in-img-format=tiff\
 --region-of-interest='[5, 252, 507, 507]' --rotation=-1
"""

# find first the package/subpackages in the path of this file.
import sys
from sys import path
import os
from os import path
# So insert in the path the directory that contains this file
sys.path.insert(0, os.path.split(path.dirname(__file__))[0])

from IMAT.tomorec import reconstruction_command as tomocmd
import IMAT.tomorec.configs as tomocfg

def setup_cmd_options():
    """
    Build an argument parser

    Returns :: Python ArgumentParser set up and ready to parse command line arguments
    """

    import argparse

    parser = argparse.ArgumentParser(description='Run tomographic reconstruction via third party tools')

    grp_req = parser.add_argument_group('Mandatory/required options')

    grp_req.add_argument("-i","--input-path", required=True, type=str, help="Input directory")

    grp_req.add_argument("-o","--output-path", required=True, type=str,
                         help="Where to write the output slice images (reconstructred volume)")

    grp_req.add_argument("-c","--cor", required=True, type=float,
                         help="Center of rotation (in pixels). rotation around y axis is assumed")

    grp_recon = parser.add_argument_group('Reconstruction options')

    grp_recon.add_argument("-t","--tool", required=False, type=str,
                           help="Tomographic reconstruction tool to use")

    grp_recon.add_argument("-a","--algorithm", required=False, type=str,
                           help="Reconstruction algorithm (tool dependent)")

    grp_recon.add_argument("-n","--num-iter", required=False, type=int,
                           help="Number of iterations (only valid for iterative methods "
                           "(example: SIRT, ART, etc.).")

    grp_recon.add_argument("--max-angle", required=False, type=float,
                           help="Maximum angle (of the last projection), assuming first angle=0, and "
                           "uniform angle increment for every projection (note: this "
                           "is overriden by the angles found in the input FITS headers)")

    grp_pre = parser.add_argument_group('Pre-processing of input raw images/projections')

    grp_pre.add_argument("--input-path-flat", required=False, default=None,
                         type=str, help="Input directory for flat images")

    grp_pre.add_argument("--input-path-dark", required=False, default=None,
                         type=str, help="Input directory for flat images")

    img_formats = ['tiff', 'fits', 'tif', 'fit', 'png']
    grp_pre.add_argument("--in-img-format", required=False, default='fits', type=str,
                         help="Format/file extension expected for the input images. Supported: {0}".
                         format(img_formats))

    grp_pre.add_argument("--out-img-format", required=False, default='tiff', type=str,
                         help="Format/file extension expected for the input images. Supported: {0}".
                         format(img_formats))

    grp_pre.add_argument("--region-of-interest", required=False, type=str,
                         help="Region of interest (crop original "
                         "images to these coordinates, given as comma separated values: x1,y1,x2,y2. If not "
                         "given, the whole images are used.")

    grp_pre.add_argument("--air-region", required=False, type=str,
                         help="Air region /region for normalization. "
                         "If not provided, the normalization against beam intensity fluctuations in this "
                         "region will not be performed")

    grp_pre.add_argument("--median-filter-size", type=int,
                         required=False, help="Size/width of the median filter (pre-processing")

    grp_pre.add_argument("--remove-stripes", default='wf', required=False, type=str,
                         help="Methods supported: 'wf' (Wavelet-Fourier)")

    grp_pre.add_argument("--rotation", required=False, type=int,
                         help="Rotate images by 90 degrees a number of "
                         "times. The rotation is clockwise unless a negative number is given which indicates "
                         "rotation counterclocwise")

    grp_pre.add_argument("--scale-down", required=False, type=int,
                         help="Scale down factor, to reduce the size of "
                         "the images for faster (lower-resolution) reconstruction. For example a factor of 2 "
                         "reduces 1kx1k images to 512x512 images (combining blocks of 2x2 pixels into a single "
                         "pixel. The output pixels are calculated as the average of the input pixel blocks.")

    grp_pre.add_argument("--mcp-corrections", default='yes', required=False, type=str,
                         help="Perform corrections specific to images taken with the MCP detector")

    grp_post = parser.add_argument_group('Post-processing of the reconstructed volume')

    grp_post.add_argument("--circular-mask", required=False, type=float, default=0.94,
                          help="Radius of the circular mask to apply on the reconstructed volume. "
                          "It is given in [0,1] relative to the size of the smaller dimension/edge "
                          "of the slices. Empty or zero implies no masking.")

    grp_post.add_argument("--cut-off", required=False, type=float,
                          help="Cut off level (percentage) for reconstructed "
                          "volume. pixels below this percentage with respect to maximum intensity in the stack "
                          "will be set to the minimum value.")

    grp_post.add_argument("--out-median-filter", required=False, type=float,
                          help="Apply median filter (2d) on reconstructed volume with the given window size.")

    parser.add_argument("-v", "--verbose", action="count", default=1, help="Verbosity level. Default: 1. "
                        "User zero to supress outputs.")

    return parser

def grab_preproc_options(args):
    """
    Get pre-proc options from the command line (through an argument parser)

    @param parser :: arguments parser already, set up with pre-processing options

    Returns:: a pre-processing config object set up according to the user inputs in the command line
    """
    import ast

    pre_config = tomocfg.PreProcConfig()
    pre_config.input_dir = args.input_path
    pre_config.input_dir_flat = args.input_path_flat
    pre_config.input_dir_dark = args.input_path_dark

    if args.in_img_format:
        pre_config.in_img_format = args.in_img_format

    if args.out_img_format:
        pre_config.out_img_format = args.out_img_format

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
        pre_config.crop_coords = [int(val) for val in roi_coords]

    if 'yes' == args.mcp_corrections:
        pre_config.mcp_corrections = True

    if args.median_filter_size:
        if isinstance(args.median_filter_size, str) and not args.median_filter_size.isdigit():
            raise RuntimeError("The median filter size/width must be an integer")
        pre_config.median_filter_size = args.median_filter_size

    if 'wf' == args.remove_stripes:
        pre_config.stripe_removal_method = 'wavelet-fourier'

    pre_config.cor = int(args.cor)

    return pre_config

def grab_tool_alg_options(args):
    """
    Get tool and algorithm options from the command line (through an argument parser)

    @param parser :: arguments parsed already, set up with algorithm/tool options

    Returns:: an algorithm config object set up according to the user inputs in the command line
    """
    config = tomocfg.ToolAlgorithmConfig()
    config.tool = args.tool
    config.algorithm = args.algorithm

    if args.num_iter:
        if isinstance(args.num_iter, str) and not args.num_iter.isdigit():
            raise RuntimeError("The number of iterations must be an integer")
        config.num_iter = int(args.num_iter)

    return config

def grab_postproc_options(args):
    """
    Get post-processing (on the reconstructed volume) options from the command line
    (through an argument parser)

    @param parser :: arguments parsed already, set up with post-processing options

    Returns:: a post-processing object set up according to the user inputs in the command line
    """
    config = tomocfg.PostProcConfig()
    config.output_dir = args.output_path

    if args.circular_mask:
        config.circular_mask = float(args.circular_mask)

    if args.cut_off:
        config.cut_off_level = float(args.cut_off)

    if args.out_median_filter:
        config.median_filter_size = float(args.out_median_filter)

    return config


def main_tomo_rec():
    # several dependencies (numpy, scipy) are too out-of-date in standard Python 2.6
    # distributions, as found for example on rhel6
    vers = sys.version_info
    if vers < (2,7,0):
        raise RuntimeErrorn("Not running this test as it requires Python >= 2.7. Version found: {0}".
                            format(vers))

    import inspect

    import IMAT.tomorec.io as tomoio

    arg_parser = setup_cmd_options()
    args = arg_parser.parse_args()

    # Save myself early. Save command this command line script and all packages/subpackages
    tomoio.self_save_zipped_scripts(args.output_path,
                                    os.path.abspath(inspect.getsourcefile(lambda:0)))

    # Grab and check pre-processing options + algorithm setup + post-processing options
    preproc_config = grab_preproc_options(args)
    alg_config = grab_tool_alg_options(args)
    postproc_config = grab_postproc_options(args)

    cmd_line = " ".join(sys.argv)
    cfg = tomocfg.ReconstructionConfig(preproc_config, alg_config, postproc_config)
    # Does all the real work
    cmd = tomocmd.ReconstructionCommand()
    cmd.do_recon(cfg, cmd_line=cmd_line)


if __name__=='__main__':
    main_tomo_rec()
