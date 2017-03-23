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


def register_into(parser):
    """
    This function will import all the filters, and then call cli_register(parser) on them.
    """

    from . import circular_mask
    from . import crop_coords
    from . import cut_off
    from . import gaussian
    from . import median_filter
    from . import minus_log
    from . import normalise_by_air_region
    from . import normalise_by_flat_dark
    from . import outliers
    from . import rebin
    from . import rotate_stack
    from . import ring_removal
    from . import stripe_removal
    from . import value_scaling

    MODULES = [
        circular_mask, crop_coords, cut_off, gaussian, median_filter,
        minus_log, normalise_by_air_region, normalise_by_flat_dark, outliers,
        rebin, rotate_stack, ring_removal, stripe_removal, value_scaling
    ]

    for m in MODULES:
        m.cli_register(parser)

    return parser