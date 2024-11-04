# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtCore import Qt, Signal
from qtpy.QtWidgets import QWidget, QAbstractItemView

from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget.view import ErrorbarsTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget.view import LineTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MarkerTabWidgetView
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.utils.qt import load_ui, block_signals


class CurvesTabWidgetView(QWidget):
    delete_key_pressed = Signal()

    def __init__(self, parent=None):
        super(CurvesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, "curves_tab.ui", baseinstance=self)
        self.line = LineTabWidgetView(parent=self)
        self.tab_container.addTab(self.line, "Line")
        self.marker = MarkerTabWidgetView(parent=self)
        self.tab_container.addTab(self.marker, "Marker")
        self.errorbars = ErrorbarsTabWidgetView(parent=self)
        self.tab_container.addTab(self.errorbars, "Errorbars")
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.select_curve_list.setSelectionMode(QAbstractItemView.ExtendedSelection)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Delete:
            self.delete_key_pressed.emit()

    def populate_select_axes_combo_box(self, axes_names):
        with block_signals(self.select_axes_combo_box):
            self.select_axes_combo_box.clear()
            self.select_axes_combo_box.addItems(axes_names)

    def populate_select_curve_list(self, curve_names):
        with block_signals(self.select_curve_list):
            self.select_curve_list.clear()
            self.select_curve_list.addItems(curve_names)
            self.select_curve_list.setCurrentRow(0)

    def set_selected_curve_selector_text(self, new_text):
        current_item = self.select_curve_list.currentItem()
        current_item.setText(new_text)

    def remove_select_axes_combo_box_selected_item(self):
        current_index = self.select_axes_combo_box.currentIndex()
        self.select_axes_combo_box.removeItem(current_index)

    def remove_select_curve_list_current_item(self):
        current_item = self.select_curve_list.currentItem()
        self.select_curve_list.removeItemWidget(current_item)

    def remove_select_curve_list_selected_items(self):
        selection = self.select_curve_list.selectionModel().selectedIndexes()
        selection.sort(reverse=True)
        for index in selection:
            _ = self.select_curve_list.takeItem(index.row())

    # Tab enablers and disablers
    def set_errorbars_tab_enabled(self, enable):
        self.tab_container.setTabEnabled(2, enable)

    # Top level entries
    def get_selected_ax_name(self):
        return self.select_axes_combo_box.currentText()

    def get_current_curve_name(self):
        return self.select_curve_list.currentItem().text()

    def get_selected_curves_names(self):
        return [item.text() for item in self.select_curve_list.selectedItems()]

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

    def enable_curve_config(self, enable=True):
        self.hide_curve_check_box.setEnabled(enable)
        self.curve_label_line_edit.setEnabled(enable)
        tab_count = self.tab_container.count()
        for i in range(0, tab_count):
            self.tab_container.setTabEnabled(i, enable)
