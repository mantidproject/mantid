# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

from mantidqt.utils.qt import import_qt


ManageUserDirectories = import_qt('.._common', 'mantidqt.widgets', 'ManageUserDirectories')

manage_user_directories = None


def open_mud(parent):
    # Checks whether a Manage User Directories window is already open before opening another one.
    global manage_user_directories
    if manage_user_directories is None:
        manage_user_directories = ManageUserDirectories(parent)
        manage_user_directories.show()
        manage_user_directories.destroyed.connect(mud_closed)
    else:
        # Brings the already open window to the front.
        manage_user_directories.raise_()


def mud_closed():
    global manage_user_directories
    manage_user_directories = None
