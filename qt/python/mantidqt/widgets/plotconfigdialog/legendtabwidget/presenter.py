# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.legendtabwidget.advancedlegendoptionsdialog.view import AdvancedLegendOptionsView
from mantidqt.widgets.plotconfigdialog.legendtabwidget.view import LegendTabWidgetView
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties

import matplotlib.font_manager

class LegendTabWidgetPresenter:
    def __init__(self, fig, view=None, parent=None):
        self.fig = fig

        self.legends = []
        for ax in fig.get_axes():
            self.legends.append(ax.get_legend())

        if not view:
            self.view = LegendTabWidgetView(parent)
        else:
            self.view = view

        self.advanced_options = AdvancedLegendOptionsView(self.view)

        self.populate_font_combo_box()
        self.init_view()

        self.view.transparency_spin_box.valueChanged.connect(
            lambda: self.view.set_transparency_slider(self.view.get_transparency_spin_box_value()))
        self.view.transparency_slider.valueChanged.connect(
            lambda: self.view.set_transparency_spin_box(self.view.get_transparency_slider_value()))
        self.view.hide_legend_check_box.stateChanged.connect(
            lambda: self.hide_legend_ticked(not self.view.get_hide_legend()))
        self.view.hide_box_check_box.stateChanged.connect(
            lambda: self.hide_box_ticked(not self.view.get_hide_box()))
        self.view.advanced_options_push_button.clicked.connect(self.show_advanced_options)
        self.advanced_options.rejected.connect(self.advanced_options_cancelled)

    def init_view(self):
        legend_props = LegendProperties.from_legend(self.legends[0])
        self.check_font_in_list(legend_props.entries_font)
        self.check_font_in_list(legend_props.title_font)

        self.view.set_title(legend_props.title)
        self.view.set_background_color(legend_props.background_color)
        self.view.set_edge_color(legend_props.edge_color)
        self.view.set_transparency_spin_box(legend_props.transparency)
        self.view.set_transparency_slider(legend_props.transparency)
        self.view.set_entries_font(legend_props.entries_font)
        self.view.set_entries_size(legend_props.entries_size)
        self.view.set_entries_color(legend_props.entries_color)
        self.view.set_title_font(legend_props.title_font)
        self.view.set_title_size(legend_props.title_size)
        self.view.set_title_color(legend_props.title_color)
        self.view.set_marker_size(legend_props.marker_size)

        self.advanced_options.set_border_padding(legend_props.border_padding)
        self.advanced_options.set_column_spacing(legend_props.column_spacing)
        self.advanced_options.set_label_spacing(legend_props.label_spacing)
        self.advanced_options.set_marker_position(legend_props.marker_position)
        self.advanced_options.set_number_of_columns(legend_props.columns)
        self.advanced_options.set_number_of_markers(legend_props.markers)
        self.advanced_options.set_round_edges(legend_props.round_edges)
        self.advanced_options.set_shadow(legend_props.shadow)
        self.advanced_options.set_marker_label_padding(legend_props.marker_label_padding)

        visible = legend_props.visible
        self.view.set_hide_legend(not visible)
        self.hide_legend_ticked(visible)

        box_visible = legend_props.box_visible
        self.view.set_hide_box(not box_visible)
        self.hide_box_ticked(box_visible)

    def apply_properties(self):
        props = self.view.get_properties()
        advanced_props = self.advanced_options.get_properties()

        for legend in self.legends:
            legend = legend.axes.legend(ncol=advanced_props.columns,
                                        prop={'size': props.entries_size},
                                        numpoints=advanced_props.markers,
                                        markerfirst=advanced_props.marker_position == "Left of Entries",
                                        frameon=props.box_visible,
                                        fancybox=advanced_props.round_edges,
                                        shadow=advanced_props.shadow,
                                        framealpha=props.transparency,
                                        facecolor=props.background_color,
                                        edgecolor=props.edge_color,
                                        title=props.title,
                                        borderpad=advanced_props.border_padding,
                                        labelspacing=advanced_props.label_spacing,
                                        handlelength=props.marker_size,
                                        handletextpad=advanced_props.marker_label_padding,
                                        columnspacing=advanced_props.column_spacing)

            title = legend.get_title()
            title.set_fontname(props.title_font)
            title.set_fontsize(props.title_size)
            title.set_color(props.title_color)

            for text in legend.get_texts():
                text.set_fontname(props.entries_font)
                text.set_fontsize(props.entries_size)
                text.set_color(props.entries_color)

            legend.set_visible(props.visible)

            legend.draggable(True)

    def populate_font_combo_box(self):
        font_list = matplotlib.font_manager.findSystemFonts()
        self.fonts = set(sorted(
            matplotlib.font_manager.FontProperties(fname=font_name).get_name() for font_name in font_list))
        self.view.entries_font_combo_box.addItems(self.fonts)
        self.view.title_font_combo_box.addItems(self.fonts)

    def hide_legend_ticked(self, enable):
        self.view.title_line_edit.setEnabled(enable)
        self.view.title_font_combo_box.setEnabled(enable)
        self.view.title_size_spin_box.setEnabled(enable)
        self.view.title_color_selector_widget.setEnabled(enable)
        self.view.hide_box_check_box.setEnabled(enable)

        if not self.view.hide_box_check_box.isChecked():
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
        self.view.background_color_selector_widget.setEnabled(enable)
        self.view.edge_color_selector_widget.setEnabled(enable)
        self.view.transparency_slider.setEnabled(enable)
        self.view.transparency_spin_box.setEnabled(enable)

    def check_font_in_list(self, font):
        if self.view.entries_font_combo_box.findText(font) == -1:
            self.fonts.add(font)
            self.fonts = sorted(self.fonts)
            self.view.entries_font_combo_box.clear()
            self.view.title_font_combo_box.clear()
            self.view.entries_font_combo_box.addItems(self.fonts)
            self.view.title_font_combo_box.addItems(self.fonts)

    def show_advanced_options(self):
        self.advanced_options.show()
        self.current_advanced_properties = self.advanced_options.get_properties()

    def advanced_options_cancelled(self):
        self.advanced_options.set_border_padding(self.current_advanced_properties.border_padding)
        self.advanced_options.set_column_spacing(self.current_advanced_properties.column_spacing)
        self.advanced_options.set_label_spacing(self.current_advanced_properties.label_spacing)
        self.advanced_options.set_marker_position(self.current_advanced_properties.marker_position)
        self.advanced_options.set_number_of_columns(self.current_advanced_properties.columns)
        self.advanced_options.set_number_of_markers(self.current_advanced_properties.markers)
        self.advanced_options.set_round_edges(self.current_advanced_properties.round_edges)
        self.advanced_options.set_shadow(self.current_advanced_properties.shadow)