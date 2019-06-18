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


class PlotConfigDialogView(QDialog):

    def __init__(self, parent=None):
        super(PlotConfigDialogView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'plot_config_dialog.ui', baseinstance=self)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def add_tab_widget(self, tab_widget):
        self.main_tab_widget.addTab(*tab_widget)
