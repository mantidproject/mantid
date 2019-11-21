# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

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
        self.current_view_props = None

        # Dictionary mapping ax name to Axes object
        self.axes_names_dict = get_axes_names_dict(self.fig)
        # Add axes names to "select axes" combo box
        self.populate_select_axes_combo_box()
        # Display top axes' properties in input fields
        self.update_view()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.update_view)

    def apply_properties(self):
        """Update the axes with the user inputted properties"""
        ax = self.get_selected_ax()
        try:
            new_props = self.view.get_properties()
        except ValueError as e:
            self.view.error_occurred(str(e))
        else:
            self.set_ax_title(ax, new_props.title)
            ax.set_xlabel(new_props.xlabel)
            ax.set_xscale(new_props.xscale)
            if new_props.xscale == 'Log' and new_props.xlim[0] < 0:
                ax.set_xlim(new_props.xlim[1]*0.01, new_props.xlim[1])
            else:
                ax.set_xlim(new_props.xlim)
            ax.set_ylabel(new_props.ylabel)
            ax.set_yscale(new_props.yscale)
            if new_props.yscale == 'Log' and new_props.ylim[0] < 0:
                ax.set_ylim(new_props.ylim[1]*0.01, new_props.ylim[1])
            else:
                ax.set_ylim(new_props.ylim)
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
        ax_props = self.get_selected_ax_properties()
        self.view.set_title(ax_props.title)
        self.view.set_xlower_limit(ax_props.xlim[0])
        self.view.set_xupper_limit(ax_props.xlim[1])
        self.view.set_xlabel(ax_props.xlabel)
        self.view.set_xscale(ax_props.xscale)
        self.view.set_ylower_limit(ax_props.ylim[0])
        self.view.set_yupper_limit(ax_props.ylim[1])
        self.view.set_ylabel(ax_props.ylabel)
        self.view.set_yscale(ax_props.yscale)
        self.current_view_props = AxProperties.from_view(self.view)
