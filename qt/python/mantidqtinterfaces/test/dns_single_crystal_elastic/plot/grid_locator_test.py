# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import MagicMock, patch
import numpy as np
from mpl_toolkits.axisartist.grid_finder import MaxNLocator, FormatterPrettyPrint, FixedLocator
import mantidqtinterfaces.dns_single_crystal_elastic.plot.grid_locator as grid_locator


class GridLocatorTest(unittest.TestCase):
    def test_get_grid_formatter_state_zero(self):
        locator, formatter = grid_locator.get_grid_formatter(0)
        self.assertIsInstance(locator, MaxNLocator)
        self.assertIsInstance(formatter, FormatterPrettyPrint)

    def test_get_grid_formatter_non_zero_state(self):
        locator, formatter = grid_locator.get_grid_formatter(2)
        self.assertIsInstance(locator, FixedLocator)
        self.assertIsInstance(formatter, FormatterPrettyPrint)

    @staticmethod
    def test_get_grid_helper_no_switch():
        inv_tr, tr = grid_locator.get_grid_helper_arguments(1, 2, 3, 4, switch=False)
        x = [1, 0]
        y = [0, 1]
        h, k = tr(x, y)
        np.testing.assert_array_equal(h, np.array([1, 2]))
        np.testing.assert_array_equal(k, np.array([3, 4]))
        x_tr, y_tr = inv_tr(h, k)
        np.testing.assert_array_equal(x_tr, np.array([1, 0]))
        np.testing.assert_array_equal(y_tr, np.array([0, 1]))

    @staticmethod
    def test_get_grid_helper_with_switch():
        inv_tr, tr = grid_locator.get_grid_helper_arguments(1, 2, 3, 4, switch=True)
        x = [1, 0]
        y = [0, 1]
        h, k = tr(x, y)
        np.testing.assert_array_equal(h, np.array([4, 3]))
        np.testing.assert_array_equal(k, np.array([2, 1]))
        x_tr, y_tr = inv_tr(h, k)
        np.testing.assert_array_equal(x_tr, np.array([1, 0]))
        np.testing.assert_array_equal(y_tr, np.array([0, 1]))

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.grid_locator.GridHelperCurveLinear")
    def test_get_grid_helper_new(self, mock_grid_helper_cls):
        mock_instance = MagicMock()
        mock_grid_helper_cls.return_value = mock_instance
        result = grid_locator.get_grid_helper(None, grid_state=0, a=1, b=2, c=3, d=4, switch=False)
        mock_grid_helper_cls.assert_called_once()
        self.assertEqual(result, mock_instance)

    def test_get_grid_helper_update(self):
        mock_grid_helper = MagicMock()
        result = grid_locator.get_grid_helper(mock_grid_helper, grid_state=0, a=1, b=2, c=3, d=4, switch=False)
        mock_grid_helper.update_grid_finder.assert_called_once()
        self.assertEqual(result, mock_grid_helper)


if __name__ == "__main__":
    unittest.main()
