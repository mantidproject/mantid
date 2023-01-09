# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import matplotlib
import unittest

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.project.plotsloader import PlotsLoader
from mantidqt.project.plotssaver import PlotsSaver
from matplotlib.colors import Normalize, LogNorm

matplotlib.use("AGG")


class PlotsSaverTest(unittest.TestCase):
    def setUp(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.plots_loader = PlotsLoader()

        # Make a figure with a given input with all these values already set
        self.loader_plot_dict = {
            "axes": [
                {
                    "colorbar": {"exists": False},
                    "legend": {
                        "exists": True,
                        "visible": True,
                        "title": "Legend",
                        "background_color": "#ffffff",
                        "edge_color": "#000000",
                        "transparency": 0.5,
                        "entries_font": "DejaVu Sans",
                        "entries_size": 10.0,
                        "entries_color": "#000000",
                        "title_font": "DejaVu Sans",
                        "title_size": 12.0,
                        "title_color": "#000000",
                        "marker_size": 2.0,
                        "box_visible": True,
                        "shadow": False,
                        "round_edges": True,
                        "columns": 1,
                        "column_spacing": 0.5,
                        "label_spacing": 0.5,
                        "marker_position": "Left of Entries",
                        "markers": 1,
                        "border_padding": 0.5,
                        "marker_label_padding": 1.0,
                    },
                    "lines": [
                        {
                            "alpha": 1,
                            "color": "#1f77b4",
                            "label": "ws1: spec 2",
                            "lineIndex": 0,
                            "lineStyle": "-",
                            "lineWidth": 1.5,
                            "markerStyle": {
                                "edgeColor": "#1f77b4",
                                "edgeWidth": 1.0,
                                "faceColor": "#1f77b4",
                                "markerSize": 6.0,
                                "markerType": "None",
                                "zOrder": 2,
                            },
                            "errorbars": {"exists": False},
                        }
                    ],
                    "properties": {
                        "axisOn": True,
                        "bounds": (0.0, 0.0, 0.0, 0.0),
                        "dynamic": True,
                        "frameOn": True,
                        "visible": True,
                        "xAxisProperties": {
                            "fontSize": 10.0,
                            "gridStyle": {"gridOn": False},
                            "majorTickFormat": None,
                            "majorTickFormatter": "ScalarFormatter",
                            "majorTickLocator": "AutoLocator",
                            "majorTickLocatorValues": None,
                            "minorTickFormat": None,
                            "minorTickFormatter": "NullFormatter",
                            "minorTickLocator": "NullLocator",
                            "minorTickLocatorValues": None,
                            "visible": True,
                        },
                        "xAxisScale": "linear",
                        "xLim": (0.0, 1.0),
                        "xAutoScale": False,
                        "yAxisProperties": {
                            "fontSize": 10.0,
                            "gridStyle": {"gridOn": False},
                            "majorTickFormat": None,
                            "majorTickFormatter": "ScalarFormatter",
                            "majorTickLocator": "AutoLocator",
                            "majorTickLocatorValues": None,
                            "minorTickFormat": None,
                            "minorTickFormatter": "NullFormatter",
                            "minorTickLocator": "NullLocator",
                            "minorTickLocatorValues": None,
                            "visible": True,
                        },
                        "yAxisScale": "linear",
                        "yLim": (0.0, 1.0),
                        "yAutoScale": False,
                        "facecolor": (0.0, 0.0, 0.0, 0.0),
                        "showMinorGrid": False,
                        "tickParams": {
                            "xaxis": {
                                "major": {
                                    "bottom": True,
                                    "top": True,
                                    "labelbottom": True,
                                    "labeltop": True,
                                    "direction": "inout",
                                    "width": 1,
                                    "size": 6,
                                },
                                "minor": {
                                    "bottom": True,
                                    "top": True,
                                    "labelbottom": True,
                                    "labeltop": True,
                                    "direction": "inout",
                                    "width": 1,
                                    "size": 3,
                                },
                            },
                            "yaxis": {
                                "major": {
                                    "left": True,
                                    "right": True,
                                    "labelleft": True,
                                    "labelright": True,
                                    "direction": "inout",
                                    "width": 1,
                                    "size": 6,
                                },
                                "minor": {
                                    "left": True,
                                    "right": True,
                                    "labelleft": True,
                                    "labelright": True,
                                    "direction": "inout",
                                    "width": 1,
                                    "size": 3,
                                },
                            },
                        },
                        "spineWidths": {"left": 0.4, "right": 0.4, "bottom": 0.4, "top": 0.4},
                    },
                    "textFromArtists": {},
                    "texts": [
                        {
                            "position": (0, 0),
                            "style": {
                                "alpha": 1,
                                "color": "#000000",
                                "hAlign": "left",
                                "rotation": 0.0,
                                "textSize": 10.0,
                                "vAlign": "baseline",
                                "zOrder": 3,
                            },
                            "text": "text",
                            "useTeX": False,
                        }
                    ],
                    "title": "",
                    "xAxisTitle": "",
                    "yAxisTitle": "",
                }
            ],
            "creationArguments": [[{"workspaces": "ws1", "specNum": 2, "function": "plot"}]],
            "label": "",
            "properties": {"dpi": 100.0, "figHeight": 4.8, "figWidth": 6.4},
        }

        self.fig = self.plots_loader.make_fig(self.loader_plot_dict, create_plot=False)

        self.plot_saver = PlotsSaver()

    def tearDown(self):
        ADS.clear()

    def test_save_plots(self):
        plot_dict = {}
        return_value = self.plot_saver.save_plots(plot_dict)

        self.assertEqual(return_value, [])

    def test_get_dict_from_fig(self):
        self.fig.axes[0].creation_args = [{"specNum": 2, "function": "plot"}]
        return_value = self.plot_saver.get_dict_from_fig(self.fig)

        self.loader_plot_dict["creationArguments"] = [[{"specNum": 2, "function": "plot", "normalize_by_bin_width": True}]]

        self.maxDiff = None
        self.assertDictEqual(return_value, self.loader_plot_dict)

    def test_get_dict_from_axes(self):
        self.plot_saver.figure_creation_args = [{"function": "plot"}]
        return_value = self.plot_saver.get_dict_for_axes(self.fig.axes[0])

        self.loader_plot_dict["axes"][0]["_is_norm"] = True
        expected_value = self.loader_plot_dict["axes"][0]

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_axes_properties(self):
        return_value = self.plot_saver.get_dict_from_axes_properties(self.fig.axes[0])

        expected_value = self.loader_plot_dict["axes"][0]["properties"]

        self.maxDiff = None
        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_tick_properties(self):
        return_value = self.plot_saver.get_dict_from_tick_properties(self.fig.axes[0])

        expected_value = self.loader_plot_dict["axes"][0]["properties"]["tickParams"]

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_spine_widths(self):
        return_value = self.plot_saver.get_dict_from_spine_widths(self.fig.axes[0])

        expected_value = self.loader_plot_dict["axes"][0]["properties"]["spineWidths"]

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

        expected_value = {"dpi": 100.0, "figHeight": 4.8, "figWidth": 6.4}

        self.assertDictEqual(return_value, expected_value)

    def test_get_dict_from_fig_with_Normalize(self):
        self.fig.axes[0].creation_args = [{"specNum": None, "function": "pcolormesh", "norm": Normalize()}]
        return_value = self.plot_saver.get_dict_from_fig(self.fig)
        expected_creation_args = [
            [
                {
                    "specNum": None,
                    "function": "pcolormesh",
                    "norm": {"type": "Normalize", "clip": False, "vmin": None, "vmax": None},
                    "normalize_by_bin_width": True,
                }
            ]
        ]

        self.loader_plot_dict["creationArguments"] = expected_creation_args

        self.maxDiff = None
        self.assertDictEqual(return_value, self.loader_plot_dict)

    def test_get_dict_from_fig_with_LogNorm(self):
        self.fig.axes[0].creation_args = [{"specNum": None, "function": "pcolormesh", "norm": LogNorm()}]
        return_value = self.plot_saver.get_dict_from_fig(self.fig)
        expected_creation_args = [
            [
                {
                    "specNum": None,
                    "function": "pcolormesh",
                    "norm": {"type": "LogNorm", "clip": False, "vmin": None, "vmax": None},
                    "normalize_by_bin_width": True,
                }
            ]
        ]

        self.loader_plot_dict["creationArguments"] = expected_creation_args

        self.maxDiff = None
        self.assertDictEqual(return_value, self.loader_plot_dict)


if __name__ == "__main__":
    unittest.main()
