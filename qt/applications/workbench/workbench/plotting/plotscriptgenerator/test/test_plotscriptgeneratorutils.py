# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from numpy import array

from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string


class PlotScriptGeneratorUtilsTest(unittest.TestCase):

    def test_convert_args_to_string_returns_correct_string(self):
        kwargs_dict = {'key0': 'val0', 'key1': [2, 'str'], 'key2': 1,
                       'key3': {'a': 1.1, 'b': {'c': ['str2', 1.1]}},
                       'ndarray': array([1.1, 1.2])}
        expected_str = ("key0='val0', key1=[2, 'str'], key2=1, key3={'a': 1.1, "
                        "'b': {'c': ['str2', 1.1]}}, ndarray=[1.1, 1.2]")
        self.assertEqual(expected_str, convert_args_to_string(None, kwargs_dict))


if __name__ == '__main__':
    unittest.main()
