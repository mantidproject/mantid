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


class OpenManageUserDirectories():
    currently_open_mud = None

    @classmethod
    def open_mud(cls, parent):
        # Checks whether a Manage User Directories window is already open before opening another one.
        if cls.currently_open_mud:
            # Brings the already open window to the front.
            cls.currently_open_mud.raise_()
        else:
            cls.currently_open_mud = ManageUserDirectories(parent)
            cls.currently_open_mud.show()
            cls.currently_open_mud.destroyed.connect(cls.mud_closed)

    @classmethod
    def mud_closed(cls):
        cls.currently_open_mud = None
