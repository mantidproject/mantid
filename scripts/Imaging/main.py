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
            pydevd.settrace(
                'localhost',
                port=config.func.debug_port,
                stdoutToServer=True,
                stderrToServer=True)

    if config.func.imopr:
        from imopr import imopr
        res = imopr.execute(config)
    elif config.func.aggregate:
        from aggregate import aggregate
        res = aggregate.execute(config)
    elif config.func.convert:
        from convert import convert
        res = convert.execute(config)
    else:
        from recon import recon
        cmd_line = " ".join(sys.argv)
        res = recon.execute(config, cmd_line)

    return res


if __name__ == '__main__':
    main()
