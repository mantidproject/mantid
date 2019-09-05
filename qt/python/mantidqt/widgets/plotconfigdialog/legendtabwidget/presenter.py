# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.advancedlegendoptionsdialog.presenter import AdvancedLegendOptionsPresenter
from mantidqt.widgets.plotconfigdialog.legendtabwidget.view import LegendTabWidgetView
from mantidqt.widgets.plotconfigdialog.legendtabwidget import LegendProperties

import matplotlib.font_manager

class LegendTabWidgetPresenter:
    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        self.ax = fig.gca()
        if not view:
            self.view = LegendTabWidgetView(parent)
        else:
            self.view = view

        self.legend = self.ax.get_legend()
        self.populate_font_combo_box()
        self.init_view()

        self.view.transparency_spin_box.valueChanged.connect(
            lambda: self.view.set_transparency_slider(self.view.get_transparency_spin_box_value()))
        self.view.transparency_slider.valueChanged.connect(
            lambda: self.view.set_transparency_spin_box(self.view.get_transparency_slider_value()))
        self.view.hide_legend_check_box.stateChanged.connect(self.hide_legend_ticked)
        self.view.hide_box_check_box.stateChanged.connect(self.hide_box_ticked)
        self.view.advanced_options_push_button.clicked.connect(self.show_advanced_options)

    def init_view(self):
        legend_props = LegendProperties.from_legend(self.legend)
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

        if not self.legend.get_visible():
            self.view.hide_legend_check_box.setChecked(True)
            self.hide_legend_ticked()

        if not self.legend.get_frame_on():
            self.view.hide_box_check_box.setChecked(True)
            self.hide_box_ticked()

    def apply_properties(self):
        props = self.view.get_properties()
        self.legend.remove()
        legend = self.ax.legend(ncol=1,
                                prop={'size': props.entries_size},
                                numpoints=1,
                                markerfirst=True,
                                frameon=not self.view.hide_box_check_box.isChecked(),
                                fancybox=True,
                                shadow=False,
                                framealpha=props.transparency,
                                facecolor=props.background_color,
                                edgecolor=props.edge_color,
                                title=props.title,
                                borderpad=None,
                                labelspacing=0.5,
                                handlelength=props.marker_size,
                                handletextpad=None,
                                columnspacing=5.0)

        title = legend.get_title()
        title.set_fontname(props.title_font)
        title.set_fontsize(props.title_size)
        title.set_color(props.title_color)

        for text in legend.get_texts():
            text.set_fontname(props.entries_font)
            text.set_fontsize(props.entries_size)
            text.set_color(props.entries_color)

        if self.view.hide_legend_check_box.isChecked():
            legend.set_visible(False)
        else:
            legend.set_visible(True)

        legend.draggable(True)

    def populate_font_combo_box(self):
        font_list = matplotlib.font_manager.findSystemFonts()
        self.fonts = set(sorted(
            matplotlib.font_manager.FontProperties(fname=font_name).get_name() for font_name in font_list))
        self.view.entries_font_combo_box.addItems(self.fonts)
        self.view.title_font_combo_box.addItems(self.fonts)

    def hide_legend_ticked(self):
        checked = not self.view.hide_legend_check_box.isChecked()

        self.view.title_line_edit.setEnabled(checked)
        self.view.title_font_combo_box.setEnabled(checked)
        self.view.title_size_spin_box.setEnabled(checked)
        self.view.title_color_selector_widget.setEnabled(checked)
        self.view.hide_box_check_box.setEnabled(checked)

        if not self.view.hide_box_check_box.isChecked():
            self.view.background_color_selector_widget.setEnabled(checked)
            self.view.edge_color_selector_widget.setEnabled(checked)
            self.view.transparency_slider.setEnabled(checked)
            self.view.transparency_spin_box.setEnabled(checked)

        self.view.entries_font_combo_box.setEnabled(checked)
        self.view.entries_size_spin_box.setEnabled(checked)
        self.view.entries_color_selector_widget.setEnabled(checked)
        self.view.advanced_options_push_button.setEnabled(checked)

    def hide_box_ticked(self):
        checked = not self.view.hide_box_check_box.isChecked()

        self.view.background_color_selector_widget.setEnabled(checked)
        self.view.edge_color_selector_widget.setEnabled(checked)
        self.view.transparency_slider.setEnabled(checked)
        self.view.transparency_spin_box.setEnabled(checked)

    def check_font_in_list(self, font):
        if self.view.entries_font_combo_box.findText(font) == -1:
            self.fonts.add(font)
            self.fonts = sorted(self.fonts)
            self.view.entries_font_combo_box.clear()
            self.view.title_font_combo_box.clear()
            self.view.entries_font_combo_box.addItems(self.fonts)
            self.view.title_font_combo_box.addItems(self.fonts)

    def show_advanced_options(self):
        self.advanced_options = AdvancedLegendOptionsPresenter(parent=self.parent.parent.window)
