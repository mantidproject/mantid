# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Utility functions to deal with fetching fonts"""

# third-party imports
from qtpy.QtGui import QFont, QFontDatabase

import mantid.kernel.environment as mtd_env

# Plain-text fonts
MONOSPACE = [
    "Consolas",
    "Monospace",
    "DejaVu Sans Mono",
    "Bitstream Vera Sans Mono",
    "Andale Mono",
    "Liberation Mono",
    "Courier New",
    "Courier",
    "monospace",
    "Fixed",
    "Terminal",
]


# Define reasonable point sizes on various OSes
if mtd_env.is_mac():
    MONOSPACE = ["Menlo"] + MONOSPACE
    PT_SIZE = 12
elif mtd_env.is_windows():
    PT_SIZE = 10
elif mtd_env.is_linux():
    if mtd_env.is_ubuntu():
        MONOSPACE = ["Ubuntu Mono"] + MONOSPACE
        PT_SIZE = 11
    else:
        MONOSPACE = ["Ubuntu Mono", "DejaVu Sans Mono"] + MONOSPACE
        PT_SIZE = 10
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
