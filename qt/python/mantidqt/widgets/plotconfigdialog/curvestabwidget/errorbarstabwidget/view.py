# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget

from mantidqt.widgets.plotconfigdialog.colorselector import ColorSelector
from mantidqt.utils.qt import load_ui


class ErrorbarsTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(ErrorbarsTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab_errorbars_tab.ui',
                          baseinstance=self)
        self.color_selector_widget = ColorSelector(parent=self)
        self.layout.replaceWidget(self.color_dummy_widget,
                                  self.color_selector_widget)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def get_hide(self):
        return self.hide_errorbars_tickbox.checkState() == Qt.Checked

    def set_hide(self, state):
        self.hide_errorbars_tickbox.setCheckState(state)

    def get_width(self):
        return self.width_spin_box.value()

    def set_width(self, width):
        self.width_spin_box.setValue(width)

    def get_capsize(self):
        return self.capsize_spin_box.value()

    def set_capsize(self, size):
        self.capsize_spin_box.setValue(size)

    def get_cap_thickness(self):
        return self.cap_thickness_spin_box.value()

    def set_cap_thickness(self, thickness):
        self.cap_thickness_spin_box.setValue(thickness)

    def get_error_every(self):
        return self.error_every_spin_box.value()

    def set_error_every(self, error_every):
        self.error_every_spin_box.setValue(error_every)

    def get_color(self):
        return self.color_selector_widget.get_color()

    def set_color(self, color):
        self.color_selector_widget.set_color(color)
