# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets


def getWidgetIfOpen(name):
    allWidgets = QtWidgets.QApplication.allWidgets()
    for widget in allWidgets:
        if widget.accessibleName() == name:
            return widget
    return None
