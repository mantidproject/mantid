# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import matplotlib
matplotlib.use('AGG')

import unittest  # noqa
import matplotlib.pyplot as plt  # noqa
import matplotlib.figure  # noqa
import matplotlib.text  # noqa

from mantidqt.project.plotsloader import PlotsLoader  # noqa
import mantid.plots.plotfunctions  # noqa
from mantid.api import AnalysisDataService as ADS  # noqa
from mantid.dataobjects import Workspace2D  # noqa
from mantid.py3compat import mock  # noqa


def pass_func():
    pass


class PlotsLoaderTest(unittest.TestCase):
    def setUp(self):
        self.plots_loader = PlotsLoader()
        plt.plot = mock.MagicMock()
        mantid.plots.plotfunctions.plot = mock.MagicMock()
        self.dictionary = {u'legend': {u'exists': False}, u'lines': [],
                           u'properties': {u'axisOn': True, u'bounds': (0.0, 0.0, 0.0, 0.0), u'dynamic': True,
                                           u'frameOn': True, u'visible': True,
                                           u'xAxisProperties': {u'fontSize': 10.0,
                                                                u'gridStyle': {u'gridOn': False},
                                                                u'majorTickFormat': None,
                                                                u'majorTickFormatter': 'ScalarFormatter',
                                                                u'majorTickLocator': 'AutoLocator',
                                                                u'majorTickLocatorValues': None,
                                                                u'minorTickFormat': None,
                                                                u'minorTickFormatter': 'NullFormatter',
                                                                u'minorTickLocator': 'NullLocator',
                                                                u'minorTickLocatorValues': None,
                                                                u'position': u'Bottom',
                                                                u'visible': True},
                                           u'xAxisScale': u'linear', u'xLim': (0.0, 1.0),
                                           u'yAxisProperties': {u'fontSize': 10.0,
                                                                u'gridStyle': {u'gridOn': False},
                                                                u'majorTickFormat': None,
                                                                u'majorTickFormatter': 'ScalarFormatter',
                                                                u'majorTickLocator': 'AutoLocator',
                                                                u'majorTickLocatorValues': None,
                                                                u'minorTickFormat': None,
                                                                u'minorTickFormatter': 'NullFormatter',
                                                                u'minorTickLocator': 'NullLocator',
                                                                u'minorTickLocatorValues': None,
                                                                u'position': u'Left',
                                                                u'visible': True},
                                           u'yAxisScale': u'linear', u'yLim': (0.0, 1.0)},
                           u'textFromArtists': {}, u'texts': [], u'title': u'', u'xAxisTitle': u'', u'yAxisTitle': u''}

    def test_load_plots_does_the_right_calls(self):
        self.plots_loader.make_fig = mock.MagicMock()
        self.plots_loader.load_plots(["plot1", "plot2"])

        self.assertEqual(self.plots_loader.make_fig.call_count, 2)

    @mock.patch("matplotlib.figure.Figure.show")
    def test_make_fig_makes_the_right_calls(self, pass_func):
        ws = Workspace2D()
        ADS.add("ws", ws)
        plot_dict = {"label": "plot", "creationArguments": [[{"workspaces": "ws", "wkspIndex": 0}, {}, {}]]}
        self.plots_loader.plot_func = mock.MagicMock()
        self.plots_loader.restore_figure_data = mock.MagicMock()

        self.plots_loader.make_fig(plot_dict)

        self.assertEqual(self.plots_loader.plot_func.call_count, 1)
        self.assertEqual(self.plots_loader.restore_figure_data.call_count, 1)

    def test_restore_fig_properties(self):
        matplotlib.figure.Figure.set_figheight = mock.MagicMock()
        matplotlib.figure.Figure.set_figwidth = mock.MagicMock()
        matplotlib.figure.Figure.set_dpi = mock.MagicMock()
        self.plots_loader.restore_fig_properties(matplotlib.figure.Figure(), {"figHeight": 1, "figWidth": 1, "dpi": 1})

        self.assertEqual(matplotlib.figure.Figure.set_figheight.call_count, 1)
        self.assertEqual(matplotlib.figure.Figure.set_figwidth.call_count, 1)
        self.assertEqual(matplotlib.figure.Figure.set_dpi.call_count, 1)

    def test_restore_fig_axes(self):
        self.plots_loader.update_properties = mock.MagicMock()
        self.plots_loader.update_lines = mock.MagicMock()
        self.plots_loader.create_text_from_dict = mock.MagicMock()
        self.plots_loader.update_legend = mock.MagicMock()

        fig = matplotlib.figure.Figure()
        self.plots_loader.restore_fig_axes(matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0]), self.dictionary)

        self.assertEqual(self.plots_loader.update_properties.call_count, 1)
        self.assertEqual(self.plots_loader.update_lines.call_count, 0)
        self.assertEqual(self.plots_loader.create_text_from_dict.call_count, 0)
        self.assertEqual(self.plots_loader.update_legend.call_count, 1)

    def test_create_text_from_dict(self):
        fig = matplotlib.figure.Figure()
        ax = matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])
        ax.text = mock.MagicMock()

        self.plots_loader.create_text_from_dict(ax=ax, dic={"text": "text", "position": (1, 1), "useTeX": 1,
                                                            "style": {"alpha": 1, "textSize": 1, "color": 1,
                                                                      "hAlign": 1, "vAlign": 1, "rotation": 1,
                                                                      "zOrder": 1}})
        self.assertEqual(ax.text.call_count, 1)
        ax.text.assert_called_once_with(fontdict={u'zorder': 1, u'fontsize': 1, u'color': 1, u'alpha': 1,
                                                  u'rotation': 1, u'verticalalignment': 1, u'usetex': 1,
                                                  u'horizontalalignment': 1}, s=u'text', x=1, y=1)

    @mock.patch("matplotlib.figure.Figure.show")
    def test_load_plot_from_dict(self, pass_func):
        # The fact this runs is the test
        self.plots_loader.load_plots([self.dictionary])
