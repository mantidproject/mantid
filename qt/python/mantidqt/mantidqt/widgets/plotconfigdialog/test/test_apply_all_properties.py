# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
from matplotlib import use as mpl_use

mpl_use("Agg")
from matplotlib.colors import LogNorm
from matplotlib.ticker import NullLocator
from matplotlib.patches import BoxStyle
from matplotlib.pyplot import figure

from mantid.plots.legend import LegendProperties
from mantid.plots.utility import convert_color_to_hex
from unittest.mock import Mock, patch
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.presenter import AxesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.imagestabwidget import ImageProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog.presenter import PlotConfigDialogPresenter

AX_VIEW = "mantidqt.widgets.plotconfigdialog.axestabwidget.presenter.AxesTabWidgetView"
CURVE_VIEW = "mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter.CurvesTabWidgetView"
IMAGE_VIEW = "mantidqt.widgets.plotconfigdialog.imagestabwidget.presenter.ImagesTabWidgetView"
LEGEND_VIEW = "mantidqt.widgets.plotconfigdialog.legendtabwidget.presenter.LegendTabWidgetView"

new_ax_view_props = {
    "title": "New Title",
    "xlim": [0.1, 10],
    "xlabel": "New X Label",
    "xscale": "log",
    "xautoscale": False,
    "ylim": [0.2, 3],
    "ylabel": "New Y Label",
    "yscale": "log",
    "yautoscale": False,
    "minor_ticks": True,
    "minor_gridlines": True,
    "canvas_color": "#ffff00",
}

new_curve_view_props = {
    "label": "New label",
    "hide": False,
    "linestyle": "dotted",
    "drawstyle": "steps",
    "linewidth": 0.5,
    "color": "#ff0000",
    "marker": "_",
    "markerfacecolor": "#ff0001",
    "markeredgecolor": "#ff0002",
    "hide_errors": True,
    "errorevery": 2,
    "capsize": 3,
    "capthick": 4,
    "ecolor": "#ff6550",
    "elinewidth": 5,
}

new_image_props = {"label": "new label", "colormap": "jet", "vmin": 1, "vmax": 5, "interpolation": "hanning", "scale": "Logarithmic"}

new_legend_props = {
    "visible": True,
    "title": "Legend",
    "background_color": "#ffffff",
    "edge_color": "#000000",
    "transparency": 0.5,
    "entries_font": "Bitstream Vera Sans",
    "entries_size": 12,
    "entries_color": "#000000",
    "title_font": "Bitstream Vera Sans",
    "title_size": 14,
    "title_color": "#000000",
    "marker_size": 3.0,
    "box_visible": True,
    "shadow": True,
    "round_edges": False,
    "columns": 1,
    "column_spacing": 0.5,
    "label_spacing": 0.5,
    "marker_position": "Left of Entries",
    "markers": 2,
    "border_padding": 0.0,
    "marker_label_padding": 1.0,
}


class CurveNameSideEffect:
    def __init__(self, old_name, new_name, switch_count):
        self.old_name = old_name
        self.new_name = new_name
        self.switch_count = switch_count

        self.call_count = 0

    def __call__(self):
        self.call_count += 1
        if self.call_count <= self.switch_count:
            return self.old_name
        return self.new_name


def mock_axes_tab_presenter_update_view(presenter):
    presenter.current_view_props = new_ax_view_props


def _run_apply_properties_on_figure_with_curve(curve_view_mock):
    fig = figure()
    ax = fig.add_subplot(111)
    ax.errorbar([0, 1], [0, 1], yerr=[0.1, 0.2], label="old label")
    ax.containers[0][2][0].axes.creation_args = [{"errorevery": 2}]
    curve_view_mock.get_current_curve_name = CurveNameSideEffect("old label", "New label", switch_count=6)

    with patch.object(AxesTabWidgetPresenter, "update_view", mock_axes_tab_presenter_update_view):
        presenter = PlotConfigDialogPresenter(fig, view=Mock())
    presenter.tab_widget_views[1][0].select_curve_combo_box.currentIndex.return_value = 0
    with patch.object(presenter.tab_widget_presenters[1], "update_view", lambda: None):
        with patch.object(presenter.tab_widget_presenters[1], "axis_changed", lambda: None):
            presenter.apply_properties()
    return ax


def _run_apply_properties_on_figure_with_image():
    img_fig = figure()
    img_ax = img_fig.add_subplot(111)
    image = img_ax.imshow([[0, 1], [0, 1]])
    cb = img_fig.colorbar(image)
    cb.set_label("old label")

    with patch.object(AxesTabWidgetPresenter, "update_view", mock_axes_tab_presenter_update_view):
        presenter = PlotConfigDialogPresenter(img_fig, view=Mock())
    with patch.object(presenter.tab_widget_presenters[1], "update_view", lambda: None):
        with patch.object(presenter.tab_widget_presenters[1], "axis_changed", lambda: None):
            presenter.apply_properties()
    return img_ax


def _run_apply_properties_on_figure_with_legend(curve_view_mock):
    fig = figure()
    ax = fig.add_subplot(111)
    ax.plot([1, 2, 3], label="old label")
    legend = ax.legend()
    legend.get_frame().set_alpha(0.5)
    curve_view_mock.get_current_curve_name = CurveNameSideEffect("old label", "New label", switch_count=3)

    with patch.object(AxesTabWidgetPresenter, "update_view", mock_axes_tab_presenter_update_view):
        presenter = PlotConfigDialogPresenter(fig, view=Mock())
    with patch.object(presenter.tab_widget_presenters[1], "update_view", lambda: None):
        with patch.object(presenter.tab_widget_presenters[1], "axis_changed", lambda: None):
            presenter.apply_properties()
    return ax


class ApplyAllPropertiesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Mock axes tab view
        mock_axes_view = Mock(get_selected_ax_name=lambda: "(0, 0)", get_properties=lambda: AxProperties(new_ax_view_props))
        cls.ax_view_patch = patch(AX_VIEW, lambda x: mock_axes_view)
        cls.ax_view_mock = cls.ax_view_patch.start()

        # Mock curves tab view
        cls.curve_view_mock = Mock(
            get_selected_ax_name=lambda: "(0, 0)",
            select_curve_list=Mock(selectedItems=lambda: []),
            get_properties=lambda: CurveProperties(new_curve_view_props),
        )
        cls.curve_view_patch = patch(CURVE_VIEW, lambda x: cls.curve_view_mock)
        cls.curve_view_patch.start()

        cls.ax = _run_apply_properties_on_figure_with_curve(cls.curve_view_mock)
        cls.new_curve = cls.ax.containers[0]

        # Mock images tab view
        cls.img_view_mock = Mock(get_selected_image_name=lambda: "(0, 0) - child0", get_properties=lambda: ImageProperties(new_image_props))
        cls.img_view_patch = patch(IMAGE_VIEW, lambda x: cls.img_view_mock)
        cls.img_view_patch.start()

        cls.img_ax = _run_apply_properties_on_figure_with_image()
        cls.new_img = cls.img_ax.images[0]

        # Mock legend tab view
        cls.legend_view_mock = Mock(get_properties=lambda: LegendProperties(new_legend_props))
        cls.legend_view_patch = patch(LEGEND_VIEW, lambda x: cls.legend_view_mock)
        cls.legend_view_patch.start()

        cls.legend_ax = _run_apply_properties_on_figure_with_legend(cls.curve_view_mock)
        cls.new_legend = cls.legend_ax.get_legend()

    @classmethod
    def tearDownClass(cls):
        cls.ax_view_patch.stop()
        cls.curve_view_patch.stop()
        cls.img_view_patch.stop()
        cls.legend_view_patch.stop()

    def test_apply_properties_on_figure_with_image_sets_label(self):
        self.assertEqual(new_image_props["label"], self.new_img.colorbar.ax.get_ylabel())

    def test_apply_properties_on_figure_with_image_sets_colormap(self):
        self.assertEqual(new_image_props["colormap"], self.new_img.cmap.name)

    def test_apply_properties_on_figure_with_image_sets_vmin(self):
        self.assertEqual(new_image_props["vmin"], self.new_img.norm.vmin)

    def test_apply_properties_on_figure_with_image_sets_vmax(self):
        self.assertEqual(new_image_props["vmax"], self.new_img.norm.vmax)

    def test_apply_properties_on_figure_with_image_sets_interpolation(self):
        self.assertEqual(new_image_props["interpolation"].lower(), self.new_img.get_interpolation())

    def test_apply_properties_on_figure_with_image_sets_scale(self):
        self.assertTrue(isinstance(self.new_img.norm, LogNorm))

    def test_apply_properties_on_figure_with_curve_sets_title(self):
        self.assertEqual(new_ax_view_props["title"], self.ax.get_title())

    def test_apply_properties_on_figure_with_curve_sets_xlim(self):
        expected_lims = new_ax_view_props["xlim"]
        for i, lim in enumerate(expected_lims):
            self.assertAlmostEqual(lim, self.ax.get_xlim()[i])

    def test_apply_properties_on_figure_with_curve_sets_xlabel(self):
        self.assertEqual(new_ax_view_props["xlabel"], self.ax.get_xlabel())

    def test_apply_properties_on_figure_with_curve_sets_xscale(self):
        self.assertEqual(new_ax_view_props["xscale"], self.ax.get_xscale())

    def test_apply_properties_on_figure_with_curve_sets_ylim(self):
        expected_lims = new_ax_view_props["ylim"]
        for i, lim in enumerate(expected_lims):
            self.assertAlmostEqual(lim, self.ax.get_ylim()[i])

    def test_apply_properties_on_figure_with_curve_sets_ylabel(self):
        self.assertEqual(new_ax_view_props["ylabel"], self.ax.get_ylabel())

    def test_apply_properties_on_figure_with_curve_sets_yscale(self):
        self.assertEqual(new_ax_view_props["yscale"], self.ax.get_yscale())

    def test_apply_properties_on_figure_with_curve_sets_curve_label(self):
        self.assertEqual(new_curve_view_props["label"], self.new_curve.get_label())

    def test_apply_properties_on_figure_with_curve_sets_curve_visibility(self):
        self.assertTrue(self.new_curve[0].get_visible())

    def test_apply_properties_on_figure_with_curve_sets_line_style(self):
        self.assertEqual(":", self.new_curve[0].get_linestyle())

    def test_apply_properties_on_figure_with_curve_sets_draw_style(self):
        self.assertEqual(new_curve_view_props["drawstyle"], self.new_curve[0].get_drawstyle())

    def test_apply_properties_on_figure_with_curve_sets_line_width(self):
        self.assertEqual(new_curve_view_props["linewidth"], self.new_curve[0].get_linewidth())

    def test_apply_properties_on_figure_with_curve_sets_color(self):
        self.assertEqual(new_curve_view_props["color"], self.new_curve[0].get_color())

    def test_apply_properties_on_figure_with_curve_sets_marker(self):
        self.assertEqual(new_curve_view_props["marker"], self.new_curve[0].get_marker())

    def test_apply_properties_on_figure_with_curve_sets_marker_face_color(self):
        self.assertEqual(new_curve_view_props["markerfacecolor"], self.new_curve[0].get_markerfacecolor())

    def test_apply_properties_on_figure_with_curve_sets_marker_edge_color(self):
        self.assertEqual(new_curve_view_props["markeredgecolor"], self.new_curve[0].get_markeredgecolor())

    def test_apply_properties_on_figure_with_curve_sets_errorbar_visibility(self):
        self.assertEqual(
            new_curve_view_props["hide_errors"], not any([err_set.get_visible() for err_sets in self.new_curve[1:] for err_set in err_sets])
        )

    def test_apply_properties_on_figure_with_curve_sets_cap_size(self):
        self.assertEqual(new_curve_view_props["capsize"], self.new_curve[1][0].get_markersize() / 2)

    def test_apply_properties_on_figure_with_curve_sets_cap_thickness(self):
        self.assertEqual(new_curve_view_props["capthick"], self.new_curve[1][0].get_markeredgewidth())

    def test_apply_properties_on_figure_with_curve_sets_errorbar_color(self):
        cap_color = self.new_curve[1][0].get_color()
        bar_color = self.new_curve[2][0].get_color()
        if not isinstance(cap_color, str) or not cap_color.startswith("#"):
            cap_color = convert_color_to_hex(cap_color[0])
        if not isinstance(bar_color, str) or not bar_color.startswith("#"):
            bar_color = convert_color_to_hex(bar_color[0])
        self.assertEqual(new_curve_view_props["ecolor"], cap_color)
        self.assertEqual(new_curve_view_props["ecolor"], bar_color)

    def test_apply_properties_on_figure_with_curve_sets_errorbar_line_width(self):
        self.assertEqual(new_curve_view_props["elinewidth"], self.new_curve[2][0].get_linewidth()[0])

    def test_apply_properties_on_figure_with_legend_sets_visible(self):
        self.assertEqual(new_legend_props["visible"], self.new_legend.get_visible())

    def test_apply_properties_on_figure_with_legend_sets_title(self):
        self.assertEqual(new_legend_props["title"], self.new_legend.get_title().get_text())

    def test_apply_properties_on_figure_with_legend_sets_background_color(self):
        if int(matplotlib.__version__[0]) >= 2:
            self.assertEqual(new_legend_props["background_color"], convert_color_to_hex(self.new_legend.get_frame().get_facecolor()))
        else:
            self.assertEqual("#ffffff", convert_color_to_hex(self.new_legend.get_frame().get_facecolor()))

    def test_apply_properties_on_figure_with_legend_sets_edge_color(self):
        if int(matplotlib.__version__[0]) >= 2:
            self.assertEqual(new_legend_props["edge_color"], convert_color_to_hex(self.new_legend.get_frame().get_edgecolor()))
        else:
            self.assertEqual("#000000", convert_color_to_hex(self.new_legend.get_frame().get_edgecolor()))

    def test_apply_properties_on_figure_with_legend_sets_transparency(self):
        self.assertEqual(new_legend_props["transparency"], self.new_legend.get_frame().get_alpha())

    def test_apply_properties_on_figure_with_legend_sets_entries_font(self):
        self.assertTrue(self.new_legend.get_texts()[0].get_fontname().endswith("Sans"))

    def test_apply_properties_on_figure_with_legend_sets_entries_size(self):
        self.assertEqual(new_legend_props["entries_size"], self.new_legend.get_texts()[0].get_fontsize())

    def test_apply_properties_on_figure_with_legend_sets_entries_color(self):
        self.assertEqual(new_legend_props["entries_color"], self.new_legend.get_texts()[0].get_color())

    def test_apply_properties_on_figure_with_legend_sets_title_font(self):
        self.assertTrue(self.new_legend.get_title().get_fontname().endswith("Sans"))

    def test_apply_properties_on_figure_with_legend_sets_title_size(self):
        self.assertEqual(new_legend_props["title_size"], self.new_legend.get_title().get_fontsize())

    def test_apply_properties_on_figure_with_legend_sets_title_color(self):
        self.assertEqual(new_legend_props["title_color"], self.new_legend.get_title().get_color())

    def test_apply_properties_on_figure_with_legend_sets_marker_size(self):
        self.assertEqual(new_legend_props["marker_size"], self.new_legend.handlelength)

    def test_apply_properties_on_figure_with_legend_sets_box_visible(self):
        self.assertEqual(new_legend_props["box_visible"], self.new_legend.get_frame().get_visible())

    def test_apply_properties_on_figure_with_legend_sets_shadow(self):
        self.assertEqual(new_legend_props["shadow"], self.new_legend.shadow)

    def test_apply_properties_on_figure_with_legend_sets_round_edges(self):
        self.assertEqual(new_legend_props["round_edges"], isinstance(self.new_legend.legendPatch.get_boxstyle(), BoxStyle.Round))

    def test_apply_properties_on_figure_with_legend_sets_number_of_columns(self):
        self.assertEqual(new_legend_props["columns"], self.new_legend._ncols)

    def test_apply_properties_on_figure_with_legend_sets_column_spacing(self):
        self.assertEqual(new_legend_props["column_spacing"], self.new_legend.columnspacing)

    def test_apply_properties_on_figure_with_legend_sets_label_spacing(self):
        self.assertEqual(new_legend_props["label_spacing"], self.new_legend.labelspacing)

    def test_apply_properties_on_figure_with_legend_sets_marker_position(self):
        align = self.new_legend._legend_handle_box.get_children()[0].align
        position = "Left of Entries" if align == "baseline" else "Right of Entries"
        self.assertEqual(new_legend_props["marker_position"], position)

    def test_apply_properties_on_figure_with_legend_sets_number_of_markers(self):
        self.assertEqual(new_legend_props["markers"], self.new_legend.numpoints)

    def test_apply_properties_on_figure_with_legend_sets_border_padding(self):
        self.assertEqual(new_legend_props["border_padding"], self.new_legend.borderpad)

    def test_apply_properties_on_figure_with_legend_sets_marker_label_padding(self):
        self.assertEqual(new_legend_props["marker_label_padding"], self.new_legend.handletextpad)

    def test_apply_properties_on_figure_sets_minor_ticks(self):
        self.assertEqual(new_ax_view_props["minor_ticks"], not isinstance(self.ax.xaxis.minor.locator, NullLocator))
        self.assertEqual(new_ax_view_props["minor_ticks"], not isinstance(self.ax.yaxis.minor.locator, NullLocator))

    def test_apply_properties_on_figure_sets_minor_gridlines(self):
        self.assertEqual(new_ax_view_props["minor_gridlines"], self.ax.show_minor_gridlines)


if __name__ == "__main__":
    unittest.main()
