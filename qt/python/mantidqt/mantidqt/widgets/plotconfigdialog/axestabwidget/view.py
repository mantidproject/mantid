# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget, QTabBar

from mantidqt.utils.qt import load_ui
from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.colorselector import ColorSelector


class AxesTabWidgetView(QWidget):
    def __init__(self, parent=None):
        super(AxesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, "axes_tab_widget.ui", baseinstance=self)
        self.color_selector_widget = ColorSelector(parent=self)
        self.color_selector_layout.replaceWidget(self.color_selector_dummy_widget, self.color_selector_widget)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # QTabBar cannot be created in QTDesigner
        # QTabWidget not suitable because we reuse controls for each axis
        self.axis_tab_bar = QTabBar(parent=self)
        self.x_tab = self.axis_tab_bar.addTab("x")
        self.y_tab = self.axis_tab_bar.addTab("y")
        self.z_tab = self.axis_tab_bar.addTab("z")
        self.axis_tab_bar_layout.replaceWidget(self.dummy_axis_tab_bar, self.axis_tab_bar)

        self.lower_limit_validator = LineEditDoubleValidator(self.lower_limit_line_edit, 0.0)
        self.upper_limit_validator = LineEditDoubleValidator(self.upper_limit_line_edit, 1.0)
        self.lower_limit_line_edit.setValidator(self.lower_limit_validator)
        self.upper_limit_line_edit.setValidator(self.upper_limit_validator)

    def populate_select_axes_combo_box(self, axes_names):
        self.select_axes_combo_box.addItems(axes_names)

    def set_selected_axes_selector_text(self, new_text):
        """Replace the text of the selected item in the combo box"""
        current_index = self.select_axes_combo_box.currentIndex()
        self.select_axes_combo_box.setItemText(current_index, new_text)

    def get_selected_ax_name(self):
        return self.select_axes_combo_box.currentText()

    def get_properties(self):
        return AxProperties.from_view(self)

    def get_title(self):
        return self.axes_title_line_edit.text()

    def set_title(self, title):
        self.axes_title_line_edit.setText(title)

    def get_show_minor_ticks(self):
        return self.show_minor_ticks_check_box.isChecked()

    def set_show_minor_ticks(self, check):
        self.show_minor_ticks_check_box.setChecked(check)

    def get_show_minor_gridlines(self):
        return self.show_minor_gridlines_check_box.isChecked()

    def set_show_minor_gridlines(self, check):
        self.show_minor_gridlines_check_box.setChecked(check)

    def set_minor_grid_tick_controls_visible(self, visible):
        self.show_minor_gridlines_check_box.setVisible(visible)
        self.show_minor_ticks_check_box.setVisible(visible)

    def set_minor_gridlines_check_box_enabled(self, enabled):
        self.show_minor_gridlines_check_box.setEnabled(enabled)

    def get_lower_limit(self):
        if not self.lower_limit_line_edit.hasAcceptableInput():
            self.lower_limit_line_edit.validator().fixup(self.lower_limit_line_edit.text())
        return float(self.lower_limit_line_edit.text())

    def get_upper_limit(self):
        if not self.upper_limit_line_edit.hasAcceptableInput():
            self.upper_limit_line_edit.validator().fixup(self.upper_limit_line_edit.text())
        return float(self.upper_limit_line_edit.text())

    def get_label(self):
        return self.label_line_edit.text()

    def get_scale(self):
        return self.scale_combo_box.currentText()

    def get_canvas_color(self):
        return self.color_selector_widget.get_color()

    def get_autoscale_enabled(self):
        return self.autoscale.isChecked()

    def get_z_axis_selector_checked(self):
        return self.axis_tab_bar.currentIndex() == 2

    def set_lower_limit(self, limit):
        self.lower_limit_validator.last_valid_value = str(limit)
        self.lower_limit_line_edit.setText(str(limit))

    def set_upper_limit(self, limit):
        self.upper_limit_validator.last_valid_value = str(limit)
        self.upper_limit_line_edit.setText(str(limit))

    def set_label(self, label):
        self.label_line_edit.setText(label)

    def set_scale(self, scale):
        self.scale_combo_box.setCurrentText(scale.title())

    def set_canvas_color(self, color_hex):
        self.color_selector_widget.set_color(color_hex)

    def set_autoscale_enabled(self, enabled):
        self.autoscale.setChecked(enabled)

    def set_limit_input_enabled(self, enabled):
        self.lower_limit_line_edit.setEnabled(enabled)
        self.upper_limit_line_edit.setEnabled(enabled)

    def set_z_axis_selector_enabled(self, enabled):
        self.axis_tab_bar.setTabEnabled(2, enabled)

    def set_x_axis_selector_click(self):
        self.axis_tab_bar.setCurrentIndex(0)

    def set_scale_combo_box_enabled(self, eneabled):
        self.scale_combo_box.setEnabled(eneabled)

    def get_axis(self):
        return "xyz"[self.axis_tab_bar.currentIndex()]
