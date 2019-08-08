# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from mantid.py3compat.mock import MagicMock
from workbench.plotting.plotscriptgenerator.figure import (get_figure_command_kwargs,
                                                           generate_figure_command)


class PlotScriptGeneratorFigureTest(unittest.TestCase):

    def setUp(self):
        self.mock_figure = MagicMock(get_figwidth=lambda: 10,
                                     get_figheight=lambda: 7,
                                     dpi=111,
                                     get_label=lambda: 'fig_num')

    def test_get_figure_command_kwargs_returns_correct_dict(self):
        expected_dict = {'figsize': (10, 7), 'dpi': 111, 'num': 'fig_num'}
        self.assertEqual(expected_dict, get_figure_command_kwargs(self.mock_figure))

    def test_generate_figure_command_returns_correct_string(self):
        expected_command = "plt.figure(dpi=111, figsize=(10, 7), num='fig_num')"
        self.assertEqual(expected_command, generate_figure_command(self.mock_figure))


if __name__ == '__main__':
    unittest.main()
