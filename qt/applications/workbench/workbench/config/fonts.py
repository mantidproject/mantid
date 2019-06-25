# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
""" Utility functions to deal with fetching fonts
"""
from __future__ import (absolute_import, unicode_literals)

# std imports
import os
import os.path as osp
import sys

# third-party imports
from qtpy.QtGui import QFont, QFontDatabase


def is_ubuntu():
    """Return True if we're running an Ubuntu distro else return False"""
    # platform.linux_distribution doesn't exist in Python 3.5
    if sys.platform.startswith('linux') and osp.isfile('/etc/lsb-release'):
        release_info = open('/etc/lsb-release').read()
        if 'Ubuntu' in release_info:
            return True
        else:
            return False
    else:
        return False


# Plain-text fonts
MONOSPACE = ['Consolas', 'Monospace', 'DejaVu Sans Mono', 'Bitstream Vera Sans Mono',
             'Andale Mono', 'Liberation Mono', 'Courier New',
             'Courier', 'monospace', 'Fixed', 'Terminal']


# Define reasonable point sizes on various OSes
if sys.platform == 'darwin':
    MONOSPACE = ['Menlo'] + MONOSPACE
    PT_SIZE = 12
elif os.name == 'nt':
    PT_SIZE = 10
elif is_ubuntu():
    MONOSPACE = ['Ubuntu Mono'] + MONOSPACE
    PT_SIZE = 11
else:
    PT_SIZE = 9


# Cached font for this system
_TEXT_FONT_CACHE = None


def text_font():
    """Return the first monospace font for this system.
    The font is cached on first access.
    """
    global _TEXT_FONT_CACHE
    if _TEXT_FONT_CACHE is None:
        fontdb = QFontDatabase()
        families = fontdb.families()
        for family in MONOSPACE:
            if family in families:
                _TEXT_FONT_CACHE = QFont(family)
                break
        _TEXT_FONT_CACHE.setPointSize(PT_SIZE)
        _TEXT_FONT_CACHE.setFixedPitch(True)

    return _TEXT_FONT_CACHE
