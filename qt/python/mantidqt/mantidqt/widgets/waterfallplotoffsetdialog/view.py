# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog

from mantidqt.utils.qt import load_ui


class WaterfallPlotOffsetDialogView(QDialog):

    def __init__(self, parent=None):
        super(WaterfallPlotOffsetDialogView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'waterfall_plot_offset_dialog.ui', baseinstance=self)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setModal(True)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def get_x_offset(self):
        return self.x_offset_spin_box.value()

    def get_y_offset(self):
        return self.y_offset_spin_box.value()

    def set_x_offset(self, offset):
        self.x_offset_spin_box.setValue(offset)

    def set_y_offset(self, offset):
        self.y_offset_spin_box.setValue(offset)
