# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog import get_axes_names_dict
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties, MARKER_MAP
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
        """Update the curve's properties from values in the view"""
        props = self.view.get_properties()
        curve = self.get_selected_curve()

        # Top level properties
        curve.set_visible(not props.hide_curve)
        self.set_curve_label(curve, props.label)

        # Line properties
        curve.set_linestyle(props.line_style)
        curve.set_drawstyle(props.draw_style)
        curve.set_linewidth(props.line_width)
        curve.set_color(props.line_color)

        # Marker properties
        curve.set_marker(MARKER_MAP[props.marker_style])
        curve.set_markersize(props.marker_size)
        curve.set_markerfacecolor(props.marker_face_color)
        curve.set_markeredgecolor(props.marker_edge_color)

    def set_curve_label(self, line, label):
        """Set label on curve and update its entry in the combo box"""
        line.set_label(label)
        self.view.set_current_curve_selector_text(label)

    def remove_selected_curve(self):
        """
        Remove selected curve from figure and combobox. If there are no
        curves left on the axes remove that axes from axes the combo box
        """
        self.get_selected_curve().remove()
        ax = self.get_selected_ax()
        ax.figure.canvas.draw()
        with BlockQSignals(self.view.select_curve_combo_box):
            self.view.remove_select_curve_combo_box_current_item()
            if self.view.select_curve_combo_box.count() == 0:
                self.view.remove_select_axes_combo_box_current_item()

    def get_selected_ax(self):
        """
        Get selected axes object from name in combo box.
        If not found return None.
        """
        try:
            return self.axes_names_dict[self.view.get_selected_ax_name()]
        except KeyError:
            return None

    def get_selected_curve(self):
        """Get selected Line2D object"""
        for curve in self.get_selected_ax().get_lines():
            if curve.get_label() == self.view.get_selected_curve_name():
                return curve

    def get_selected_curve_properties(self):
        """Get CurveProperties object from current Line2D object"""
        return CurveProperties.from_curve(self.get_selected_curve())

    def populate_select_axes_combo_box(self):
        """
        Add Axes names to select axes combo box.
        Names are generated similary to in AxesTabWidgetPresenter
        """
        # Sort names by axes position
        names = sorted(self.axes_names_dict.keys(),
                       key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def populate_select_curve_combo_box(self):
        """
        Add curves on selected axes to the select curves combo box.
        Return False if there are no lines on the axes (this can occur
        when a user uses the "Remove Curve" button), else return True.
        """
        with BlockQSignals(self.view.select_curve_combo_box):
            self.view.select_curve_combo_box.clear()
        curve_names = []
        selected_ax = self.get_selected_ax()
        if not selected_ax:
            self.view.close()
            return False
        lines = self.get_selected_ax().get_lines()
        for line in lines:
            curve_names.append(line.get_label())
        self.view.populate_select_curve_combo_box(curve_names)
        return True

    def populate_curve_combo_box_and_update_properties(self):
        """
        Populate curve combo box and update the view with the curve's
        properties.
        """
        if self.populate_select_curve_combo_box():
            self.set_selected_curve_view_properties()

    def set_selected_curve_view_properties(self):
        """Update the view's fields with the selected curve's properties"""
        self.view.set_properties(self.get_selected_curve_properties())
