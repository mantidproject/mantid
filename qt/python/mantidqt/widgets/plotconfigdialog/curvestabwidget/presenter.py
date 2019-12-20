# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.utils.qt import block_signals
from mantidqt.widgets.plotconfigdialog import get_axes_names_dict, curve_in_ax
from mantidqt.widgets.plotconfigdialog.curvestabwidget import (
    CurveProperties, curve_has_errors, remove_curve_from_ax)
from mantidqt.widgets.plotconfigdialog.curvestabwidget.view import CurvesTabWidgetView
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties
from workbench.plotting.figureerrorsmanager import FigureErrorsManager


class CurvesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None, legend_tab=None):
        self.fig = fig

        # The legend tab is passed in so that it can be removed if all curves are removed.
        self.legend_tab = legend_tab
        self.legend_props = None

        if not view:
            self.view = CurvesTabWidgetView(parent)
        else:
            self.view = view

        self.current_view_properties = None

        # Fill the fields in the view
        self.axes_names_dict = get_axes_names_dict(self.fig, curves_only=True)
        self.populate_select_axes_combo_box()
        self.curve_names_dict = {}
        self.populate_curve_combo_box_and_update_view()

        self.set_apply_to_all_buttons_enabled()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.populate_curve_combo_box_and_update_view)
        self.view.select_curve_combo_box.currentIndexChanged.connect(
            self.update_view)
        self.view.remove_curve_button.clicked.connect(
            self.remove_selected_curve)
        self.view.line.apply_to_all_button.clicked.connect(
            self.line_apply_to_all)
        self.view.marker.apply_to_all_button.clicked.connect(
            self.marker_apply_to_all)
        self.view.errorbars.apply_to_all_button.clicked.connect(
            self.errorbars_apply_to_all)

    def apply_properties(self):
        """Take properties from views and set them on the selected curve"""
        ax = self.get_selected_ax()
        if ax.legend_:
            self.legend_props = LegendProperties.from_legend(ax.legend_)

        view_props = self.get_view_properties()
        if view_props == self.current_view_properties:
            return
        plot_kwargs = view_props.get_plot_kwargs()
        # Re-plot curve
        self._replot_selected_curve(plot_kwargs)
        curve = self.get_selected_curve()
        # Set the curve's new name in the names dict and combo box
        self.set_new_curve_name_in_dict_and_combo_box(curve, view_props.label)
        FigureErrorsManager.toggle_errors(curve, view_props)
        self.current_view_properties = view_props

        FigureErrorsManager.update_limits_and_legend(ax, self.legend_props)

    def close_tab(self):
        """Close the tab and set the view to None"""
        self.view.close()
        self.view = None

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
        return self.curve_names_dict[self.view.get_selected_curve_name()]

    def get_selected_curve_properties(self):
        """Get a CurveProperties object from the selected curve"""
        return CurveProperties.from_curve(self.get_selected_curve())

    def get_view_properties(self):
        """Get top level properties from view"""
        return self.view.get_properties()

    def _replot_selected_curve(self, plot_kwargs):
        """Replot the selected curve with the given plot kwargs"""
        ax = self.get_selected_ax()
        curve = self.get_selected_curve()
        new_curve = FigureErrorsManager.replot_curve(ax, curve, plot_kwargs)
        self.curve_names_dict[self.view.get_selected_curve_name()] = new_curve

    def populate_curve_combo_box_and_update_view(self):
        """
        Populate curve combo box and update the view with the curve's
        properties.
        """
        self.curve_names_dict = {}
        if self._populate_select_curve_combo_box():
            self.update_view()

    def populate_select_axes_combo_box(self):
        """
        Add Axes names to 'select axes' combo box.
        Names are generated similary to in AxesTabWidgetPresenter
        """
        # Sort names by axes position
        names = []
        for name, ax in self.axes_names_dict.items():
            if curve_in_ax(ax):
                names.append(name)
        names = sorted(names, key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def remove_selected_curve(self):
        """
        Remove selected curve from figure and combobox. If there are no
        curves left on the axes remove that axes from the axes combo box
        """
        ax = self.get_selected_ax()
        if ax.legend_:
            self.legend_props = LegendProperties.from_legend(ax.legend_)
        # Remove curve from ax and remove from curve names dictionary
        remove_curve_from_ax(self.get_selected_curve())
        self.curve_names_dict.pop(self.view.get_selected_curve_name())
        self.set_apply_to_all_buttons_enabled()

        ax = self.get_selected_ax()
        # Update the legend and redraw
        FigureErrorsManager.update_limits_and_legend(ax, self.legend_props)
        ax.figure.canvas.draw()

        # Remove the curve from the curve selection combo box
        if self.remove_selected_curve_combo_box_entry():
            return
        self.update_view()

    def remove_selected_curve_combo_box_entry(self):
        """
        Remove selected entry in 'select_curve_combo_box'. If no curves remain
        on the axes remove the axes entry from the 'select_axes_combo_box'. If
        no axes with curves remain close the tab and return True
        """
        with block_signals(self.view.select_curve_combo_box):
            self.view.remove_select_curve_combo_box_selected_item()
            if self.view.select_curve_combo_box.count() == 0:
                self.view.remove_select_axes_combo_box_selected_item()
                if self.view.select_axes_combo_box.count() == 0:
                    self.close_tab()
                    if self.legend_tab:
                        self.legend_tab.close_tab()
                    return True

    def set_new_curve_name_in_dict_and_combo_box(self, curve, new_label):
        """Update a curve name in the curve names dict and combo box"""
        old_name = self.view.get_selected_curve_name()
        if new_label:
            curve_name = self._generate_curve_name(curve, new_label)
            self.view.set_selected_curve_selector_text(curve_name)
            self.curve_names_dict[curve_name] = self.curve_names_dict.pop(old_name)

    def set_errorbars_tab_enabled(self):
        """Enable/disable the errorbar tab for selected curve"""
        enable_errorbars = curve_has_errors(self.get_selected_curve())
        self.view.set_errorbars_tab_enabled(enable_errorbars)

    def update_view(self):
        """Update the view with the selected curve's properties"""
        curve_props = CurveProperties.from_curve(self.get_selected_curve())
        self.view.update_fields(curve_props)
        self.set_errorbars_tab_enabled()
        self.current_view_properties = curve_props

    # Private methods
    def _generate_curve_name(self, curve, label):
        if label:
            if label == '_nolegend_':
                return None
            else:
                name = label
        else:
            name = '_nolabel_'
        # Deal with case of curves sharing the same label
        idx, base_name = 1, name
        while name in self.curve_names_dict:
            if self.curve_names_dict[name] == curve:
                break
            name = base_name + " ({})".format(idx)
            idx += 1
        return name

    def _get_selected_ax_errorbars(self):
        """Get all errorbar containers in selected axes"""
        ax = self.get_selected_ax()
        return FigureErrorsManager.get_errorbars_from_ax(ax)

    def _populate_select_curve_combo_box(self):
        """
        Add curves on selected axes to the 'select curves' combo box.
        Return False if there are no lines on the axes (this can occur
        when a user uses the "Remove Curve" button), else return True.
        """
        with block_signals(self.view.select_curve_combo_box):
            self.view.select_curve_combo_box.clear()
        selected_ax = self.get_selected_ax()
        if not selected_ax:
            self.view.close()
            return False

        active_lines = FigureErrorsManager.get_curves_from_ax(selected_ax)
        for line in active_lines:
            self._update_selected_curve_name(line)

        self.view.populate_select_curve_combo_box(
            sorted(self.curve_names_dict.keys(), key=lambda s: s.lower()))
        return True

    def _update_selected_curve_name(self, curve):
        """Update the selected curve's name in the curve_names_dict"""
        name = self._generate_curve_name(curve, curve.get_label())
        if name:
            self.curve_names_dict[name] = curve

    def set_apply_to_all_buttons_enabled(self):
        """
        Enables the Apply to All buttons in the line, marker, and errorbar tabs
        if there is more than one curve.
        """
        if len(self.curve_names_dict) > 1:
            self.view.line.set_apply_to_all_enabled(True)
            self.view.marker.set_apply_to_all_enabled(True)
            self.view.errorbars.set_apply_to_all_enabled(True)
        else:
            self.view.line.set_apply_to_all_enabled(False)
            self.view.marker.set_apply_to_all_enabled(False)
            self.view.errorbars.set_apply_to_all_enabled(False)

    def line_apply_to_all(self):
        """
        Applies the settings in the line tab for the current curve to all other curves.
        """
        current_curve_index = self.view.select_curve_combo_box.currentIndex()

        line_style = self.view.line.get_style()
        draw_style = self.view.line.get_draw_style()
        width = self.view.line.get_width()

        for i in range(len(self.curve_names_dict)):
            self.view.select_curve_combo_box.setCurrentIndex(i)

            self.view.line.set_style(line_style)
            self.view.line.set_draw_style(draw_style)
            self.view.line.set_width(width)

            self.apply_properties()

        self.fig.canvas.draw()
        self.view.select_curve_combo_box.setCurrentIndex(current_curve_index)

    def marker_apply_to_all(self):
        current_curve_index = self.view.select_curve_combo_box.currentIndex()

        marker_style = self.view.marker.get_style()
        marker_size = self.view.marker.get_size()

        for i in range(len(self.curve_names_dict)):
            self.view.select_curve_combo_box.setCurrentIndex(i)

            self.view.marker.set_style(marker_style)
            self.view.marker.set_size(marker_size)

            self.apply_properties()

        self.fig.canvas.draw()
        self.view.select_curve_combo_box.setCurrentIndex(current_curve_index)

    def errorbars_apply_to_all(self):
        current_curve_index = self.view.select_curve_combo_box.currentIndex()

        checked = self.view.errorbars.get_hide()

        if not checked:
            width = self.view.errorbars.get_width()
            capsize = self.view.errorbars.get_capsize()
            cap_thickness = self.view.errorbars.get_cap_thickness()
            error_every = self.view.errorbars.get_error_every()

        for i in range(len(self.curve_names_dict)):
            self.view.select_curve_combo_box.setCurrentIndex(i)

            self.view.errorbars.set_hide(checked)

            if not checked:
                self.view.errorbars.set_width(width)
                self.view.errorbars.set_capsize(capsize)
                self.view.errorbars.set_cap_thickness(cap_thickness)
                self.view.errorbars.set_error_every(error_every)

            self.apply_properties()

        self.fig.canvas.draw()
        self.view.select_curve_combo_box.setCurrentIndex(current_curve_index)
