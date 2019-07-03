# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.colors import LogNorm
from matplotlib.pyplot import figure

from mantid.py3compat.mock import Mock, patch
from mantidqt.widgets.plotconfigdialog.colorselector import convert_color_to_hex
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.imagestabwidget import ImageProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog.presenter import PlotConfigDialogPresenter

AX_VIEW = 'mantidqt.widgets.plotconfigdialog.axestabwidget.presenter.AxesTabWidgetView'
CURVE_VIEW = 'mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter.CurvesTabWidgetView'
IMAGE_VIEW = 'mantidqt.widgets.plotconfigdialog.imagestabwidget.presenter.ImagesTabWidgetView'

new_ax_view_props = {
    'title': 'New Title',
    'xlim': [0.1, 10],
    'xlabel': 'New X Label',
    'xscale': 'log',
    'ylim': [0.2, 3],
    'ylabel': 'New Y Label',
    'yscale': 'log'}

new_curve_view_props = {
    'label': 'New label',
    'hide': False,
    'linestyle': 'dotted',
    'drawstyle': 'steps',
    'linewidth': 0.5,
    'color': '#ff0000',
    'marker': '_',
    'markerfacecolor': '#ff0001',
    'markeredgecolor': '#ff0002',
    'hide_errors': True,
    'errorevery': 2,
    'capsize': 3,
    'capthick': 4,
    'ecolor': '#ff6550',
    'elinewidth': 5}

new_image_props = {
    'label': 'new label',
    'colormap': 'jet',
    'vmin': 1,
    'vmax': 5,
    'interpolation': 'hanning',
    'scale': 'Logarithmic'}


def _run_apply_properties_on_figure_with_curve():
    fig = figure()
    ax = fig.add_subplot(111)
    ax.errorbar([0, 1], [0, 1], yerr=[0.1, 0.2], label='old label')
    mock_dialog_view = Mock()
    presenter = PlotConfigDialogPresenter(fig, view=mock_dialog_view)
    presenter.apply_properties()
    return ax


def _run_apply_properties_on_figure_with_image():
    img_fig = figure()
    img_ax = img_fig.add_subplot(111)
    img_ax.imshow([[0, 1], [0, 1]], label='old label')
    mock_dialog_view = Mock()
    presenter = PlotConfigDialogPresenter(img_fig, view=mock_dialog_view)
    presenter.apply_properties()
    return img_ax


class ApplyAllPropertiesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Mock axes tab view
        mock_axes_view = Mock(
            get_selected_ax_name=lambda: '(0, 0)',
            get_properties=lambda: AxProperties(new_ax_view_props))
        cls.ax_view_patch = patch(AX_VIEW, lambda x: mock_axes_view)
        cls.ax_view_mock = cls.ax_view_patch.start()

        # Mock curves tab view
        cls.curve_view_mock = Mock(
            get_selected_curve_name=lambda: 'old label',
            get_selected_ax_name=lambda: '(0, 0)',
            get_properties=lambda: CurveProperties(new_curve_view_props))
        cls.curve_view_patch = patch(CURVE_VIEW, lambda x: cls.curve_view_mock)
        cls.curve_view_patch.start()

        cls.ax = _run_apply_properties_on_figure_with_curve()
        cls.new_curve = cls.ax.containers[0]

        # Mock images tab view
        cls.img_view_mock = Mock(
            get_selected_image_name=lambda: '(0, 0) - old label',
            get_properties=lambda: ImageProperties(new_image_props))
        cls.img_view_patch = patch(IMAGE_VIEW, lambda x: cls.img_view_mock)
        cls.img_view_patch.start()

        cls.img_ax = _run_apply_properties_on_figure_with_image()
        cls.new_img = cls.img_ax.images[0]

    @classmethod
    def tearDownClass(cls):
        cls.ax_view_patch.stop()
        cls.curve_view_patch.stop()
        cls.img_view_patch.stop()

    def test_apply_properties_on_figure_with_image_sets_label(self):
        self.assertEqual(new_image_props['label'], self.new_img.get_label())

    def test_apply_properties_on_figure_with_image_sets_colormap(self):
        self.assertEqual(new_image_props['colormap'], self.new_img.cmap.name)

    def test_apply_properties_on_figure_with_image_sets_vmin(self):
        self.assertEqual(new_image_props['vmin'], self.new_img.norm.vmin)

    def test_apply_properties_on_figure_with_image_sets_vmax(self):
        self.assertEqual(new_image_props['vmax'], self.new_img.norm.vmax)

    def test_apply_properties_on_figure_with_image_sets_interpolation(self):
        self.assertEqual(new_image_props['interpolation'].lower(),
                         self.new_img.get_interpolation())

    def test_apply_properties_on_figure_with_image_sets_scale(self):
        self.assertTrue(isinstance(self.new_img.norm, LogNorm))

    def test_apply_properties_on_figure_with_curve_sets_title(self):
        self.assertEqual(new_ax_view_props['title'], self.ax.get_title())

    def test_apply_properties_on_figure_with_curve_sets_xlim(self):
        expected_lims = new_ax_view_props['xlim']
        for i, lim in enumerate(expected_lims):
            self.assertAlmostEqual(lim, self.ax.get_xlim()[i])

    def test_apply_properties_on_figure_with_curve_sets_xlabel(self):
        self.assertEqual(new_ax_view_props['xlabel'], self.ax.get_xlabel())

    def test_apply_properties_on_figure_with_curve_sets_xscale(self):
        self.assertEqual(new_ax_view_props['xscale'], self.ax.get_xscale())

    def test_apply_properties_on_figure_with_curve_sets_ylim(self):
        expected_lims = new_ax_view_props['ylim']
        for i, lim in enumerate(expected_lims):
            self.assertAlmostEqual(lim, self.ax.get_ylim()[i])

    def test_apply_properties_on_figure_with_curve_sets_ylabel(self):
        self.assertEqual(new_ax_view_props['ylabel'], self.ax.get_ylabel())

    def test_apply_properties_on_figure_with_curve_sets_yscale(self):
        self.assertEqual(new_ax_view_props['yscale'], self.ax.get_yscale())

    def test_apply_properties_on_figure_with_curve_sets_curve_label(self):
        self.assertEqual(new_curve_view_props['label'], self.new_curve.get_label())

    def test_apply_properties_on_figure_with_curve_sets_curve_visibility(self):
        self.assertTrue(self.new_curve[0].get_visible())

    def test_apply_properties_on_figure_with_curve_sets_line_style(self):
        self.assertEqual(':', self.new_curve[0].get_linestyle())

    def test_apply_properties_on_figure_with_curve_sets_draw_style(self):
        self.assertEqual(new_curve_view_props['drawstyle'],
                         self.new_curve[0].get_drawstyle())

    def test_apply_properties_on_figure_with_curve_sets_line_width(self):
        self.assertEqual(new_curve_view_props['linewidth'],
                         self.new_curve[0].get_linewidth())

    def test_apply_properties_on_figure_with_curve_sets_color(self):
        self.assertEqual(new_curve_view_props['color'],
                         self.new_curve[0].get_color())

    def test_apply_properties_on_figure_with_curve_sets_marker(self):
        self.assertEqual(new_curve_view_props['marker'],
                         self.new_curve[0].get_marker())

    def test_apply_properties_on_figure_with_curve_sets_marker_face_color(self):
        self.assertEqual(new_curve_view_props['markerfacecolor'],
                         self.new_curve[0].get_markerfacecolor())

    def test_apply_properties_on_figure_with_curve_sets_marker_edge_color(self):
        self.assertEqual(new_curve_view_props['markeredgecolor'],
                         self.new_curve[0].get_markeredgecolor())

    def test_apply_properties_on_figure_with_curve_sets_errorbar_visibility(self):
        self.assertEqual(new_curve_view_props['hide_errors'],
                         not any([err_set.get_visible()
                                  for err_sets in self.new_curve[1:]
                                  for err_set in err_sets]))

    def test_apply_properties_on_figure_with_curve_sets_error_every(self):
        self.assertEqual(1, len(self.new_curve[2][0].get_segments()))
        self.assertEqual(new_curve_view_props['errorevery'],
                         self.new_curve.errorevery)

    def test_apply_properties_on_figure_with_curve_sets_cap_size(self):
        self.assertEqual(new_curve_view_props['capsize'],
                         self.new_curve[1][0].get_markersize()/2)

    def test_apply_properties_on_figure_with_curve_sets_cap_thickness(self):
        self.assertEqual(new_curve_view_props['capthick'],
                         self.new_curve[1][0].get_markeredgewidth())

    def test_apply_properties_on_figure_with_curve_sets_errorbar_color(self):
        cap_color = self.new_curve[1][0].get_color()
        bar_color = self.new_curve[2][0].get_color()
        if cap_color[0] != '#':
            cap_color = convert_color_to_hex(cap_color[0])
        if bar_color[0] != '#':
            bar_color = convert_color_to_hex(bar_color[0])
        self.assertEqual(new_curve_view_props['ecolor'], cap_color)
        self.assertEqual(new_curve_view_props['ecolor'], bar_color)

    def test_apply_properties_on_figure_with_curve_sets_errorbar_line_width(self):
        self.assertEqual(new_curve_view_props['elinewidth'],
                         self.new_curve[2][0].get_linewidth()[0])


if __name__ == '__main__':
    unittest.main()
