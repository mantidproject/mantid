# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from matplotlib import use as mpl_use

mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure

from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog import curvestabwidget as funcs


class CurvePropertiesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.props_dict = {
            'label': 'ax0',
            'linestyle': '-.',
            'linewidth': 4,
            'drawstyle': 'steps',
            'color': '#ff0000',
            'marker': 'v',
            'markersize': 10,
            'markeredgecolor': 'g',
            'markeredgewidth': 0.4,
            'markerfacecolor': 'k',
            'visible': False}

        fig0 = figure()
        ax0 = fig0.add_subplot(211)
        ax0.plot([0, 1, 2], [0, 1, 2], **cls.props_dict)
        ax1 = fig0.add_subplot(212)
        ax1.errorbar([0, 2, 4], [0, 2, 4], xerr=[0, 0.1, 0.2],
                     yerr=[0, 0.1, 0.2], fmt='none', label='ax1')
        ax1.containers[0][2][0].axes.creation_args = [{'errorevery': 1}]
        cls.props = CurveProperties.from_curve(ax0.get_lines()[0])
        cls.error_props = CurveProperties.from_curve(ax1.containers[0])

    def _create_artist(self, errors=False):
        fig = figure()
        ax = fig.add_subplot(111)
        if errors:
            artist = ax.errorbar([0, 1], [0, 1], yerr=[0.1, 0.1])
        else:
            artist = ax.plot([0, 1], [0, 1])[0]
        return artist

    def test_label_set(self):
        self.assertEqual(self.props_dict['label'], self.props.label)

    def test_hide_curve_set(self):
        self.assertEqual(True, self.props.hide)

    def test_line_style_set(self):
        self.assertEqual('dashdot', self.props.linestyle)

    def test_line_width_set(self):
        self.assertEqual(4, self.props.linewidth)

    def test_draw_style_set(self):
        self.assertEqual('steps', self.props.drawstyle)

    def test_line_color_set(self):
        self.assertEqual('#ff0000', self.props.color)

    def test_marker_style_set(self):
        self.assertEqual('triangle_down', self.props.marker)

    def test_marker_size_set(self):
        self.assertEqual(self.props_dict['markersize'], self.props.markersize)

    def test_marker_face_color_set(self):
        self.assertEqual('#000000', self.props.markerfacecolor)

    def test_marker_edge_color_set(self):
        self.assertEqual('#008000', self.props.markeredgecolor)

    def test_label_set_errorbar(self):
        self.assertEqual('ax1', self.error_props.label)

    def test_get_plot_kwargs(self):
        expected_dict = {'capsize': 0.0,
                         'capthick': 1.0,
                         'color': '#ff0000',
                         'drawstyle': u'steps',
                         'ecolor': '#ff0000',
                         'elinewidth': 1.0,
                         'errorevery': 1,
                         'label': 'ax0',
                         'linestyle': 'dashdot',
                         'linewidth': 4.0,
                         'marker': 'v',
                         'markeredgecolor': '#008000',
                         'markerfacecolor': '#000000',
                         'markersize': 10.0,
                         'visible': False}
        self.assertEqual(expected_dict, self.props.get_plot_kwargs())

    def test_curve_hidden_on_line2d(self):
        line2d = self._create_artist(errors=False)
        self.assertFalse(funcs.curve_hidden(line2d))
        line2d.set_visible(False)
        self.assertTrue(funcs.curve_hidden(line2d))

    def test_curve_hidden_on_errorbar_container(self):
        container = self._create_artist(errors=True)
        self.assertFalse(funcs.curve_hidden(container))
        # Hide errorbars but not connecting line
        [caps.set_visible(False) for caps in container[1] if container[1]]
        [bars.set_visible(False) for bars in container[2]]
        self.assertFalse(funcs.curve_hidden(container))
        # Now hide connecting line
        container[0].set_visible(False)
        self.assertTrue(funcs.curve_hidden(container))
