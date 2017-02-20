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

import nose

_avail_modules = {
    'filters': 'tests/filters_test/',
    'configs': 'tests/configs_test.py',
    'config': 'tests/configs_test.py',
    'data': 'tests/imgdata_test.py',
    'helper': 'tests/helper_test.py',
    'parallel': 'tests/parallel_test/',
    'tools': 'tests/recon/tools/',
    'all': 'tests/'
}


def _run_tests(args):
    try:
        args = args[1]

        if args in ['-h', '--help']:
            # just go inside the except block
            raise IndexError

    except IndexError:
        print(
            'Please specify the folder/test file to be executed, or one of the available modules: {0}'.
            format(_avail_modules.keys()))
        return

    try:
        # check if a module name was passed
        test_path = str(_avail_modules[args])
    except KeyError:
        # it wasn't an available module, maybe a full path
        test_path = args

    print("Running tests from", test_path)

    import os
    test_path = os.path.expandvars(os.path.expanduser(test_path))

    try:
        # for verbose run add [test_path, "-vv", "--collect-only"]
        nose.run(defaultTest=test_path, argv=[test_path])
    except ImportError:
        print('Module/test not found, try passing the path to the test \
        (e.g. tests/recon/configs_test.py) or one of the available modules: {0}'
              .format(_avail_modules.keys()))
        return


if __name__ == '__main__':
    import sys
    _run_tests(sys.argv)
