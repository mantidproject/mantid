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

from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget import LineProperties


class LinePropertiesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fig = figure()
        cls.ax = fig.add_subplot(111)

    def test_line_properties_read_correctly(self):
        plot_args = {'linewidth': 1.1, 'linestyle': '--', 'drawstyle': 'steps',
                     'color': 'g'}
        expected = {'width': 1.1, 'style': 'dashed', 'draw_style': 'steps',
                    'color': '#008000'}
        curve = self.ax.plot([0, 1], [0, 1], **plot_args)[0]
        line_props = LineProperties.from_line(curve)
        for prop, value in expected.items():
            self.assertEqual(value, getattr(line_props, prop))

    def test_returns_none_for_non_Line2D_object(self):
        self.assertEqual(None, LineProperties.from_line([]))


if __name__ == '__main__':
    unittest.main()
