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

from workbench.plotting.plotscriptgenerator.figure import (get_subplots_command_kwargs,
                                                           _remove_kwargs_if_default)


class PlotScriptGeneratorFigureTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.input_kwargs = {
            'figsize': (1.2, 1.3),
            'nrows': 2,
            'ncols': 2,
            'frameon': True,
            'subplot_kw': {
                'projection': 'mantid'
            },
            'dpi': 110,
            'num': 'fig label'
        }
        cls.fig, axes = plt.subplots(**cls.input_kwargs)

    @classmethod
    def tearDownClass(cls):
        plt.close()
        del cls.fig

    def test_get_figure_command_kwargs_returns_dict_with_expected_keys(self):
        expected_keys = [
            'dpi', 'edgecolor', 'facecolor', 'figsize', 'frameon', 'ncols', 'nrows', 'num',
            'subplot_kw'
        ]
        keys = sorted(get_subplots_command_kwargs(self.fig))
        self.assertEqual(expected_keys, keys)

    def test_remove_kwargs_if_default_returns_dict_not_containing_kwargs_with_default_values(self):
        expected_keys = ['dpi', 'figsize', 'ncols', 'nrows', 'num', 'subplot_kw']
        keys = sorted(_remove_kwargs_if_default(self.input_kwargs))
        self.assertEqual(expected_keys, keys)


if __name__ == '__main__':
    unittest.main()
