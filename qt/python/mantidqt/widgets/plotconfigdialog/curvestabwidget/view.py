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

from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget.view import ErrorbarsTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget.view import LineTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MarkerTabWidgetView
from mantidqt.utils.qt import load_ui


class CurvesTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(CurvesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab.ui',
                          baseinstance=self)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.line = LineTabWidgetView(self)
        self.tab_container.addTab(self.line, "Line")
        self.marker = MarkerTabWidgetView(self)
        self.tab_container.addTab(self.marker, "Marker")
        self.errorbars = ErrorbarsTabWidgetView(self)
        self.tab_container.addTab(self.errorbars, "Errorbars")

    def populate_select_axes_combo_box(self, axes_names):
        self.select_axes_combo_box.addItems(axes_names)

    def populate_select_curve_combo_box(self, curve_names):
        self.select_curve_combo_box.addItems(curve_names)

    def set_selected_curve_selector_text(self, new_text):
        current_index = self.select_curve_combo_box.currentIndex()
        self.select_curve_combo_box.setItemText(current_index, new_text)

    def remove_select_axes_combo_box_selected_item(self):
        current_index = self.select_axes_combo_box.currentIndex()
        self.select_axes_combo_box.removeItem(current_index)

    def remove_select_curve_combo_box_selected_item(self):
        current_index = self.select_curve_combo_box.currentIndex()
        self.select_curve_combo_box.removeItem(current_index)

    # Tab enablers and disablers
    def enable_errorbars_tab(self):
        self.tab_container.setTabEnabled(2, True)

    def disable_errorbars_tab(self):
        self.tab_container.setTabEnabled(2, False)

    def enable_line_tab(self):
        self.tab_container.setTabEnabled(0, True)

    def disable_line_tab(self):
        self.tab_container.setTabEnabled(0, False)

    def enable_marker_tab(self):
        self.tab_container.setTabEnabled(1, True)

    def disable_marker_tab(self):
        self.tab_container.setTabEnabled(1, False)

    # Top level entries
    def get_selected_ax_name(self):
        return self.select_axes_combo_box.currentText()

    def get_selected_curve_name(self):
        return self.select_curve_combo_box.currentText()

    def set_curve_label(self, label):
        self.curve_label_line_edit.setText(label)

    def get_curve_label(self):
        return self.curve_label_line_edit.text()

    def get_hide_curve(self):
        return self.hide_curve_check_box.checkState()

    def set_hide_curve(self, state):
        self.hide_curve_check_box.setCheckState(state)

    # Property object getters and setters
    def get_properties(self):
        return CurveProperties.from_view(self)

    def set_properties(self, curve_props):
        """Set all fields in the view from CurveProperties object"""
        self.set_curve_label(curve_props.label)
        self.set_hide_curve(curve_props.hide_curve)
        if curve_props.line:
            self.line.set_style(curve_props.line.style)
            self.line.set_draw_style(curve_props.line.draw_style)
            self.line.set_width(curve_props.line.width)
            self.line.set_color(curve_props.line.color)
        if curve_props.marker:
            self.marker.set_style(curve_props.marker.style)
            self.marker.set_size(curve_props.marker.size)
            self.marker.face_color_selector_widget.set_color(curve_props.marker.face_color)
            self.marker.edge_color_selector_widget.set_color(curve_props.marker.edge_color)
        # Errorbar properties
        if curve_props.errorbars:
            self.errorbars.set_hide(curve_props.errorbars.hide)
            self.errorbars.set_width(curve_props.errorbars.width)
            self.errorbars.set_capsize(curve_props.errorbars.capsize)
            self.errorbars.set_cap_thickness(curve_props.errorbars.cap_thickness)
            self.errorbars.set_error_every(curve_props.errorbars.error_every)
            self.errorbars.set_color(curve_props.errorbars.color)
