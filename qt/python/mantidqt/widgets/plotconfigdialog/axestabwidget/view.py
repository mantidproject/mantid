# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtCore import Qt
from qtpy.QtGui import QDoubleValidator
from qtpy.QtWidgets import QWidget, QMessageBox

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties


class AxesTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(AxesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'axes_tab_widget.ui',
                          baseinstance=self)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # Set validator for the axis limit spin boxes
        for limit in ['upper', 'lower']:
            line_edit = getattr(self, f'{limit}_limit_line_edit')
            validator = QDoubleValidator()
            line_edit.setValidator(validator)

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

    def get_lower_limit(self):
        return float(self.lower_limit_line_edit.text())

    def get_upper_limit(self):
        return float(self.upper_limit_line_edit.text())

    def get_label(self):
        return self.label_line_edit.text()

    def get_scale(self):
        return self.scale_combo_box.currentText()

    def set_lower_limit(self, limit):
        self.lower_limit_line_edit.setText(str(limit))

    def set_upper_limit(self, limit):
        self.upper_limit_line_edit.setText(str(limit))

    def set_label(self, label):
        self.label_line_edit.setText(label)

    def set_scale(self, scale):
        self.scale_combo_box.setCurrentText(scale.title())

    def get_axis(self):
        return self.axis_button_group.checkedButton().text()
