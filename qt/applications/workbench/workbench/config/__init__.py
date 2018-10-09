# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
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
