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

# std imports
import os
import sys

# third-party imports
from qtpy.QtCore import QSettings

# local imports
from .user import UserConfig

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
ORGANIZATION = 'mantidproject'
ORG_DOMAIN = 'mantidproject.org'
APPNAME = 'mantidworkbench'

DEFAULT_SCRIPT_CONTENT = ""
if sys.version_info < (3, 0):
    DEFAULT_SCRIPT_CONTENT += "# The following line helps with future compatibility with Python 3" + os.linesep + \
                              "# print must now be used as a function, e.g print('Hello','World')" + os.linesep + \
                              "from __future__ import (absolute_import, division, print_function, unicode_literals)" + \
                              os.linesep + os.linesep

DEFAULT_SCRIPT_CONTENT += "# import mantid algorithms, numpy and matplotlib" + os.linesep + \
                          "from mantid.simpleapi import *" + os.linesep + \
                          "import matplotlib.pyplot as plt" + os.linesep + \
                          "import numpy as np" + os.linesep + os.linesep

# Iterable containing defaults for each configurable section of the code
# General application settings are in the main section
DEFAULTS = {
    'high_dpi_scaling': True,
    'MainWindow': {
        'size': (1260, 740),
        'position': (10, 10),
    },
    'project': {
        'prompt_save_on_close': True,
        'prompt_save_editor_modified': True,
        'prompt_on_deleting_workspace': False
    }
}

# -----------------------------------------------------------------------------
# 'Singleton' instance
# -----------------------------------------------------------------------------
QSettings.setDefaultFormat(QSettings.IniFormat)
CONF = UserConfig(ORGANIZATION, APPNAME, defaults=DEFAULTS)
