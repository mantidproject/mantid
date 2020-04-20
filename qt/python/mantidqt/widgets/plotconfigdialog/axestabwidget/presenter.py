# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.plots import MantidAxes3D
from mantidqt.widgets.plotconfigdialog import generate_ax_name, get_axes_names_dict
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.view import AxesTabWidgetView


class AxesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if not view:
            self.view = AxesTabWidgetView(parent)
        else:
            self.view = view

        # Store a copy of the current view props. This allows us to tell if any
        # properties have been changed by the user; especially solving issues
        # with x and y axis limits interfering with figure autoscaling
        self.current_view_props = {}

        self.current_axis = "x"

        # Dictionary mapping ax name to Axes object
        self.axes_names_dict = get_axes_names_dict(self.fig)
        # Add axes names to "select axes" combo box
        self.populate_select_axes_combo_box()
        # Display top axes' properties in input fields
        self.update_view()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.update_view)
        self.view.axis_button_group.buttonClicked.connect(
            self.axis_changed)

    def apply_properties(self):
        """Update the axes with the user inputted properties"""
        # Make sure current_view_props is up to date if values have been changed
        self.axis_changed()

        ax = self.get_selected_ax()

        self.set_ax_title(ax, self.current_view_props['title'])

        if "xlabel" in self.current_view_props:
            ax.set_xlabel(self.current_view_props['xlabel'])
            ax.set_xscale(self.current_view_props['xscale'])
            ax.set_xlim(self.current_view_props['xlim'])

        if "ylabel" in self.current_view_props:
            ax.set_ylabel(self.current_view_props['ylabel'])
            ax.set_yscale(self.current_view_props['yscale'])
            ax.set_ylim(self.current_view_props['ylim'])

        if "zlabel" in self.current_view_props:
            ax.set_zlabel(self.current_view_props['zlabel'])
            ax.set_zscale(self.current_view_props['zscale'])
            ax.set_zlim(self.current_view_props['zlim'])

        self.update_view()

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
        names = sorted(self.axes_names_dict.keys(),
                       key=lambda x: x[x.rfind("("):])
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

    def update_view(self):
        """Update the properties in the view from the selected axes"""
        self.current_view_props.clear()
        ax_props = self.get_selected_ax_properties()

        plot_is_3d = "zlim" in ax_props

        # Enable the z-axis option if the plot is 3D.
        self.view.z_radio_button.setEnabled(plot_is_3d)

        # For tiled plots
        if not plot_is_3d and self.view.z_radio_button.isChecked():
            self.view.x_radio_button.click()

        # Changing the axis scale doesn't work with 3D plots, this is a known matplotlib issue,
        # so the scale option is disabled.
        self.view.scale_combo_box.setEnabled("zlim" not in ax_props)

        ax = self.view.get_axis()
        self.view.set_title(ax_props.title)
        lim = ax_props[f"{ax}lim"]
        self.view.set_lower_limit(lim[0])
        self.view.set_upper_limit(lim[1])
        self.view.set_label(ax_props[f"{ax}label"])
        self.view.set_scale(ax_props[f"{ax}scale"])

        self.current_view_props.update(self.view.get_properties())

    def axis_changed(self):
        ax = self.current_axis

        self.current_view_props['title'] = self.view.get_title()
        self.current_view_props[f"{ax}lim"] = (self.view.get_lower_limit(), self.view.get_upper_limit())
        self.current_view_props[f"{ax}label"] = self.view.get_label()
        self.current_view_props[f"{ax}scale"] = self.view.get_scale()

        new_ax = self.view.get_axis()
        self.current_axis = new_ax

        if f"{new_ax}lim" in self.current_view_props:
            lim = self.current_view_props[f"{new_ax}lim"]
            self.view.set_lower_limit(lim[0])
            self.view.set_upper_limit(lim[1])
            self.view.set_label(self.current_view_props[f"{new_ax}label"])
            self.view.set_scale(self.current_view_props[f"{new_ax}scale"])
        else:
            ax_props = self.get_selected_ax_properties()
            ax = self.view.get_axis()
            lim = ax_props[f"{ax}lim"]
            self.view.set_lower_limit(lim[0])
            self.view.set_upper_limit(lim[1])
            self.view.set_label(ax_props[f"{ax}label"])
            self.view.set_scale(ax_props[f"{ax}scale"])
