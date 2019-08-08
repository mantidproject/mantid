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
from copy import copy

from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator.lines import (_get_plot_command_kwargs_from_line2d,
                                                          _get_errorbar_specific_plot_kwargs,
                                                          generate_plot_command)
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

LINE2D_KWARGS = {
    'alpha': 0.5,
    'color': 'r',
    'drawstyle': 'steps',
    'fillstyle': 'left',
    'label': 'test label',
    'linestyle': '--',
    'linewidth': 1.1,
    'marker': 'o',
    'markeredgecolor': 'g',
    'markeredgewidth': 1.2,
    'markerfacecolor': 'y',
    'markerfacecoloralt': 'k',
    'markersize': 1.3,
    'markevery': 2,
    'solid_capstyle': 'butt',
    'solid_joinstyle': 'round',
    'visible': False,
    'zorder': 1.4,
    'specNum': 1
}
ERRORBAR_ONLY_KWARGS = {
    'ecolor': '#ff0000',
    'elinewidth': 1.5,
    'capsize': 1.6,
    'capthick': 1.7,
    'barsabove': True,
    'errorevery': 1
}
ERRORBAR_KWARGS = copy(LINE2D_KWARGS)
ERRORBAR_KWARGS.update(ERRORBAR_ONLY_KWARGS)
MANTID_ONLY_KWARGS = {'specNum': 1, 'distribution': False, 'update_axes_labels': False}


class PlotScriptGeneratorLinesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                                      DataY=[2, 3, 4, 5],
                                      DataE=[1, 2, 3, 4],
                                      NSpec=2,
                                      OutputWorkspace='test_ws')

    def setUp(self):
        fig = plt.figure()
        self.ax = fig.add_subplot(1, 1, 1, projection='mantid')

    def tearDown(self):
        plt.close()

    def test_get_plot_command_kwargs_from_line2d_returns_correct_dict(self):
        line = self.ax.plot(self.test_ws, **LINE2D_KWARGS)[0]
        ret = _get_plot_command_kwargs_from_line2d(line)
        self.assertEqual(LINE2D_KWARGS, ret)

    def test_generate_plot_command_returns_correct_string_for_line2d(self):
        kwargs = copy(LINE2D_KWARGS)
        kwargs.update(MANTID_ONLY_KWARGS)
        line = self.ax.plot(self.test_ws, **kwargs)[0]
        output = generate_plot_command(line)
        expected_command = ("plot({}, {})".format(self.test_ws.name(),
                                                  convert_args_to_string(None, kwargs)))
        self.assertEqual(expected_command, output)

    def test_get_errorbar_specific_plot_kwargs_returns_correct_dict(self):
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop('markeredgewidth')
        err_cont = self.ax.errorbar(self.test_ws, **kwargs)
        output = _get_errorbar_specific_plot_kwargs(err_cont)
        self.assertEqual(ERRORBAR_ONLY_KWARGS, output)

    def test_generate_plot_command_returns_correct_string_for_errorbar_container(self):
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop('markeredgewidth')
        kwargs.update(MANTID_ONLY_KWARGS)
        err_cont = self.ax.errorbar(self.test_ws, **kwargs)
        output = generate_plot_command(err_cont)
        expected_command = ("errorbar({}, {})".format(self.test_ws.name(),
                                                      convert_args_to_string(None, kwargs)))
        self.assertEqual(expected_command, output)


if __name__ == '__main__':
    unittest.main()
