# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest
import matplotlib.figure
import matplotlib.axes
import matplotlib.lines
import matplotlib.text

from mantidqt.project.plotssaver import PlotsSaver


class PlotsLoaderTest(unittest.TestCase):
    def setUp(self):
        self.plot_saver = PlotsSaver()

    def test_save_plots(self):
        plot_dict = {}
        return_value = self.plot_saver.save_plots(plot_dict)

        self.assertEqual(return_value, [])

    def test_get_dict_from_fig(self):
        fig = matplotlib.figure.Figure()
        return_value = self.plot_saver.get_dict_from_fig(fig)

        expected_value = {u'axes': [], u'creationArguments': [], u'label': u'',
                          u'properties': {u'dpi': 100.0, u'figHeight': 4.8, u'figWidth': 6.4}}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_axes(self):
        fig = matplotlib.figure.Figure()
        axes = matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])
        return_value = self.plot_saver.get_dict_for_axes(axes)

        expected_value = {u'legend': {u'exists': False}, u'lines': [],
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

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_axes_properties(self):
        fig = matplotlib.figure.Figure()
        axes = matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])
        return_value = self.plot_saver.get_dict_from_axes_properties(axes)

        expected_value = {u'axisOn': True, u'bounds': (0.0, 0.0, 0.0, 0.0), u'dynamic': True, u'frameOn': True,
                          u'visible': True,
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
                          u'yAxisScale': u'linear', u'yLim': (0.0, 1.0)}

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_axis_properties(self):
        fig = matplotlib.figure.Figure()
        axes = matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])
        axis = axes.xaxis
        return_value = self.plot_saver.get_dict_from_axis_properties(axis)

        expected_value = {u'fontSize': 10.0, u'gridStyle': {u'gridOn': False}, u'majorTickFormat': None,
                          u'majorTickFormatter': 'ScalarFormatter', u'majorTickLocator': 'AutoLocator',
                          u'majorTickLocatorValues': None, u'minorTickFormat': None,
                          u'minorTickFormatter': 'NullFormatter', u'minorTickLocator': 'NullLocator',
                          u'minorTickLocatorValues': None, u'position': u'Bottom', u'visible': True}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_for_grid_style(self):
        fig = matplotlib.figure.Figure()
        axes = matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])
        axis = axes.xaxis
        return_value = self.plot_saver.get_dict_for_grid_style(axis)

        expected_value = {u'gridOn': False}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_line(self):
        line = matplotlib.lines.Line2D([1, 2, 3], [1, 2, 3])
        return_value = self.plot_saver.get_dict_from_line(line, 0)

        expected_value = {u'alpha': 1, u'color': u'#1f77b4', u'label': u'', u'lineIndex': 0, u'lineStyle': u'-',
                          u'lineWidth': 1.5, u'markerStyle': {u'edgeColor': u'#1f77b4', u'edgeWidth': 1.0,
                                                              u'faceColor': u'#1f77b4', u'markerSize': 6.0,
                                                              u'markerType': u'None', u'zOrder': 2}}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_marker_style(self):
        line = matplotlib.lines.Line2D([1, 2, 3], [1, 2, 3])
        return_value = self.plot_saver.get_dict_from_marker_style(line)

        expected_value = {u'edgeColor': u'#1f77b4', u'edgeWidth': 1.0, u'faceColor': u'#1f77b4', u'markerSize': 6.0,
                          u'markerType': u'None', u'zOrder': 2}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_text_style(self):
        text = matplotlib.text.Text(text="test")
        return_value = self.plot_saver.get_dict_from_text(text)

        expected_value = {u'position': (0, 0), u'style': {u'alpha': 1, u'color': u'#000000', u'hAlign': u'left',
                                                          u'mAlign': None, u'rotation': 0.0, u'textSize': 10.0,
                                                          u'vAlign': u'baseline', 'zOrder': 3},
                          u'text': u'test', u'useTeX': False}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_fig_properties(self):
        fig = matplotlib.figure.Figure()
        return_value = self.plot_saver.get_dict_from_fig_properties(fig)

        expected_value = {u'dpi': 100.0, u'figHeight': 4.8, u'figWidth': 6.4}

        self.assertDictEqual(return_value, expected_value)
