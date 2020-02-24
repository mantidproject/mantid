# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from qtpy import QtWidgets


def warning(error, parent=None):
    if not parent:
        parent = QtWidgets.QWidget()
    QtWidgets.QMessageBox.warning(parent, "Error", str(error))


def question(question, parent=None):
    qm = QtWidgets.QMessageBox
    ret = qm.question(parent, '', question, qm.Yes | qm.No)

    if ret == qm.Yes:
        return True
    else:
        return False
