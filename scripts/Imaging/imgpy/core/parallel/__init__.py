from __future__ import (absolute_import, division, print_function)

"""
parallel.shared_mem: Runs a function in parallel.
                     Expects and uses a single 3D shared memory array between the processes.
parallel.two_shared_mem: Runs a function in parallel.
                         Expects and uses two 3D shared memory arrays between the processes.
parallel.exclusive_mem: Runs a function in parallel.
                         Uses a 3D memory array, but each process will copy the piece of data
                         it's processing, before processing it, if the data is in any way read
                         or modified, triggering the copy-on-write.
"""

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
