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

import sys


class CustomTimer(object):
    """
    Custom fallback implementation for timer that provides only a basic loading bar.
    This timer bar will be used if tqdm is not present of the system.
    """

    def __init__(self, total, prefix=''):
        self._total = total
        self._iter = 0
        self._prefix = prefix + ": "
        self._bar_len = 40

    def update(self, iterations):
        self._iter += iterations

        filled_len = int(
            round(self._bar_len * self._iter / float(self._total)))

        bar = self._prefix + '[' + '=' * filled_len + \
            '-' * (self._bar_len - filled_len) + \
            "]" + str(self._iter) + ' / ' + str(self._total)

        print(bar, end='\r')
        sys.stdout.flush()

        if self._iter == self._total:
            print()

    def close(self):
        """
        Do nothing, just here to maintain compatibility with the TQDM timer
        """
        pass
