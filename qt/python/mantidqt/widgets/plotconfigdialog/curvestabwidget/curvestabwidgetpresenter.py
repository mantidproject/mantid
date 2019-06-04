# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog import get_axes_names_dict
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.curvestabwidgetview import CurvesTabWidgetView


class BlockQSignals:

    def __init__(self, qobject):
        self.qobject = qobject

    def __enter__(self):
        self.qobject.blockSignals(True)

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.qobject.blockSignals(False)


class CurvesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if not view:
            self.view = CurvesTabWidgetView(parent)
        else:
            self.view = view

        self.axes_names_dict = get_axes_names_dict(self.fig)
        self.populate_select_axes_combo_box()
        self.populate_select_curve_combo_box()
        self.set_selected_curve_view_properties()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.populate_curve_combo_box_and_update_properties
        )
        self.view.select_curve_combo_box.currentIndexChanged.connect(
            self.set_selected_curve_view_properties
        )
        self.view.remove_curve_button.clicked.connect(
            self.remove_selected_curve
        )

    def apply_properties(self):
        props = self.view.get_properties()
        line = self.get_selected_curve()

        line.set_visible(not props.hide_curve)
        self.set_curve_label(line, props.label)

    def set_curve_label(self, line, label):
        line.set_label(label)
        self.view.set_current_curve_selector_text(label)

    def remove_selected_curve(self):
        self.get_selected_curve().remove()
        ax = self.get_selected_ax()
        ax.figure.canvas.draw()
        with BlockQSignals(self.view.select_curve_combo_box):
            self.view.remove_select_curve_combo_box_current_item()
            if self.view.select_curve_combo_box.count() == 0:
                self.view.remove_select_axes_combo_box_current_item()

    def clear_select_curves_combo_box_signal_blocking(self):
        self.view.select_curve_combo_box.blockSignals(True)
        self.view.select_curve_combo_box.clear()
        self.view.select_curve_combo_box.blockSignals(False)

    def get_selected_ax(self):
        return self.axes_names_dict[self.view.get_selected_ax_name()]

    def get_selected_curve(self):
        for curve in self.get_selected_ax().get_lines():
            if curve.get_label() == self.view.get_selected_curve_name():
                return curve

    def get_selected_curve_properties(self):
        return CurveProperties.from_curve(self.get_selected_curve())

    def populate_select_axes_combo_box(self):
        # Sort names by axes position
        names = sorted(self.axes_names_dict.keys(),
                       key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def populate_select_curve_combo_box(self):
        with BlockQSignals(self.view.select_curve_combo_box):
            self.clear_select_curves_combo_box_signal_blocking()
        curve_names = []
        lines = self.get_selected_ax().get_lines()
        for line in lines:
            curve_names.append(line.get_label())
        self.view.populate_select_curve_combo_box(curve_names)

    def populate_curve_combo_box_and_update_properties(self):
        self.populate_select_curve_combo_box()
        self.set_selected_curve_view_properties()

    def set_selected_curve_view_properties(self):
        self.view.set_properties(self.get_selected_curve_properties())
