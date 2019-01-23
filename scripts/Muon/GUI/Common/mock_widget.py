# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import unicode_literals

import PyQt4.QtGui as QtGui

_QAPP = None


def mockQapp():
    """
    Constructs a QApplication object if one does not already exist
    :return: The QApplication object
    """
    global _QAPP
    if _QAPP is None:
        _QAPP = QtGui.QApplication([''])

    return _QAPP
