# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets
from qtpy.QtGui import QPalette, QColor
from qtpy import QtCore

orange = QColor(255, 165, 0)
FIT_STATUSES = {"No fit": QtCore.Qt.black,
                "Success": QtCore.Qt.green,
                "Changes too small": orange,
                "Failed": QtCore.Qt.red}


class FitQualityDelegate(QtWidgets.QStyledItemDelegate):

    def __init__(self, parent):
        super(FitQualityDelegate, self).__init__(parent)

    def paint(self, painter, options, index):
        newOptions = QtWidgets.QStyleOptionViewItem(options)
        textColor = self.get_fit_status_color(index.data())
        newOptions.palette.setColor(QPalette.Text, textColor)
        super(FitQualityDelegate, self).paint(painter, newOptions, index)

    def get_fit_status_color(self, status):
        try:
            return FIT_STATUSES[status]
        except KeyError:
            return QtCore.Qt.black
