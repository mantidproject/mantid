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

from workbench.plotting.plotscriptgenerator.axes import (
    get_add_subplot_pos_args, get_add_subplot_kwargs, generate_add_subplot_command)


class PlotScriptGeneratorAxesTest(unittest.TestCase):

    def tearDown(self):
        plt.close()

    def test_get_add_subplot_kwargs_returns_correct_dict(self):
        input_kwargs = {
            'projection': 'mantid', 'visible': False, 'xscale': 'log',
            'frame_on': False, 'xlim': (0.1, 1.1), 'yscale': 'linear',
            'ylim': (0.1, 1.1), 'sharex': True, 'sharey': None
        }
        fig, ax = plt.subplots(subplot_kw=input_kwargs)
        output_kwargs = get_add_subplot_kwargs(ax)
        for key, value in input_kwargs.items():
            self.assertEqual(output_kwargs[key], value)

    def test_generate_add_subplot_command_returns_correct_string(self):
        kwargs = {
            'projection': 'mantid', 'visible': False, 'xscale': 'log',
            'frame_on': False, 'xlim': (0.1, 1.1), 'yscale': 'linear',
            'ylim': (0.1, 1.1), 'title': 'myPlot'}
        fig = plt.figure()
        ax = fig.add_subplot(2, 2, 1, **kwargs)
        code = generate_add_subplot_command(ax)
        expected = ("add_subplot(2, 2, 1, frame_on=False, label='', "
                    "projection='mantid', sharex=None, sharey=None, title='myPlot', "
                    "visible=False, xlabel='', xlim=(0.1, 1.1), xscale='log', "
                    "ylabel='', ylim=(0.1, 1.1), yscale='linear')")
        self.assertEqual(expected, code)
