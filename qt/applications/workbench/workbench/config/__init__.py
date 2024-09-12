# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Main configuration module.

A singleton instance called CONF is defined. Modules wishing to access the settings
should import the CONF object as

    from workbench.config import CONF

and use it to access the settings
"""

# std imports
from collections import namedtuple
import os
import sys

# third-party imports
from qtpy.QtCore import Qt, QSettings

# local imports
from .user import UserConfig

# Type to hold properties for additional QMainWindow instances
WindowConfig = namedtuple("WindowConfig", ("parent", "flags"))

# -----------------------------------------------------------------------------
# "Private" Constants
# -----------------------------------------------------------------------------

# Default parent for additional QMainWindow instances
_ADDITIONAL_MAINWINDOWS_PARENT = None

# -----------------------------------------------------------------------------
# Public Constants
# -----------------------------------------------------------------------------
# The strings APPNAME, ORG_DOMAIN, ORGANIZATION are duplicated
# in mantidqt/dialogs/errorreports/main.py

ORGANIZATION = "mantidproject"
ORG_DOMAIN = "mantidproject.org"
APPNAME = "mantidworkbench"

DEFAULT_SCRIPT_CONTENT = ""
DEFAULT_SCRIPT_CONTENT += (
    "# import mantid algorithms, numpy and matplotlib"
    + os.linesep
    + "from mantid.simpleapi import *"
    + os.linesep
    + "import matplotlib.pyplot as plt"
    + os.linesep
    + "import numpy as np"
    + os.linesep
    + os.linesep
)

# Flags defining a standard Window
WINDOW_STANDARD_FLAGS = Qt.WindowFlags(Qt.Window)

# Flags defining our meaning of keeping figure windows on top.
# On Windows the standard Qt.Window flags + setting a parent keeps the widget on top
# Other OSs use the Qt.Tool type with a close button
if sys.platform == "win32":
    WINDOW_ONTOP_FLAGS = WINDOW_STANDARD_FLAGS
elif sys.platform == "darwin":
    WINDOW_ONTOP_FLAGS = Qt.Tool | Qt.CustomizeWindowHint | Qt.WindowCloseButtonHint | Qt.WindowMinimizeButtonHint
else:
    WINDOW_ONTOP_FLAGS = Qt.Tool | Qt.CustomizeWindowHint | Qt.WindowCloseButtonHint | Qt.WindowMinimizeButtonHint

# Iterable containing defaults for each configurable section of the code
# General application settings are in the main section
DEFAULTS = {
    "MainWindow": {
        "size": (1260, 740),
        "position": (10, 10),
    },
    "AdditionalWindows": {"behaviour": "On top"},
    "project": {
        "prompt_save_on_close": True,
        "prompt_save_editor_modified": True,
        "prompt_on_deleting_workspace": False,
        "save_altered_workspaces_only": False,
    },
    "Editors": {
        "completion_enabled": True,
    },
}

# State encodes widget layout (among other things).
# Increment this when the state of the next version is incompatible with the previous.
SAVE_STATE_VERSION = 2

# 'Singleton' instance
QSettings.setDefaultFormat(QSettings.IniFormat)
CONF = UserConfig(ORGANIZATION, APPNAME, defaults=DEFAULTS)


# Configuration for additional MainWindow instances: matplotlib figures, custom user interfaces etc
def get_window_config():
    """
    :return: A WindowConfig object describing the desired window configuration based on the current settings
    """
    try:
        windows_behaviour = CONF.get("AdditionalWindows", "behaviour", type=str)
        windows_on_top = True if windows_behaviour == "On top" else False
    except KeyError:
        windows_on_top = False
    if windows_on_top:
        parent = _ADDITIONAL_MAINWINDOWS_PARENT
        flags = WINDOW_ONTOP_FLAGS
    else:
        parent = None
        flags = WINDOW_STANDARD_FLAGS

    return WindowConfig(parent, flags)


def set_additional_windows_parent(widget):
    """
    Sets the parent for any new MainWindow instances created and updates the existing QMainWindow instances
    :param widget: A QWidget that will act as a parent for a each new window
    """
    global _ADDITIONAL_MAINWINDOWS_PARENT
    _ADDITIONAL_MAINWINDOWS_PARENT = widget
