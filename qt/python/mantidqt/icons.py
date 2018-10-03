#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
Icon access. Icons are provided by qtawesome.
"""
from __future__ import absolute_import

# std imports

# third party imports
import qtawesome as qta


def get_icon(*names, **kwargs):
    """Return a QIcon corresponding to the icon name.
    See qtawesome.icon documentation for a full list of options.
    """
    return qta.icon(*names, **kwargs)
