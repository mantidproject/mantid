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

import subprocess

# third-party imports
from qtpy.QtGui import QFont, QFontDatabase, QColor
from qtpy.QtCore import QSettings

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


# Returns if the user is using a dark theme on macOS
def _is_theme_dark():
    try:
        result = subprocess.run(
            ["/usr/bin/defaults", "read", "-g", "AppleInterfaceStyle"],
            capture_output=True,
            text=True,
            check=False,
            timeout=1,
        )
    except (OSError, subprocess.SubprocessError):
        return False
    return bool(result.stdout.strip())


# Returns the background color of the current line in the code editor
def _get_currentline_background_color():
    if IS_MAC:
        if IS_DARK_MODE:
            return QColor(0, 52, 110)
    return QColor(247, 236, 248)


if IS_MAC := mtd_env.is_mac():
    IS_DARK_MODE = _is_theme_dark()
else:
    IS_DARK_MODE = False

settings = QSettings()
settings.setValue("OS/IS_MAC", IS_MAC)
settings.setValue("OS/IS_DARK_MODE", IS_DARK_MODE)

# The current line background color to be used in the code editor
CURRENTLINE_BKGD_COLOR = _get_currentline_background_color()
