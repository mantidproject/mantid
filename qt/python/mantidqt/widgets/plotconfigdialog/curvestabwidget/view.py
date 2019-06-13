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

from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget.view import ErrorbarsTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget.view import LineTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MarkerTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.utils.qt import load_ui


class CurvesTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(CurvesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab.ui',
                          baseinstance=self)
        self.line = LineTabWidgetView(parent=self)
        self.tab_container.addTab(self.line, "Line")
        self.marker = MarkerTabWidgetView(parent=self)
        self.tab_container.addTab(self.marker, "Marker")
        self.errorbars = ErrorbarsTabWidgetView(parent=self)
        self.tab_container.addTab(self.errorbars, "Errorbars")
        self.setAttribute(Qt.WA_DeleteOnClose, True)

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
    def set_errorbars_tab_enabled(self, enable):
        self.tab_container.setTabEnabled(2, enable)

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
        if state:
            self.hide_curve_check_box.setCheckState(Qt.Checked)
        else:
            self.hide_curve_check_box.setCheckState(Qt.Unchecked)

    # Property object getters and setters
    def get_properties(self):
        return CurveProperties.from_view(self)

    def update_fields(self, curve_props):
        self.set_curve_label(curve_props.label)
        self.set_hide_curve(curve_props.hide)
        self.line.update_fields(curve_props)
        self.marker.update_fields(curve_props)
        self.errorbars.update_fields(curve_props)
