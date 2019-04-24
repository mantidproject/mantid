# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#

from __future__ import (absolute_import)

from mantidqt.utils.qt import import_qt

_getIcon = import_qt('._icons', 'mantidqt', 'getIcon')


def get_icon(*names, **options):
    return _getIcon(*names, **options)
