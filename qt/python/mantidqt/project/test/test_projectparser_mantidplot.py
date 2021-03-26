# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest
from os import path

from mantid import config
from mantidqt.project.projectparser_mantidplot import MantidPlotProjectParser


class ProjectParserMantidPlotTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for directory in config.getDataSearchDirs():
            if 'UnitTest' in directory:
                cls.base_path = path.join(directory, 'project_load', 'Mantidplot')
                break
        cls.project_with_multiple_workspaces = path.join(cls.base_path, "mantidplot_project_multiple_workspaces.mantid")
        cls.project_with_workspace_group = path.join(cls.base_path, "mantidplot_project_workspace_group.mantid")
        cls.project_with_simple_1d_plot = path.join(cls.base_path, "mantidplot_project_simple_plot.mantid")
        cls.project_with_multiple_1d_plots = path.join(cls.base_path, "mantidplot_project_multiple_plots.mantid")
        cls.project_plot_with_multiple_lines = path.join(cls.base_path,
                                                         "mantidplot_project_plot_with_multiple_lines.mantid")
        cls.project_with_tiled_lots = path.join(cls.base_path, "mantidplot_project_tiled_plots.mantid")
        cls.project_with_errorbars = path.join(cls.base_path, "mantidplot_project_errorbar_plots.mantid")
        cls.project_with_logscale = path.join(cls.base_path, "mantidplot_project_logscale_plots.mantid")

    def test_parsing_workspaces(self):
        expected_workspaces = ['BASIS_79827_divided_sqw', 'irs26176_graphite002_red', 'MUSR00062261']
        parser = MantidPlotProjectParser(self.project_with_multiple_workspaces)

        workspace_list = parser.get_workspaces()

        self.assertCountEqual(workspace_list, expected_workspaces)

    def test_parsing_workspace_group(self):
        expected_workspaces = ['MUSR00015189_1', 'MUSR00015189_2', 'MUSR00062260', 'MUSR00062261']
        parser = MantidPlotProjectParser(self.project_with_workspace_group)

        workspace_list = parser.get_workspaces()

        self.assertCountEqual(workspace_list, expected_workspaces)

    def test_parsing_simple_1d_plot(self):
        parser = MantidPlotProjectParser(self.project_with_simple_1d_plot)
        expected_creation_args = {
            'linestyle': 'solid',
            'linewidth': 1.5,
            'drawstyle': 'default',
            'marker': None,
            'markersize': 6,
            'function': 'plot',
            'wkspIndex': 1,
            'workspaces': 'MUSR00062261'
        }

        plot_list = parser.get_plots()
        plot_dict = plot_list[0]
        axes_dict = plot_dict["axes"]
        axes_properties = axes_dict[0]["properties"]
        creation_args = plot_dict["creationArguments"][0][0]

        self.assertEqual(len(plot_list), 1)
        self.assertEqual(len(axes_dict), 1)
        self.assertEqual(plot_dict["label"], "MUSR00062261-1")
        self.assertEqual(axes_dict[0]["title"], "MUSR00062261")
        self.assertEqual(axes_dict[0]["xAxisTitle"], "Time (μs)")
        self.assertEqual(axes_dict[0]["yAxisTitle"], "Counts (μs)⁻¹")
        self.assertEqual(axes_dict[0]["title"], "MUSR00062261")
        self.assertEqual(axes_properties["xLim"], [-5, 35])
        self.assertEqual(axes_properties["yLim"], [0, 300000.0])
        self.assertEqual(creation_args, expected_creation_args)

    def test_parsing_multiple_1d_plots(self):
        parser = MantidPlotProjectParser(self.project_with_multiple_1d_plots)
        expected_creation_args = {
            'linestyle': 'solid',
            'linewidth': 1.5,
            'drawstyle': 'default',
            'marker': None,
            'markersize': 6,
            'function': 'plot',
            'wkspIndex': 1,
            'workspaces': 'MUSR00062261'
        }

        plot_list = parser.get_plots()
        creation_args_1 = plot_list[0]["creationArguments"][0][0]
        creation_args_2 = plot_list[1]["creationArguments"][0][0]
        creation_args_3 = plot_list[2]["creationArguments"][0][0]

        self.assertEqual(len(plot_list), 3)
        self.assertEqual(creation_args_1, expected_creation_args)
        expected_creation_args["wkspIndex"] = 8
        self.assertEqual(creation_args_2, expected_creation_args)
        expected_creation_args["wkspIndex"] = 32
        self.assertEqual(creation_args_3, expected_creation_args)

    def test_parsing_tiled_plot(self):
        number_of_tiles = 4
        parser = MantidPlotProjectParser(self.project_with_tiled_lots)

        plot_list = parser.get_plots()
        plot_dict = plot_list[0]
        creation_args = plot_dict["creationArguments"]
        axes_args = plot_dict["axes"]
        index_list = [creation_args[i][0]["wkspIndex"] for i in range(len(creation_args))]

        self.assertEqual(len(plot_list), 1)
        self.assertEqual(len(axes_args), number_of_tiles)
        self.assertEqual(len(creation_args), number_of_tiles)
        self.assertEqual(index_list, [0, 4, 18, 26])

    def test_parsing_plot_with_multiple_lines(self):
        number_of_lines = 4
        parser = MantidPlotProjectParser(self.project_plot_with_multiple_lines)

        plot_list = parser.get_plots()
        plot_dict = plot_list[0]
        creation_args = plot_dict["creationArguments"][0]
        workspace_list = [creation_args[i]["workspaces"] for i in range(len(creation_args))]
        index_list = [creation_args[i]["wkspIndex"] for i in range(len(creation_args))]

        self.assertEqual(len(plot_list), 1)
        self.assertEqual(len(creation_args), number_of_lines)
        self.assertEqual(workspace_list, ["MUSR00062261"] * number_of_lines)
        self.assertEqual(index_list, [0, 1, 2, 3])

    def test_axes_scale_read_read_correctly_from_project_file(self):
        parser = MantidPlotProjectParser(self.project_with_logscale)

        plot_list = parser.get_plots()
        plot_dict = plot_list[0]
        axes_dict = plot_dict["axes"]
        axes_properties = axes_dict[0]["properties"]

        self.assertEqual(axes_properties["yAxisScale"], 'linear')
        self.assertEqual(axes_properties["xAxisScale"], 'log')

    def test_error_bars_correctly_loaded_from_project_file(self):
        parser = MantidPlotProjectParser(self.project_with_errorbars)

        plot_list = parser.get_plots()
        plot_dict = plot_list[0]
        creation_args = plot_dict["creationArguments"][0][0]

        self.assertEqual(creation_args["function"], "errorbar")


if __name__ == "__main__":
    unittest.main()
