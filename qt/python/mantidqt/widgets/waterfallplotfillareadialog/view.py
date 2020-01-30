# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.colorselector import ColorSelector


class WaterfallPlotFillAreaDialogView(QDialog):

    def __init__(self, parent=None):
        super(WaterfallPlotFillAreaDialogView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'waterfall_plot_fill_area_dialog.ui', baseinstance=self)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setModal(True)
        self.colour_selector_widget = ColorSelector(parent=self)
        self.vertical_layout_2.replaceWidget(self.colour_selector_dummy_widget, self.colour_selector_widget)
        self.setAttribute(Qt.WA_DeleteOnClose, True)
