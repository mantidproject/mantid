# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import PyQt4.QtGui as QtGui


def warning(error, parent=None):
    if not parent:
        parent = QtGui.QWidget()
    QtGui.QMessageBox.warning(parent, "Error", str(error))
