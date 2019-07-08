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


class LineTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(LineTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab_line_tab.ui',
                          baseinstance=self)
        self.color_selector_widget = ColorSelector(parent=self)
        self.grid_layout.replaceWidget(self.color_selector_dummy_widget,
                                       self.color_selector_widget)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def get_style(self):
        return self.line_style_combo_box.currentText()

    def set_style(self, line_style):
        self.line_style_combo_box.setCurrentText(line_style)

    def get_draw_style(self):
        return self.draw_style_combo_box.currentText()

    def set_draw_style(self, draw_style):
        self.draw_style_combo_box.setCurrentText(draw_style)

    def get_width(self):
        return self.line_width_spin_box.value()

    def set_width(self, width):
        self.line_width_spin_box.setValue(width)

    def get_color(self):
        return self.color_selector_widget.get_color()

    def set_color(self, color_hex):
        self.color_selector_widget.set_color(color_hex)

    def update_fields(self, curve_props):
        self.set_style(curve_props.linestyle)
        self.set_draw_style(curve_props.drawstyle)
        self.set_width(curve_props.linewidth)
        self.set_color(curve_props.color)
