# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import numpy as np
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
                spin_box.setRange(np.finfo(np.float32).min,
                                  np.finfo(np.float32).max)

    def populate_select_axes_combo_box(self, axes_names):
        self.select_axes_combo_box.addItems(axes_names)

    def set_current_axes_selector_text(self, new_text):
        current_index = self.select_axes_combo_box.currentIndex()
        self.select_axes_combo_box.setItemText(current_index, new_text)

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

    def get_selected_ax_name(self):
        return self.select_axes_combo_box.currentText()

    def set_properties(self, ax_props):
        self.set_title(ax_props.title)
        self.set_xlower_limit(ax_props.xlim[0])
        self.set_xupper_limit(ax_props.xlim[1])
        self.set_xlabel(ax_props.xlabel)
        self.set_xscale(ax_props.xscale)
        self.set_ylower_limit(ax_props.ylim[0])
        self.set_yupper_limit(ax_props.ylim[1])
        self.set_ylabel(ax_props.ylabel)
        self.set_yscale(ax_props.yscale)

    def get_properties(self):
        return AxProperties.from_view(self)
