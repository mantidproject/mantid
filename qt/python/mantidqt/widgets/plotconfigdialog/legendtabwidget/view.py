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

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.colorselector import ColorSelector
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties
from mantidqt.widgets.plotconfigdialog.legendtabwidget.advancedlegendoptionsdialog.view import AdvancedLegendOptionsView


class LegendTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(LegendTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'legend_tab.ui',
                          baseinstance=self)
        self.background_color_selector_widget = ColorSelector(parent=self)
        self.edge_color_selector_widget = ColorSelector(parent=self)
        self.title_color_selector_widget = ColorSelector(parent=self)
        self.entries_color_selector_widget = ColorSelector(parent=self)
        self.grid_layout.replaceWidget(self.background_color_selector_dummy_widget,
                                       self.background_color_selector_widget)
        self.grid_layout.replaceWidget(self.edge_color_selector_dummy_widget,
                                       self.edge_color_selector_widget)
        self.grid_layout.replaceWidget(self.entries_color_selector_dummy_widget,
                                       self.entries_color_selector_widget)
        self.grid_layout.replaceWidget(self.title_color_selector_dummy_widget,
                                       self.title_color_selector_widget)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.advanced_options = AdvancedLegendOptionsView(self)

    def set_transparency_slider(self, transparency):
        self.transparency_slider.setValue(transparency)

    def get_transparency_slider_value(self):
        return self.transparency_slider.value()

    def set_transparency_spin_box(self, transparency):
        self.transparency_spin_box.setValue(transparency)

    def get_transparency_spin_box_value(self):
        return self.transparency_spin_box.value()

    def set_title(self, title):
        self.title_line_edit.setText(title)

    def get_title(self):
        return self.title_line_edit.text()

    def set_background_color(self, color):
        self.background_color_selector_widget.set_color(color)

    def get_background_color(self):
        return self.background_color_selector_widget.get_color()

    def set_edge_color(self, color):
        self.edge_color_selector_widget.set_color(color)

    def get_edge_color(self):
        return self.edge_color_selector_widget.get_color()

    def set_entries_font(self, font):
        self.entries_font_combo_box.setCurrentText(font)

    def get_entries_font(self):
        return self.entries_font_combo_box.currentText()

    def set_entries_size(self, size):
        self.entries_size_spin_box.setValue(size)

    def get_entries_size(self):
        return self.entries_size_spin_box.value()

    def set_entries_color(self, color):
        self.entries_color_selector_widget.set_color(color)

    def get_entries_color(self):
        return self.entries_color_selector_widget.get_color()

    def set_title_font(self, font):
        self.title_font_combo_box.setCurrentText(font)

    def get_title_font(self):
        return self.title_font_combo_box.currentText()

    def set_title_size(self, size):
        self.title_size_spin_box.setValue(size)

    def get_title_size(self):
        return self.title_size_spin_box.value()

    def set_title_color(self, color):
        self.title_color_selector_widget.set_color(color)

    def get_title_color(self):
        return self.title_color_selector_widget.get_color()

    def set_marker_size(self, size):
        self.marker_size_spin_box.setValue(size)

    def get_marker_size(self):
        return self.marker_size_spin_box.value()

    def get_hide_box(self):
        return self.hide_box_check_box.isChecked()

    def set_hide_box(self, hide):
        self.hide_box_check_box.setChecked(hide)

    def get_hide_legend(self):
        return self.hide_legend_check_box.isChecked()

    def set_hide_legend(self, hide):
        self.hide_legend_check_box.setChecked(hide)

    def get_properties(self):
        props = LegendProperties.from_view(self)
        advanced_props = self.advanced_options.get_properties()
        props.update(advanced_props)
        return props
