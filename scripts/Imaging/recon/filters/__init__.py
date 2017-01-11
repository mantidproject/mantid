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

IMPORT_ERR_MSG = ("Inconsistency found. Could not import {0} which should be "
                  "available in this package. Details/reason: {1}")

try:
    import preproc
except ImportError as exc:
    raise ImportError(
        IMPORT_ERR_MSG.format("'preproc' (input/output routines)", exc))
try:
    import postproc
except ImportError as exc:
    raise ImportError(
        IMPORT_ERR_MSG.format("'postproc' (input/output routines)", exc))
