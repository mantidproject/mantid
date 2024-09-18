# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mpl_toolkits.mplot3d.axes3d import Axes3D

from mantid.plots import convert_color_to_hex, axesfunctions
from mantidqt.widgets.plotconfigdialog import generate_ax_name, get_axes_names_dict
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.view import AxesTabWidgetView


class AxesTabWidgetPresenter:
    def __init__(self, fig, view=None, parent=None, success_callback=None, error_callback=None):
        self.fig = fig
        if not view:
            self.view = AxesTabWidgetView(parent)
        else:
            self.view = view
        self.success_callback = success_callback
        self.error_callback = error_callback

        # Store a copy of the current view props.
        self.current_view_props = {}

        self.current_axis = "x"

        # Dictionary mapping ax name to Axes object
        self.axes_names_dict = get_axes_names_dict(self.fig)
        # Add axes names to "select axes" combo box
        self.populate_select_axes_combo_box()
        # Display top axes' properties in input fields
        self.update_view()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(self.update_view)
        self.view.axis_tab_bar.currentChanged.connect(self.axis_changed)
        self.view.show_minor_ticks_check_box.toggled.connect(self.show_minor_ticks_checked)
        self.view.autoscale.toggled.connect(self.autoscale_changed)
        if self.success_callback and self.error_callback:
            self.view.apply_all_button.clicked.connect(self._apply_all_properties_with_errors)
        else:
            self.view.apply_all_button.clicked.connect(self.apply_all_properties)

    def apply_properties(self):
        """Update the axes with the user inputted properties"""
        # Make sure current_view_props is up to date if values have been changed
        self.axis_changed()
        ax = self.get_selected_ax()
        self.set_ax_title(ax, self.current_view_props["title"])
        self._apply_properties_to_axes(ax)

    def apply_all_properties(self):
        """Update the axes with the user inputted properties"""
        # Make sure current_view_props is up to date if values have been changed
        self.axis_changed()
        for ax in self.axes_names_dict.values():
            self._apply_properties_to_axes(ax)
            ax.figure.canvas.draw()

    def _apply_properties_to_axes(self, ax):
        """Apply current properties to given set of axes"""
        self.set_ax_canvas_color(ax, self.current_view_props["canvas_color"])

        if not isinstance(ax, Axes3D):
            if self.current_view_props["minor_ticks"]:
                ax.minorticks_on()
            else:
                ax.minorticks_off()

            ax.show_minor_gridlines = self.current_view_props["minor_gridlines"]

            # If the grid is enabled update it
            if ax.show_minor_gridlines:
                if ax.xaxis._major_tick_kw["gridOn"] and ax.yaxis._major_tick_kw["gridOn"]:
                    ax.grid(True, which="minor")
                elif ax.xaxis._major_tick_kw["gridOn"]:
                    ax.grid(True, axis="x", which="minor")
                elif ax.yaxis._major_tick_kw["gridOn"]:
                    ax.grid(True, axis="y", which="minor")
            else:
                ax.grid(False, which="minor")

        if "xlabel" in self.current_view_props:
            ax.set_xlabel(self.current_view_props["xlabel"])
            ax.set_xscale(self.current_view_props["xscale"].lower())  # As of mpl 3.5 scale settings must be lowercase

            if self.current_view_props["xautoscale"]:
                if ax.images:
                    axesfunctions.update_colorplot_datalimits(ax, ax.images, axis="x")
                else:
                    ax.autoscale(True, axis="x")
            else:
                if isinstance(ax, Axes3D):
                    ax.set_xlim3d(self.current_view_props["xlim"])
                else:
                    ax.set_xlim(self.current_view_props["xlim"])

        if "ylabel" in self.current_view_props:
            ax.set_ylabel(self.current_view_props["ylabel"])
            ax.set_yscale(self.current_view_props["yscale"].lower())

            if self.current_view_props["yautoscale"]:
                if ax.images:
                    axesfunctions.update_colorplot_datalimits(ax, ax.images, axis="y")
                else:
                    ax.autoscale(True, axis="y")
            else:
                if isinstance(ax, Axes3D):
                    ax.set_ylim3d(self.current_view_props["ylim"])
                else:
                    ax.set_ylim(self.current_view_props["ylim"])

        if isinstance(ax, Axes3D) and "zlabel" in self.current_view_props:
            ax.set_zlabel(self.current_view_props["zlabel"])
            ax.set_zscale(self.current_view_props["zscale"].lower())
            if self.current_view_props["zautoscale"]:
                ax.autoscale(True, axis="z")
            else:
                ax.set_zlim3d(self.current_view_props["zlim"])

    def get_selected_ax(self):
        """Get Axes object of selected axes"""
        return self.axes_names_dict[self.view.get_selected_ax_name()]

    def get_selected_ax_name(self):
        """Get name of selected axes"""
        return self.view.get_selected_ax_name()

    def get_selected_ax_properties(self):
        """Get axes properties from selected axes"""
        return AxProperties.from_ax_object(self.get_selected_ax())

    def populate_select_axes_combo_box(self):
        """Add axes' names to the 'select axes' combo box"""
        # Sort names by axes position
        names = sorted(self.axes_names_dict.keys(), key=lambda x: x[x.rfind("(") :])
        self.view.populate_select_axes_combo_box(names)

    def rename_selected_axes(self, new_name):
        """
        Rename the selected axes, updating the axes_names_dict and
        the select_axes_combo_box
        """
        old_name = self.get_selected_ax_name()
        self.view.set_selected_axes_selector_text(new_name)
        self.axes_names_dict[new_name] = self.axes_names_dict.pop(old_name)

    def set_ax_title(self, ax, new_title):
        """Set axes' title and update its entry in the axes selector"""
        ax.set_title(new_title)
        self.rename_selected_axes(generate_ax_name(ax))

    def set_ax_canvas_color(self, ax, color_rgb):
        """Set axes canvas color"""
        ax.set_facecolor(color_rgb)

    def update_view(self):
        """Update the properties in the view from the selected axes"""
        self.current_view_props.clear()
        ax_props = self.get_selected_ax_properties()

        ax = self.get_selected_ax()
        plot_is_3d = isinstance(ax, Axes3D)

        # Enable the z-axis option if the plot is 3D.
        self.view.set_z_axis_selector_enabled(plot_is_3d)

        # For tiled plots
        if not plot_is_3d and self.view.get_z_axis_selector_checked():
            self.view.set_x_axis_selector_click()

        # Changing the axis scale doesn't work with 3D plots, this is a known matplotlib issue,
        # so the scale option is disabled.
        self.view.set_scale_combo_box_enabled(not plot_is_3d)

        # Minor ticks/gridlines are currently not supported for 3D plots.
        self.view.set_minor_grid_tick_controls_visible(not plot_is_3d)

        ax = self.current_axis
        self.view.set_title(ax_props.title)

        color_hex = convert_color_to_hex(ax_props["canvas_color"])
        self.view.set_canvas_color(color_hex)

        if not plot_is_3d:
            self.view.set_show_minor_ticks(ax_props.minor_ticks)
            self.view.set_minor_gridlines_check_box_enabled(ax_props.minor_ticks)
            self.view.set_show_minor_gridlines(ax_props.minor_gridlines)

        self.view.set_autoscale_enabled(ax_props[f"{ax}autoscale"])
        self.view.set_limit_input_enabled(not ax_props[f"{ax}autoscale"])

        lim = ax_props[f"{ax}lim"]
        self.view.set_lower_limit(lim[0])
        self.view.set_upper_limit(lim[1])
        self.view.set_label(ax_props[f"{ax}label"])
        self.view.set_scale(ax_props[f"{ax}scale"])

        self.current_view_props.update(self.view.get_properties())

    def autoscale_changed(self):
        autoscale_enabled = self.view.get_autoscale_enabled()
        self.view.set_limit_input_enabled(not autoscale_enabled)

    def axis_changed(self):
        ax = self.current_axis
        self.current_view_props["title"] = self.view.get_title()
        self.current_view_props["minor_ticks"] = self.view.get_show_minor_ticks()
        self.current_view_props["minor_gridlines"] = self.view.get_show_minor_gridlines()
        self.current_view_props[f"{ax}lim"] = (self.view.get_lower_limit(), self.view.get_upper_limit())
        self.current_view_props[f"{ax}label"] = self.view.get_label()
        self.current_view_props[f"{ax}scale"] = self.view.get_scale()
        self.current_view_props[f"{ax}autoscale"] = self.view.get_autoscale_enabled()
        self.current_view_props["canvas_color"] = self.view.get_canvas_color()

        new_ax = self.view.get_axis()
        self.current_axis = new_ax

        if f"{new_ax}lim" in self.current_view_props:
            self.view.set_autoscale_enabled(self.current_view_props[f"{new_ax}autoscale"])
            self.view.set_limit_input_enabled(not self.current_view_props[f"{new_ax}autoscale"])
            lim = self.current_view_props[f"{new_ax}lim"]
            self.view.set_lower_limit(lim[0])
            self.view.set_upper_limit(lim[1])
            self.view.set_label(self.current_view_props[f"{new_ax}label"])
            self.view.set_scale(self.current_view_props[f"{new_ax}scale"])
        else:
            ax_props = self.get_selected_ax_properties()
            ax = self.view.get_axis()
            self.view.set_autoscale_enabled(ax_props[f"{ax}autoscale"])
            self.view.set_limit_input_enabled(not ax_props[f"{ax}autoscale"])
            lim = ax_props[f"{ax}lim"]
            self.view.set_lower_limit(lim[0])
            self.view.set_upper_limit(lim[1])
            self.view.set_label(ax_props[f"{ax}label"])
            self.view.set_scale(ax_props[f"{ax}scale"])

    def show_minor_ticks_checked(self, checked):
        # Can't have minor gridlines without minor ticks
        self.view.set_minor_gridlines_check_box_enabled(checked)

        if not checked:
            self.view.set_show_minor_gridlines(False)

    def _apply_all_properties_with_errors(self):
        """Call apply_all_properties, notifying the parent presenter of whether there were any errors"""
        try:
            self.apply_all_properties()
        except (AssertionError, IndexError, KeyError, TypeError, ValueError) as exception:
            self.error_callback(str(exception))
            return
        self.success_callback()
