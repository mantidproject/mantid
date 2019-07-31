# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt
from numpy import array

from mantid.plots import MantidAxes  # register mantid projection  # noqa
from mantid.py3compat.mock import MagicMock
from workbench.plotting.plotscriptgenerator import convert_args_to_string
from workbench.plotting.plotscriptgenerator import PlotScriptGenerator as PSG


class PlotScriptGeneratorTest(unittest.TestCase):

    def tearDown(self):
        plt.close()

    def test_get_figure_command_kwargs_returns_correct_dict(self):
        mock_figure = MagicMock(get_figwidth=lambda: 10, get_figheight=lambda: 7,
                                dpi=111)
        expected_dict = {'figsize': (10, 7), 'dpi': 111}
        self.assertEqual(expected_dict,
                         PSG.get_figure_command_kwargs(mock_figure))

    def test_generate_figure_command_returns_correct_string(self):
        mock_figure = MagicMock(get_figwidth=lambda: 10, get_figheight=lambda: 7,
                                dpi=111)
        expected_command = "plt.figure(dpi=111, figsize=(10, 7))"
        self.assertEqual(expected_command,
                         PSG.generate_figure_command(mock_figure))

    def test_get_add_subplot_kwargs_returns_correct_dict(self):
        input_kwargs = {
            'projection': 'mantid', 'visible': False, 'xscale': 'log',
            'frame_on': False, 'xlim': (0.1, 1.1), 'yscale': 'linear',
            'ylim': (0.1, 1.1), 'sharex': True, 'sharey': None
        }
        fig, ax = plt.subplots(subplot_kw=input_kwargs)
        output_kwargs = PSG.get_add_subplot_kwargs(ax)
        for key, value in input_kwargs.items():
            self.assertEqual(output_kwargs[key], value)

    def test_generate_add_subplot_command_returns_correct_string(self):
        kwargs = {
            'projection': 'mantid', 'visible': False, 'xscale': 'log',
            'frame_on': False, 'xlim': (0.1, 1.1), 'yscale': 'linear',
            'ylim': (0.1, 1.1), 'title': 'myPlot'}
        fig = plt.figure()
        ax = fig.add_subplot(2, 2, 1, **kwargs)
        code = PSG.generate_add_subplot_command(ax)
        expected = ("add_subplot(2, 2, 1, frame_on=False, label='', "
                    "projection='mantid', sharex=None, sharey=None, title='myPlot', "
                    "visible=False, xlabel='', xlim=(0.1, 1.1), xscale='log', "
                    "ylabel='', ylim=(0.1, 1.1), yscale='linear')")
        self.assertEqual(expected, code)

    def test_get_plot_command_kwargs_from_line2d_returns_correct_dict(self):
        kwargs = {
            'alpha': 0.5, 'color': 'r', 'drawstyle': 'steps',
            'fillstyle': 'left', 'label': 'test label', 'linestyle': '--',
            'linewidth': 1.1, 'marker': 'o', 'markeredgecolor': 'g',
            'markeredgewidth': 1.2, 'markerfacecolor': 'y',
            'markerfacecoloralt': 'k', 'markersize': 1.3, 'markevery': 2,
            'solid_capstyle': 'butt', 'solid_joinstyle': 'round',
            'visible': False, 'zorder': 1.4
        }
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)
        line = ax.plot([0, 1], [1, 0], **kwargs)[0]
        ret = PSG.get_plot_command_kwargs_from_line2d(line)
        self.assertEqual(kwargs, ret)

    # Utility function tests
    def test_convert_args_to_string_returns_correct_string(self):
        kwargs_dict = {'key0': 'val0', 'key1': [2, 'str'], 'key2': 1,
                       'key3': {'a': 1.1, 'b': {'c': ['str2', 1.1]}},
                       'ndarray': array([1.1, 1.2])}
        expected_str = ("key0='val0', key1=[2, 'str'], key2=1, key3={'a': 1.1, "
                        "'b': {'c': ['str2', 1.1]}}, ndarray=[1.1, 1.2]")
        self.assertEqual(expected_str, convert_args_to_string(None, kwargs_dict))


if __name__ == '__main__':
    unittest.main()
