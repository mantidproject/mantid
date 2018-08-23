#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
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
""" Main configuration module.

A singleton instance called CONF is defined. Modules wishing to access the settings
should import the CONF object as

    from workbench.config import CONF

and use it to access the settings
"""
from __future__ import (absolute_import, unicode_literals)

from workbench.config.user import UserConfig

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
ORGANIZATION = 'mantidproject'
ORG_DOMAIN = 'mantidproject.org'
APPNAME = 'mantidworkbench'

# Iterable containing defaults for each configurable section of the code
# General application settings are in the main section
DEFAULTS = {
    'main': {
      'high_dpi_scaling': True,
      'window/size': (1260, 740),
      'window/position': (10, 10),
    }
}

# -----------------------------------------------------------------------------
# 'Singleton' instance
# -----------------------------------------------------------------------------
CONF = UserConfig(ORGANIZATION, APPNAME, defaults=DEFAULTS)
