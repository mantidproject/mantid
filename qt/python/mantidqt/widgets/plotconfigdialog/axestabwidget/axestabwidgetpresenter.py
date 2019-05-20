# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.axestabwidgetview import AxesTabWidgetView


class AxesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if not view:
            self.view = AxesTabWidgetView(parent)
        else:
            self.view = view

        # Dictionary mapping ax name to Axes object
        self.axes_names_dict = self.get_axes_names_dict()
        # Add axes names to "select axes" combo box
        self.populate_select_axes_combo_box()
        # Display top axes' properties in input fields
        self.set_selected_ax_view_properties()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.set_selected_ax_view_properties
        )

    def apply_properties(self):
        """Update the axes with the user inputted properties"""
        ax = self.get_current_ax()
        new_props = self.view.get_properties()
        self.set_ax_title(ax, new_props.title)
        ax.set_xlim(new_props.xlim)
        ax.set_xlabel(new_props.xlabel)
        ax.set_xscale(new_props.xscale)
        ax.set_ylim(new_props.ylim)
        ax.set_ylabel(new_props.ylabel)
        ax.set_yscale(new_props.yscale)

    def generate_ax_name(self, ax):
        """
        Generate a name for the given axes. This will come from the
        title of the axes (if there is one) and the position of the axes
        on the figure.
        """
        title = ax.get_title().split('\n')[0].strip()
        position = "({}, {})".format(ax.rowNum, ax.colNum)
        if title:
            return "{}: {}".format(title, position)
        return position

    def get_axes_names_dict(self):
        """
        Return dictionary mapping the axes names listed in the combobox
        to the Axes object.
        """
        axes_names = {}
        for ax in self.fig.get_axes():
            axes_names[self.generate_ax_name(ax)] = ax
        return axes_names

    def get_current_ax(self):
        """Get Axes object of currently selected axes"""
        return self.axes_names_dict[self.view.get_selected_ax_name()]

    def get_current_ax_name(self):
        """Get name of currently selected axes"""
        return self.view.get_selected_ax_name()

    def get_current_ax_properties(self):
        """Get axes properties from currently selected axes"""
        return AxProperties.from_ax_object(self.get_current_ax())

    def populate_select_axes_combo_box(self):
        """Add axes' names to the 'select axes' combo box"""
        # Sort names by axes position
        names = sorted(self.axes_names_dict.keys(),
                       key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def replace_current_axes_name(self, old_name, new_name):
        self.axes_names_dict[new_name] = self.axes_names_dict.pop(old_name)
        self.view.set_current_axes_selector_text(new_name)

    def set_ax_title(self, ax, new_title):
        """Set axes' title and update its entry in the axes selector"""
        ax.set_title(new_title)
        self.replace_current_axes_name(self.get_current_ax_name(),
                                       self.generate_ax_name(ax))

    def set_selected_ax_view_properties(self):
        """Update the properties in the view from the selected axes"""
        ax_props = self.get_current_ax_properties()
        self.view.set_properties(ax_props)




