# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from copy import copy

import matplotlib

matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt
from numpy import array

from mantid.simpleapi import CreateWorkspace
from mantid.plots import MantidAxes  # register mantid projection  # noqa
from mantid.py3compat.mock import MagicMock
from workbench.plotting.plotscriptgenerator import convert_args_to_string
from workbench.plotting.plotscriptgenerator import PlotScriptGenerator as PSG

LINE2D_KWARGS = {
    'alpha': 0.5, 'color': 'r', 'drawstyle': 'steps',
    'fillstyle': 'left', 'label': 'test label', 'linestyle': '--',
    'linewidth': 1.1, 'marker': 'o', 'markeredgecolor': 'g',
    'markeredgewidth': 1.2, 'markerfacecolor': 'y',
    'markerfacecoloralt': 'k', 'markersize': 1.3, 'markevery': 2,
    'solid_capstyle': 'butt', 'solid_joinstyle': 'round',
    'visible': False, 'zorder': 1.4, 'specNum': 1}
ERRORBAR_ONLY_KWARGS = {
    'ecolor': '#ff0000', 'elinewidth': 1.5, 'capsize': 1.6, 'capthick': 1.7,
    'barsabove': True, 'errorevery': 1}
ERRORBAR_KWARGS = copy(LINE2D_KWARGS)
ERRORBAR_KWARGS.update(ERRORBAR_ONLY_KWARGS)
MANTID_ONLY_KWARGS = {'specNum': 1, 'distribution': False}


class PlotScriptGeneratorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            OutputWorkspace='test_ws')

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
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        line = ax.plot(self.test_ws, **LINE2D_KWARGS)[0]
        ret = PSG._get_plot_command_kwargs_from_line2d(line)
        self.assertEqual(LINE2D_KWARGS, ret)

    def test_generate_plot_command_returns_correct_string_for_line2d(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        kwargs = copy(LINE2D_KWARGS)
        kwargs.update(MANTID_ONLY_KWARGS)
        line = ax.plot(self.test_ws, **kwargs)[0]
        output = PSG.generate_plot_command(line)
        expected_command = (
            "plot({}, {})".format(self.test_ws.name(),
                                  convert_args_to_string(None, kwargs)))
        self.assertEqual(expected_command, output)

    def test_get_errorbar_specific_plot_kwargs_returns_correct_dict(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop('markeredgewidth')
        err_cont = ax.errorbar(self.test_ws, **kwargs)
        output = PSG._get_errorbar_specific_plot_kwargs(err_cont)
        self.assertEqual(ERRORBAR_ONLY_KWARGS, output)

    def test_generate_plot_command_returns_correct_string_for_errorbar_container(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop('markeredgewidth')
        kwargs.update(MANTID_ONLY_KWARGS)
        err_cont = ax.errorbar(self.test_ws, **kwargs)
        output = PSG.generate_plot_command(err_cont)
        expected_command = (
            "errorbar({}, {})".format(self.test_ws.name(),
                                      convert_args_to_string(None, kwargs)))
        self.assertEqual(expected_command, output)

    def test_generate_script_returns_correct_string_for_single_errorbar_plot(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        kwargs = copy(ERRORBAR_KWARGS)
        ax.errorbar(self.test_ws, **kwargs)
        expected_string = """fig = plt.figure(dpi=100.0, figsize=(6.4, 4.8))
ax = fig.add_subplot(1, 1, 1, frame_on=True, label='', projection='mantid', sharex=None, sharey=None, title='', visible=True, xlabel='', xlim=(14.5, 25.5), xscale='linear', ylabel='', ylim=(0.07999999999999997, 0.52), yscale='linear')
ax.errorbar(test_ws, alpha=0.5, barsabove=True, capsize=1.6, capthick=1.2, color='r', distribution=False, drawstyle='steps', ecolor='#ff0000', elinewidth=1.5, errorevery=1, fillstyle='left', label='test label', linestyle='--', linewidth=1.1, marker='o', markeredgecolor='g', markerfacecolor='y', markerfacecoloralt='k', markersize=1.3, markevery=2, solid_capstyle='butt', solid_joinstyle='round', specNum=1, visible=False, zorder=1.4)
plt.show()"""
        self.assertEqual(expected_string, PSG.generate_script(fig))

    def test_generate_script_returns_correct_number_of_lines_for_overplot(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        ax.plot(self.test_ws, specNum=1)
        ax.errorbar(self.test_ws, specNum=2)
        output = PSG.generate_script(fig)
        num_lines = len(output.split('\n'))
        err_message = ("Expected to output 5 lines, found {}.\nOutput:\n{}"
                       "".format(num_lines, output))
        self.assertEqual(5, num_lines, msg=err_message)

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
