# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
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
Pre-processing operations and filters that are applied on stacks of images
or individual images.
"""

ERR_MSG = ("Inconsistency found. Could not import {0} which "
           "should be available in this package. Details: {1}")

try:
    from . import filters
except ImportError as exc:
    raise ImportError(ERR_MSG.format('filters', exc))

try:
    from . import filters_adv
except ImportError as exc:
    raise ImportError(ERR_MSG.format('filters_adv', exc))
