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


def check_version_info():
    import sys
    python_version = sys.version_info
    if python_version < (2, 7, 0):
        raise RuntimeError(
            "Not running this test as it requires Python >= 2.7. Version found: {0}".
            format(python_version))


def main():
    import pydevd
    pydevd.settrace('localhost', port=59003,
                    stdoutToServer=True, stderrToServer=True)

    check_version_info()

    from tomo_argparser import ArgumentParser
    arg_parser = ArgumentParser()
    arg_parser.parse_args()

    config = arg_parser.grab_full_config()

    from recon.helper import Helper
    helper = Helper(config)

    # first call, start timer
    helper.total_reconstruction_timer()

    if config.find_cor:
        # run find_center stuff
        import recon.find_cor
        recon.find_cor.execute(config)
    else:
        # run recon stuff
        import recon.runner
        import sys
        cmd_line = " ".join(sys.argv)
        recon.runner.execute(config, cmd_line)

    # end timer
    helper.total_reconstruction_timer()

main()
