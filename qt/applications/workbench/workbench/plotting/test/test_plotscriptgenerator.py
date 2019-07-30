# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from numpy import array

from mantid.py3compat.mock import MagicMock
from workbench.plotting.plotscriptgenerator import convert_args_to_string
from workbench.plotting.plotscriptgenerator import PlotScriptGenerator as PSG


class PlotScriptGeneratorTest(unittest.TestCase):

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
