# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.legendtabwidget.view import LegendTabWidgetView
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties

import matplotlib
import matplotlib.font_manager


class LegendTabWidgetPresenter:
    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        self.axes = fig.get_axes()

        if not view:
            self.view = LegendTabWidgetView(parent)
        else:
            self.view = view

        self.current_view_properties = None
        self.populate_font_combo_box()
        self.init_view()

        # Signals
        self.view.transparency_spin_box.valueChanged.connect(
            lambda: self.view.set_transparency_slider(self.view.get_transparency_spin_box_value()))
        self.view.transparency_slider.valueChanged.connect(
            lambda: self.view.set_transparency_spin_box(self.view.get_transparency_slider_value()))
        self.view.hide_legend_check_box.stateChanged.connect(
            lambda: self.hide_legend_ticked(not self.view.get_hide_legend()))
        self.view.hide_box_check_box.stateChanged.connect(
            lambda: self.hide_box_ticked(not self.view.get_hide_box()))
        self.view.advanced_options_push_button.clicked.connect(self.show_advanced_options)
        self.view.advanced_options.rejected.connect(self.advanced_options_cancelled)

    def init_view(self):
        if int(matplotlib.__version__[0]) < 2:
            self.view.background_color_selector_widget.setVisible(False)
            self.view.edge_color_selector_widget.setVisible(False)

        """Sets all of the initial values of the input fields when the tab is first loaded"""
        legend_props = LegendProperties.from_legend(self.axes[0].get_legend())
        self.check_font_in_list(legend_props.entries_font)
        self.check_font_in_list(legend_props.title_font)

        self.view.set_title(legend_props.title)
        self.view.set_background_color(legend_props.background_color)
        self.view.set_edge_color(legend_props.edge_color)

        # Converts alpha value (opacity value between 0 and 1) to transparency percentage.
        transparency = 100-(legend_props.transparency*100)
        self.view.set_transparency_spin_box(transparency)
        self.view.set_transparency_slider(transparency)
        self.view.set_entries_font(legend_props.entries_font)
        self.view.set_entries_size(legend_props.entries_size)
        self.view.set_entries_color(legend_props.entries_color)
        self.view.set_title_font(legend_props.title_font)
        self.view.set_title_size(legend_props.title_size)
        self.view.set_title_color(legend_props.title_color)
        self.view.set_marker_size(legend_props.marker_size)

        self.view.advanced_options.set_border_padding(legend_props.border_padding)
        self.view.advanced_options.set_column_spacing(legend_props.column_spacing)
        self.view.advanced_options.set_label_spacing(legend_props.label_spacing)
        self.view.advanced_options.set_marker_position(legend_props.marker_position)
        self.view.advanced_options.set_number_of_columns(legend_props.columns)
        self.view.advanced_options.set_number_of_markers(legend_props.markers)
        self.view.advanced_options.set_round_edges(legend_props.round_edges)
        self.view.advanced_options.set_shadow(legend_props.shadow)
        self.view.advanced_options.set_marker_label_padding(legend_props.marker_label_padding)

        visible = legend_props.visible
        self.view.set_hide_legend(not visible)
        self.hide_legend_ticked(visible)

        box_visible = legend_props.box_visible
        self.view.set_hide_box(not box_visible)
        self.hide_box_ticked(box_visible)

        self.current_view_properties = legend_props

    def apply_properties(self):
        """Takes the properties from the view and creates a new legend."""
        props = self.view.get_properties()

        if self.current_view_properties == props:
            return

        for ax in self.axes:
            LegendProperties.create_legend(props, ax)

        self.current_view_properties = props

    def populate_font_combo_box(self):
        """Adds all of the available fonts to the font combo boxes."""
        font_list = matplotlib.font_manager.findSystemFonts()

        self.fonts = set()
        for font_name in font_list:
            # This try-except is for a known matplotlib bug where get_name() causes an error for certain fonts.
            try:
                font = matplotlib.font_manager.FontProperties(fname=font_name).get_name()
            except RuntimeError:
                continue
            self.fonts.add(font)

        self.fonts = sorted(self.fonts)
        self.view.entries_font_combo_box.addItems(self.fonts)
        self.view.title_font_combo_box.addItems(self.fonts)

    def hide_legend_ticked(self, enable):
        """Disables or enables all of the input fields when the hide legend box is ticked or unticked."""
        self.view.title_line_edit.setEnabled(enable)
        self.view.title_font_combo_box.setEnabled(enable)
        self.view.title_size_spin_box.setEnabled(enable)
        self.view.title_color_selector_widget.setEnabled(enable)
        self.view.hide_box_check_box.setEnabled(enable)

        if not self.view.get_hide_box():
            self.view.background_color_selector_widget.setEnabled(enable)
            self.view.edge_color_selector_widget.setEnabled(enable)
            self.view.transparency_slider.setEnabled(enable)
            self.view.transparency_spin_box.setEnabled(enable)

        self.view.entries_font_combo_box.setEnabled(enable)
        self.view.entries_size_spin_box.setEnabled(enable)
        self.view.entries_color_selector_widget.setEnabled(enable)
        self.view.marker_size_spin_box.setEnabled(enable)
        self.view.advanced_options_push_button.setEnabled(enable)

    def hide_box_ticked(self, enable):
        """Disables or enables all options related to the legend box when hide box is ticked or unticked."""
        self.view.background_color_selector_widget.setEnabled(enable)
        self.view.edge_color_selector_widget.setEnabled(enable)
        self.view.transparency_slider.setEnabled(enable)
        self.view.transparency_spin_box.setEnabled(enable)
        self.view.advanced_options.shadow_check_box.setEnabled(enable)
        self.view.advanced_options.round_edges_check_box.setEnabled(enable)

    def check_font_in_list(self, font):
        """For some reason the default matplotlib legend font isn't a system font so it's added
        to the font combo boxes here."""
        if self.view.entries_font_combo_box.findText(font) == -1:
            self.fonts.append(font)
            self.fonts = sorted(self.fonts)
            self.view.entries_font_combo_box.clear()
            self.view.title_font_combo_box.clear()
            self.view.entries_font_combo_box.addItems(self.fonts)
            self.view.title_font_combo_box.addItems(self.fonts)

    def show_advanced_options(self):
        """Opens the advanced options dialog."""
        self.view.advanced_options.show()
        # The current properties in the dialog are stored so they can be reverted back to if the user cancels.
        self.current_advanced_properties = self.view.advanced_options.get_properties()

    def advanced_options_cancelled(self):
        """When the user clicks cancel in the advanced options dialog, the input fields reset to what they were
        before the user opened the dialog."""
        self.view.advanced_options.set_border_padding(self.current_advanced_properties.border_padding)
        self.view.advanced_options.set_column_spacing(self.current_advanced_properties.column_spacing)
        self.view.advanced_options.set_label_spacing(self.current_advanced_properties.label_spacing)
        self.view.advanced_options.set_marker_position(self.current_advanced_properties.marker_position)
        self.view.advanced_options.set_number_of_columns(self.current_advanced_properties.columns)
        self.view.advanced_options.set_number_of_markers(self.current_advanced_properties.markers)
        self.view.advanced_options.set_round_edges(self.current_advanced_properties.round_edges)
        self.view.advanced_options.set_shadow(self.current_advanced_properties.shadow)

    def close_tab(self):
        """Closes the tab and sets the view to None"""
        self.view.close()
        self.view = None
