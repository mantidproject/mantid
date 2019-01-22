# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest

from mantid.api import AnalysisDataService as ADS
from mantidqt.project.plotssaver import PlotsSaver
from mantidqt.project.plotsloader import PlotsLoader
from mantid.simpleapi import CreateSampleWorkspace

import matplotlib
matplotlib.use('AGG')


class PlotsSaverTest(unittest.TestCase):
    def setUp(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.plots_loader = PlotsLoader()

        # Make a figure with a given input with all these values already set
        self.loader_plot_dict = {u'axes': [{u'colorbar': {u'exists': False},
                                            u'legend': {u'exists': False}, u'lines': [{u'alpha': 1,
                                                                                       u'color': u'#1f77b4',
                                                                                       u'label': u'ws1: spec 2',
                                                                                       u'lineIndex': 0,
                                                                                       u'lineStyle': u'-',
                                                                                       u'lineWidth': 1.5,
                                                                                       u'markerStyle': {
                                                                                           u'edgeColor': u'#1f77b4',
                                                                                           u'edgeWidth': 1.0,
                                                                                           u'faceColor': u'#1f77b4',
                                                                                           u'markerSize': 6.0,
                                                                                           u'markerType': u'None',
                                                                                           u'zOrder': 2},
                                                                                       u'errorbars': {
                                                                                           u'exists': False
                                                                                       }}],
                                 u'properties': {u'axisOn': True, u'bounds': (0.0, 0.0, 0.0, 0.0), u'dynamic': True,
                                                 u'frameOn': True, u'visible': True,
                                                 u'xAxisProperties': {u'fontSize': 10.0,
                                                                      u'gridStyle': {u'gridOn': False},
                                                                      u'majorTickFormat': None,
                                                                      u'majorTickFormatter': u'ScalarFormatter',
                                                                      u'majorTickLocator': u'AutoLocator',
                                                                      u'majorTickLocatorValues': None,
                                                                      u'minorTickFormat': None,
                                                                      u'minorTickFormatter': u'NullFormatter',
                                                                      u'minorTickLocator': u'NullLocator',
                                                                      u'minorTickLocatorValues': None,
                                                                      u'position': u'Bottom',
                                                                      u'visible': True},
                                                 u'xAxisScale': u'linear', u'xLim': (0.0, 1.0),
                                                 u'yAxisProperties': {u'fontSize': 10.0,
                                                                      u'gridStyle': {u'gridOn': False},
                                                                      u'majorTickFormat': None,
                                                                      u'majorTickFormatter': u'ScalarFormatter',
                                                                      u'majorTickLocator': u'AutoLocator',
                                                                      u'majorTickLocatorValues': None,
                                                                      u'minorTickFormat': None,
                                                                      u'minorTickFormatter': u'NullFormatter',
                                                                      u'minorTickLocator': u'NullLocator',
                                                                      u'minorTickLocatorValues': None,
                                                                      u'position': u'Left',
                                                                      u'visible': True},
                                                 u'yAxisScale': u'linear', u'yLim': (0.0, 1.0)},
                                            u'textFromArtists': {}, u'texts': [{u'position': (0, 0),
                                                                                u'style': {u'alpha': 1,
                                                                                           u'color': u'#000000',
                                                                                           u'hAlign': u'left',
                                                                                           u'rotation': 0.0,
                                                                                           u'textSize': 10.0,
                                                                                           u'vAlign': u'baseline',
                                                                                           u'zOrder': 3},
                                                                                u'text': u'text', u'useTeX': False}],
                                            u'title': u'', u'xAxisTitle': u'',
                                            u'yAxisTitle': u''}],
                                 u'creationArguments': [[{u"workspaces": u"ws1", u"specNum": 2, u"function": u"plot"}]],
                                 u'label': u'',
                                 u'properties': {u'dpi': 100.0, u'figHeight': 4.8, u'figWidth': 6.4}}
        self.fig = self.plots_loader.make_fig(self.loader_plot_dict, create_plot=False)

        self.plot_saver = PlotsSaver()

    def tearDown(self):
        ADS.clear()

    def test_save_plots(self):
        plot_dict = {}
        return_value = self.plot_saver.save_plots(plot_dict)

        self.assertEqual(return_value, [])

    def test_get_dict_from_fig(self):
        self.fig.axes[0].creation_args = [{u"specNum": 2, "function": "plot"}]
        return_value = self.plot_saver.get_dict_from_fig(self.fig)

        self.loader_plot_dict[u'creationArguments'] = [[{u"specNum": 2, "function": "plot"}]]

        self.maxDiff = None
        self.assertDictEqual(return_value, self.loader_plot_dict)

    def test_get_dict_from_axes(self):
        self.plot_saver.figure_creation_args = [{"function": "plot"}]
        return_value = self.plot_saver.get_dict_for_axes(self.fig.axes[0])

        expected_value = self.loader_plot_dict["axes"][0]

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_axes_properties(self):
        return_value = self.plot_saver.get_dict_from_axes_properties(self.fig.axes[0])

        expected_value = self.loader_plot_dict["axes"][0]["properties"]

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_axis_properties(self):

        return_value = self.plot_saver.get_dict_from_axis_properties(self.fig.axes[0].xaxis)

        expected_value = self.loader_plot_dict["axes"][0]["properties"]["xAxisProperties"]

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_for_grid_style(self):
        return_value = self.plot_saver.get_dict_for_grid_style(self.fig.axes[0].xaxis)

        expected_value = self.loader_plot_dict["axes"][0]["properties"]["xAxisProperties"]["gridStyle"]

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_line(self):
        self.plot_saver.figure_creation_args = [{"function": "plot"}]
        line = self.fig.axes[0].lines[0]
        return_value = self.plot_saver.get_dict_from_line(line, 0)

        expected_value = self.loader_plot_dict["axes"][0]["lines"][0]

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_marker_style(self):
        line = self.fig.axes[0].lines[0]
        return_value = self.plot_saver.get_dict_from_marker_style(line)

        expected_value = self.loader_plot_dict["axes"][0]["lines"][0]["markerStyle"]

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_text_style(self):
        text = self.fig.axes[0].texts[0]
        return_value = self.plot_saver.get_dict_from_text(text)

        expected_value = self.loader_plot_dict["axes"][0]["texts"][0]

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_fig_properties(self):
        return_value = self.plot_saver.get_dict_from_fig_properties(self.fig)

        expected_value = {u'dpi': 100.0, u'figHeight': 4.8, u'figWidth': 6.4}

        self.assertDictEqual(return_value, expected_value)
