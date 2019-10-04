# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt, QSize
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties


class AdvancedLegendOptionsView(QDialog):

    def __init__(self, parent):
        super(AdvancedLegendOptionsView, self).__init__(parent=parent)
        self.ui = load_ui(__file__, 'advanced_legend_options.ui', baseinstance=self)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setFixedSize(QSize(350,350))
        self.setWindowFlags(Qt.Dialog | Qt.MSWindowsFixedSizeDialogHint)

    def get_shadow(self):
        return self.shadow_check_box.isChecked()

    def get_round_edges(self):
        return self.round_edges_check_box.isChecked()

    def get_number_of_columns(self):
        return self.number_of_columns_spin_box.value()

    def get_column_spacing(self):
        return self.column_spacing_spin_box.value()

    def get_label_spacing(self):
        return self.label_spacing_spin_box.value()

    def get_marker_position(self):
        return self.marker_position_combo_box.currentText()

    def get_number_of_markers(self):
        return self.number_of_markers_spin_box.value()

    def get_border_padding(self):
        return self.border_padding_spin_box.value()

    def set_shadow(self, shadow):
        self.shadow_check_box.setChecked(shadow)

    def set_round_edges(self, round_edges):
        self.round_edges_check_box.setChecked(round_edges)

    def set_number_of_columns(self, columns):
        self.number_of_columns_spin_box.setValue(columns)

    def set_column_spacing(self, column_spacing):
        self.column_spacing_spin_box.setValue(column_spacing)

    def set_label_spacing(self, label_spacing):
        self.label_spacing_spin_box.setValue(label_spacing)

    def set_marker_position(self, position):
        self.marker_position_combo_box.setCurrentText(position)

    def set_number_of_markers(self, markers):
        self.number_of_markers_spin_box.setValue(markers)

    def set_border_padding(self, padding):
        self.border_padding_spin_box.setValue(padding)

    def get_marker_label_padding(self):
        return self.marker_label_padding_spin_box.value()

    def set_marker_label_padding(self, padding):
        self.marker_label_padding_spin_box.setValue(padding)

    def get_properties(self):
        return LegendProperties.from_view_advanced(self)
