# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib as mpl
mpl.use('Agg')  # noqa
import matplotlib.pyplot as plt
from matplotlib.ticker import LogFormatterSciNotation, ScalarFormatter

from unittest.mock import Mock
from workbench.plotting.plotscriptgenerator.axes import (generate_axis_limit_commands,
                                                         generate_axis_label_commands,
                                                         generate_axis_facecolor_commands)
from workbench.plotting.plotscriptgenerator import generate_script


class PlotGeneratorAxisTest(unittest.TestCase):

    def test_generate_axis_label_commands_only_returns_commands_for_labels_that_are_set(self):
        mock_ax = Mock(get_xlabel=lambda: '', get_ylabel=lambda: 'y')
        expected = ["set_ylabel('y')"]
        self.assertEqual(expected, generate_axis_label_commands(mock_ax))

    def test_generate_axis_label_commands_returns_empty_list_when_no_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: '', get_ylabel=lambda: '')
        self.assertEqual([], generate_axis_label_commands(mock_ax))

    def test_generate_axis_label_commands_returns_x_and_y_command_if_both_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: "X", get_ylabel=lambda: "Y")
        expected = ["set_xlabel('X')", "set_ylabel('Y')"]
        actual = generate_axis_label_commands(mock_ax)
        self.assertEqual(expected, actual)

    def test_generate_axis_label_commands_returns_only_x_command_if_y_label_not_set(self):
        mock_ax = Mock(get_xlabel=lambda: "X", get_ylabel=lambda: "")
        expected = ["set_xlabel('X')"]
        actual = generate_axis_label_commands(mock_ax)
        self.assertEqual(expected, actual)

    def test_generate_axis_label_commands_returns_empty_list_if_no_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: "", get_ylabel=lambda: "")
        actual = generate_axis_label_commands(mock_ax)
        self.assertEqual([], actual)

    def test_generate_axis_limit_commands_returns_empty_list_if_limits_not_changed(self):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot([-10, 10], [1, 2])
        self.assertEqual([], generate_axis_limit_commands(ax))
        plt.close()
        del fig

    def test_generate_axis_limit_commands_returns_x_limit_command_if_x_limit_changed(self):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot([-10, 10], [1, 2])
        ax.set_xlim([-5, 5])
        self.assertEqual(['set_xlim([-5.0, 5.0])'], generate_axis_limit_commands(ax))
        plt.close()
        del fig

    def test_generate_axis_limit_commands_returns_x_and_y_limit_commands_if_limits_changed(self):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot([-10, 10], [1, 2])
        ax.set_xlim([-5, 5])
        ax.set_ylim([0, 4])
        self.assertEqual(['set_xlim([-5.0, 5.0])', 'set_ylim([0.0, 4.0])'],
                         generate_axis_limit_commands(ax))
        plt.close()
        del fig

    def test_generate_axis_facecolor_commands(self):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot([-10, 10], [1, 2])
        ax.set_facecolor("#8a9aff")
        self.assertEqual("set_facecolor('#8a9aff')", generate_axis_facecolor_commands(ax))
        plt.close()
        del fig

    def test_facecolor_command_not_present_if_default_colour(self):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot([-10, 10], [1, 2])
        # We didn't change the facecolor from default, so no commands should be returned.
        self.assertFalse(generate_axis_facecolor_commands(ax))
        plt.close()
        del fig

    def test_facecolor_commands_for_tiled_plot(self):
        """
        Set a canvas colour (facecolor) for only one of the subplots in a tiled plot and check
        that commands are generated for only that one.
        """
        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
        # Only set the facecolor on one subplot.
        axes[1][0].set_facecolor('#c0ffee')
        script = generate_script(fig)
        # Should be axes[i][j].set_facecolor for multiple subplots.
        self.assertNotIn("axes.set_facecolor", script)
        # These subplots have default canvas colour so should've have the facecolor command.
        self.assertNotIn("axes[0][0].set_facecolor", script)
        self.assertNotIn("axes[0][1].set_facecolor", script)
        self.assertNotIn("axes[1][1].set_facecolor", script)
        # This subplot should have our new colour.
        self.assertIn("axes[1][0].set_facecolor('#c0ffee')", script)
        plt.close()
        del fig

    def test_generate_tick_commands_for_tiled_plot(self):
        """
        Check that the tick commands are generated for every plot in the figure.
        """
        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
        script = generate_script(fig)
        # Should be axes[i][j].tick_params for multiple subplots.
        self.assertNotIn("axes.tick_params", script)
        self.assertIn("axes[0][0].tick_params", script)
        self.assertIn("axes[0][1].tick_params", script)
        self.assertIn("axes[1][0].tick_params", script)
        self.assertIn("axes[1][1].tick_params", script)
        plt.close()
        del fig

    def test_generate_tick_format_commands(self):
        """
        Check that the tick format commands are correctly generated if they are set different to the default.
        """
        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])

        # Only change the major formatter for one of the x axes and one of the y axes.
        # The rest will be default, and shouldn't generate any lines in the script.
        axes[0][1].xaxis.set_major_formatter(LogFormatterSciNotation())
        axes[1][0].yaxis.set_major_formatter(LogFormatterSciNotation())

        script = generate_script(fig)
        # Check the import is there exactly once.
        self.assertEqual(script.count("from matplotlib.ticker import"), 1)
        # We only set the major formatter for axes[0][1].xaxis, so the command should only be present there.
        self.assertNotIn("axes[0][0].xaxis.set_major_formatter", script)
        self.assertIn("axes[0][1].xaxis.set_major_formatter", script)
        self.assertNotIn("axes[1][0].xaxis.set_major_formatter", script)
        self.assertNotIn("axes[1][1].xaxis.set_major_formatter", script)
        # We only set the major formatter for axes[1][0].yaxis, so the command should only be present there.
        self.assertNotIn("axes[0][0].yaxis.set_major_formatter", script)
        self.assertNotIn("axes[0][1].yaxis.set_major_formatter", script)
        self.assertIn("axes[1][0].yaxis.set_major_formatter", script)
        self.assertNotIn("axes[1][1].yaxis.set_major_formatter", script)

    def test_generate_tick_format_commands_log_scale(self):
        """
        Check that tick format commands are correctly generated for a log axis scale.
        """
        fig = plt.figure()
        axes = fig.add_subplot(1, 1, 1, projection='mantid')
        axes.plot([-10, 10], [1, 2])
        axes.set_yscale('log')
        axes.yaxis.set_major_formatter(ScalarFormatter())

        script = generate_script(fig)
        # Scalar format is not the default for log axis, so should appear in script.
        self.assertIn("axes.yaxis.set_major_formatter(ScalarFormatter(", script)
        # LogFormatterSciNotation is default for log axis, so shouldn't appear in script for minor or major ticks.
        self.assertNotIn("_formatter(LogFormatterSciNotation", script)


if __name__ == '__main__':
    unittest.main()
