# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from numpy import finfo, float32
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties


class AxesTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(AxesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'axes_tab_widget.ui',
                          baseinstance=self)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # Set maxima and minima for the axis limit spin boxes
        for axis in ['x', 'y']:
            for limit in ['upper', 'lower']:
                spin_box = getattr(self, '%s%s_limit_spin_box' % (axis, limit))
                spin_box.setRange(finfo(float32).min,
                                  finfo(float32).max)

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

    # X-Axis getters
    def get_xlower_limit(self):
        return self.xlower_limit_spin_box.value()

    def get_xupper_limit(self):
        return self.xupper_limit_spin_box.value()

    def get_xlabel(self):
        return self.xlabel_line_edit.text()

    def get_xscale(self):
        return self.xscale_combo_box.currentText()

    # Y-Axis getters
    def get_ylower_limit(self):
        return self.ylower_limit_spin_box.value()

    def get_yupper_limit(self):
        return self.yupper_limit_spin_box.value()

    def get_ylabel(self):
        return self.ylabel_line_edit.text()

    def get_yscale(self):
        return self.yscale_combo_box.currentText()

    # X-Axis setters
    def set_xlower_limit(self, limit):
        self.xlower_limit_spin_box.setValue(limit)

    def set_xupper_limit(self, limit):
        self.xupper_limit_spin_box.setValue(limit)

    def set_xlabel(self, label):
        self.xlabel_line_edit.setText(label)

    def set_xscale(self, scale):
        self.xscale_combo_box.setCurrentText(scale.title())

    # Y-Axis setters
    def set_ylower_limit(self, limit):
        self.ylower_limit_spin_box.setValue(limit)

    def set_yupper_limit(self, limit):
        self.yupper_limit_spin_box.setValue(limit)

    def set_ylabel(self, label):
        self.ylabel_line_edit.setText(label)

    def set_yscale(self, scale):
        self.yscale_combo_box.setCurrentText(scale.title())
