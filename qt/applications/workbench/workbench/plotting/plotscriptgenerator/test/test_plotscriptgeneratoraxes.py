# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt

from mantid.py3compat.mock import Mock
from workbench.plotting.plotscriptgenerator.axes import (get_add_subplot_kwargs,
                                                         generate_add_subplot_command,
                                                         generate_axis_limit_commands,
                                                         generate_axis_label_commands)


class PlotScriptGeneratorAxesTest(unittest.TestCase):

    def setUp(self):
        self.kwargs = {
            'projection': 'mantid',
            'visible': False,
            'xscale': 'log',
            'frame_on': False,
            'yscale': 'linear',
            'sharex': None,
            'sharey': None,
            'title': 'myPlot'
        }
        fig = plt.figure()
        self.ax = fig.add_subplot(2, 2, 1, **self.kwargs)

    def tearDown(self):
        plt.close()

    def test_get_add_subplot_kwargs_returns_correct_dict(self):
        output_kwargs = get_add_subplot_kwargs(self.ax)
        for key, value in self.kwargs.items():
            self.assertEqual(output_kwargs[key], value)

    def test_generate_add_subplot_command_returns_correct_string(self):
        code = generate_add_subplot_command(self.ax)
        expected = ("add_subplot(2, 2, 1, frame_on=False, label='', "
                    "projection='mantid', sharex=None, sharey=None, title='myPlot', "
                    "visible=False, xscale='log', yscale='linear')")
        self.assertEqual(expected, code)


class PlotGeneratorAxisTest(unittest.TestCase):

    def test_generate_axis_limits_commands_returns_correct_commands(self):
        mock_ax = Mock(get_xlim=lambda: (0.5, 1), get_ylim=lambda: (2, 10))
        expected = ["set_xlim((0.5, 1))", "set_ylim((2, 10))"]
        actual = generate_axis_limit_commands(mock_ax)
        self.assertEqual(expected, actual)

    def test_generate_axis_label_commands_only_returns_commands_for_labels_that_are_set(self):
        mock_ax = Mock(get_xlabel=lambda: '', get_ylabel=lambda: 'y')
        expected = ["set_ylabel('y')"]
        self.assertEqual(expected, generate_axis_label_commands(mock_ax))

    def test_generate_axis_label_commands_returns_empty_list_when_no_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: '', get_ylabel=lambda: '')
        self.assertEqual([], generate_axis_label_commands(mock_ax))
