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

python tomo_reconstruct.py --help
"""


def check_version_info():
    import sys
    python_version = sys.version_info
    if python_version < (2, 7, 0):
        raise RuntimeError(
            "Not running this script as it requires Python >= 2.7. Version found: {0}".
            format(python_version))


def main():
    import sys
    check_version_info()

    import tomo_argparser
    config = tomo_argparser.grab_full_config()

    if config.func.debug:
        if config.func.debug_port is not None:
            import pydevd
            pydevd.settrace('localhost', port=config.func.debug_port,
                            stdoutToServer=True, stderrToServer=True)

    if config.func.find_cor:
        # run find_center stuff
        import recon.find_cor
        res = recon.find_cor.execute(config)
    elif config.func.imopr:
        import imopr.runner
        res = imopr.runner.execute(config)
    else:
        # run recon stuff
        import recon.runner
        cmd_line = " ".join(sys.argv)
        res = recon.runner.execute(config, cmd_line)

    return res

if __name__ == '__main__':
    main()
