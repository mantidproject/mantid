# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.ElementalAnalysis.Plotting import plotting_utils as putils

try:
    from unittest import mock
except ImportError:
    import mock


class PlottingUtilsTest(unittest.TestCase):
    def setUp(self):
        putils.layout = [mock.Mock() for i in putils.layout]

    def test_get_layout_num_too_high(self):
        with self.assertRaises(IndexError):
            putils.get_layout(6)

    def test_layout_functions_called(self):
        for i, layout_function in zip(range(1, 6), putils.layout):
            putils.get_layout(i)
            self.assertEquals(layout_function.call_count, 1)


if __name__ == "__main__":
    unittest.main()
