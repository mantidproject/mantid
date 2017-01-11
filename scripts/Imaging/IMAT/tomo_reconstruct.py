from __future__ import (absolute_import, division, print_function)
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
import os
from os import path
# So insert in the path the directory that contains this file
sys.path.insert(0, os.path.split(path.dirname(__file__))[0])  # noqa

from tomorec import reconstruction_command as tomocmd
import tomorec.configs as tomocfg


def main_tomo_rec():
    # several dependencies (numpy, scipy) are too out-of-date in standard Python 2.6
    # distributions, as found for example on rhel6

    vers = sys.version_info
    if vers < (2, 7, 0):
        raise RuntimeError(
            "Not running this test as it requires Python >= 2.7. Version found: {0}".
            format(vers))

    import inspect
    import pydevd
    pydevd.settrace('localhost', port=59003,
                    stdoutToServer=True, stderrToServer=True)

    import IMAT.tomorec.io as tomoio

    arg_parser = setup_cmd_options()
    args = arg_parser.parse_args()

    # Grab and check pre-processing options + algorithm setup +
    # post-processing options
    preproc_config = grab_preproc_options(args)
    alg_config = grab_tool_alg_options(args)
    postproc_config = grab_postproc_options(args)

    cmd_line = " ".join(sys.argv)
    cfg = tomocfg.ReconstructionConfig(preproc_config, alg_config,
                                       postproc_config)

    # Does all the real work
    cmd = tomocmd.ReconstructionCommand()
    # start the whole execution timer
    cmd.total_reconstruction_timer()

    if (args.find_cor):
        cmd.tomo_print(" >>> Finding COR <<<")
        cmd.find_center(cfg)
    else:
        # Save myself early. Save command this command line script and all
        # packages/subpackages
        tomoio.self_save_zipped_scripts(
            args.output_path,
            os.path.abspath(inspect.getsourcefile(lambda: 0)))
        cmd.tomo_print(" >>> Running reconstruction <<<")
        cmd.do_recon(cfg, cmd_line=cmd_line)

    # end the whole execution timer
    cmd.total_reconstruction_timer()


if __name__ == '__main__':
    main_tomo_rec()
