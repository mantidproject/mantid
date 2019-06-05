# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.axes import ErrorbarContainer

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
        self.view.show_errorbars_tab(self.get_selected_curve_errorbars())
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
        self.view.select_curve_combo_box.currentIndexChanged.connect(
            lambda: self.view.show_errorbars_tab(self.get_selected_curve_errorbars())
        )

    def apply_properties(self):
        """Update the curve's properties from values in the view"""
        props = self.view.get_properties()
        curve = self.get_selected_curve()

        if 'nolegend' not in curve.get_label():
            self.set_curve_label(curve, props.label)
        if isinstance(curve, ErrorbarContainer):
            self.apply_errorbar_properties(curve, props)
            curve = curve.lines[0]  # This is the line between errorbar points

        curve.set_visible(not props.hide_curve)

        self.apply_line_properties(curve, props)
        self.apply_marker_properties(curve, props)

    @staticmethod
    def apply_line_properties(line, props):
        line.set_linestyle(props.line_style)
        line.set_drawstyle(props.draw_style)
        line.set_linewidth(props.line_width)
        line.set_color(props.line_color)

    @staticmethod
    def apply_marker_properties(line, props):
        line.set_marker(MARKER_MAP[props.marker_style])
        line.set_markersize(props.marker_size)
        line.set_markerfacecolor(props.marker_face_color)
        line.set_markeredgecolor(props.marker_edge_color)

    def apply_errorbar_properties(self, container, props):
        caps_tuple = container.lines[1]
        bars_tuple = container.lines[2]
        for bars in bars_tuple:
            self.hide_errorbars(not props.hide_errorbars, caps_tuple, bars)
            bars.set_linewidth(props.errorbar_width)
            self.apply_error_cap_properties(caps_tuple, props)
            # self.set_error_every()
            bars.set_color(props.errorbar_color)

    @staticmethod
    def apply_error_cap_properties(caps_tuple, props):
        """Apply properties to errorbar caps"""
        for caps in caps_tuple:
            caps.set_markersize(2*props.errorbar_capsize)
            caps.set_markeredgewidth(props.errorbar_cap_thickness)
            caps.set_color(props.errorbar_color)

    @staticmethod
    def hide_errorbars(visible, caps_tuple, bars):
        """
        Show or hide errorbars
        :param visible: Bool. True to hide or False to show
        :param caps_tuple: Tuple containing top and bottom errorbar caps Line2D objects
        :param bars: LineCollection containing the errorbars' bars
        """
        bars.set_visible(visible)
        for caps in caps_tuple:
            caps.set_visible(visible)

    # Getters
    def get_selected_ax_errorbars(self):
        """Get all errobar containers in selected axes"""
        ax = self.get_selected_ax()
        return [cont for cont in ax.containers if isinstance(cont, ErrorbarContainer)]

    def get_selected_curve_errorbars(self):
        """
        Return errobar container if selected curve has one. Else return
        False
        """
        for errors in self.get_selected_ax_errorbars():
            if errors.get_label() == self.view.get_selected_curve_name():
                return errors
        return False

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
        """Get selected Line2D or ErrorbarContainer object"""
        for curve in self.get_selected_ax().get_lines():
            if curve.get_label() == self.view.get_selected_curve_name():
                return curve
        for errobar_container in self.get_selected_ax_errorbars():
            if errobar_container.get_label() == self.view.get_selected_curve_name():
                return errobar_container

    def get_selected_curve_properties(self):
        """
        Get CurveProperties object from current Line2D or ErrorbarContainer
        """
        return CurveProperties.from_curve(self.get_selected_curve())

    def populate_select_axes_combo_box(self):
        """
        Add Axes names to 'select axes' combo box.
        Names are generated similary to in AxesTabWidgetPresenter
        """
        # Sort names by axes position
        names = sorted(self.axes_names_dict.keys(),
                       key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def populate_select_curve_combo_box(self):
        """
        Add curves on selected axes to the 'select curves' combo box.
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
        for errorbar_container in self.get_selected_ax_errorbars():
            if 'nolegend' not in errorbar_container.get_label():
                curve_names.append(errorbar_container.get_label())
        for line in lines:
            if 'nolegend' not in line.get_label():
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

    def remove_selected_curve(self):
        """
        Remove selected curve from figure and combobox. If there are no
        curves left on the axes remove that axes from axes the combo box
        """
        self.get_selected_curve().remove()
        ax = self.get_selected_ax()
        ax.figure.canvas.draw()
        with BlockQSignals(self.view.select_curve_combo_box):
            self.view.remove_select_curve_combo_box_selected_item()
            if self.view.select_curve_combo_box.count() == 0:
                self.view.remove_select_axes_combo_box_selected_item()

    def set_curve_label(self, curve, label):
        """Set label on curve and update its entry in the combo box"""
        old_label = curve.get_label()
        self.update_legend_entry(curve, old_label, label)
        curve.set_label(label)
        self.view.set_selected_curve_selector_text(label)

    @staticmethod
    def update_legend_entry(curve, old_label, new_label):
        """Update the entry in the legend for a specific curve"""
        legend = CurvesTabWidgetPresenter.get_legend_from_curve(curve)
        text_labels = legend.get_texts()
        for text in text_labels:
            if text.get_text() == old_label:
                text.set_text(new_label)

    @staticmethod
    def get_legend_from_curve(curve):
        if isinstance(curve, ErrorbarContainer):
            return curve.get_children()[0].axes.get_legend()
        else:
            return curve.axes.get_legend()

    def set_selected_curve_view_properties(self):
        """Update the view's fields with the selected curve's properties"""
        self.view.set_properties(self.get_selected_curve_properties())
