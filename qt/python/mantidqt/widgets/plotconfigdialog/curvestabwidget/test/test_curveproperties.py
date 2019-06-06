# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure

from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties


class CurvePropertiesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.props_dict = {
            'label': 'ax0',
            'linestyle': '-.',
            'linewidth': 4,
            'drawstyle': 'steps',
            'color': 'r',
            'marker': 'v',
            'markersize': 10,
            'markeredgecolor': 'g',
            'markeredgewidth': 0.4,
            'markerfacecolor': 'k',
            'visible': False
        }

        fig0 = figure()
        ax0 = fig0.add_subplot(211)
        ax0.plot([0, 1, 2], [0, 1, 2], **cls.props_dict)
        ax1 = fig0.add_subplot(212)
        ax1.errorbar([0, 2, 4], [0, 2, 4], xerr=[0, 0.1, 0.2],
                     yerr=[0, 0.1, 0.2], fmt='none', label='ax1')
        cls.props = CurveProperties.from_curve(ax0.get_lines()[0])
        cls.error_props = CurveProperties.from_curve(ax1.containers[0])

    def test_label_set(self):
        self.assertEqual(self.props_dict['label'], self.props.label)

    def test_hide_curve_set(self):
        self.assertEqual(True, self.props.hide_curve)

    def test_line_style_set(self):
        self.assertEqual('dashdot', self.props.line.style)

    def test_line_width_set(self):
        self.assertEqual(4, self.props.line.width)

    def test_draw_style_set(self):
        self.assertEqual('steps', self.props.line.draw_style)

    def test_line_color_set(self):
        self.assertEqual('#ff0000', self.props.line.color)

    def test_marker_style_set(self):
        self.assertEqual('triangle_down', self.props.marker.style)

    def test_marker_size_set(self):
        self.assertEqual(self.props_dict['markersize'], self.props.marker.size)

    def test_marker_face_color_set(self):
        self.assertEqual('#000000', self.props.marker.face_color)

    def test_marker_edge_color_set(self):
        self.assertEqual('#008000', self.props.marker.edge_color)

    def test_label_set_errorbar(self):
        self.assertEqual('ax1', self.error_props.label)
