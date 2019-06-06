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
from matplotlib.pyplot import figure

from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget import MarkerProperties


class MarkerPropertiesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fig = figure()
        cls.ax = fig.add_subplot(111)

    def test_marker_properties_read_correctly_from_Line2D_object(self):
        plot_args = {'marker': 'v', 'mew': 0.1, 'mec': 'k', 'mfc': 'r',
                     'ms': 1.1}
        expected = {'style': 'triangle_down', 'size': 1.1,
                    'face_color': '#ff0000', 'edge_color': '#000000'}
        curve = self.ax.plot([0, 1], [0, 1], **plot_args)[0]
        line_props = MarkerProperties.from_line(curve)
        for prop, value in expected.items():
            self.assertEqual(value, getattr(line_props, prop))

    def test_returns_none_for_non_Line2D_object(self):
        self.assertEqual(None, MarkerProperties.from_line([]))


if __name__ == '__main__':
    unittest.main()
