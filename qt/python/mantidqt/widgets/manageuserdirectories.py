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


ManageUserDirectories_cpp = import_qt('.._common', 'mantidqt.widgets', 'ManageUserDirectories')


class ManageUserDirectories(ManageUserDirectories_cpp):
    """
    Small wrapper class around the Manage User Directories Window that
    hides the help button which can lead to a crash.
    See https://github.com/mantidproject/mantid/issues/26404.

    This is a safe, temporary fix, for release 4.0.1.
    """

    def __init__(self, parent=None):
        super(ManageUserDirectories, self).__init__(parent)
        self.setHelpButtonVisible(False)
