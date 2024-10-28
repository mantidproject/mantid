# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_utils import (
    convert_index_to_row_and_col,
    get_num_row_and_col,
    get_y_min_max_between_x_range,
)


class PlottingCanvasUtilsTest(unittest.TestCase):
    def setUp(self):
        return

    def test_get_y_min_max_between_x_range(self):
        line = mock.Mock()
        line.get_data = mock.Mock(return_value=([0, 1, 2, 3, 4, 5], [-10, 3, 7, -1, 3, 100]))

        ymin, ymax = get_y_min_max_between_x_range(line, 1, 4, 0, 0)
        self.assertEqual(ymin, -1)
        self.assertEqual(ymax, 7)

    def test_get_num_row_and_col_square(self):
        n_rows, n_cols = get_num_row_and_col(4)
        self.assertEqual(n_rows, 2)
        self.assertEqual(n_cols, 2)

    def test_get_num_row_and_col_two(self):
        n_rows, n_cols = get_num_row_and_col(2)
        self.assertEqual(n_rows, 1)
        self.assertEqual(n_cols, 2)

    def test_get_num_row_and_col_six(self):
        n_rows, n_cols = get_num_row_and_col(6)
        self.assertEqual(n_rows, 2)
        self.assertEqual(n_cols, 3)

    def test_get_num_row_and_col_27(self):
        # 27 is a square of 5.196152422706632
        n_rows, n_cols = get_num_row_and_col(27)
        self.assertEqual(n_rows, 5)
        self.assertEqual(n_cols, 6)

    def test_get_num_row_and_col_with_empty_axis(self):
        n_rows, n_cols = get_num_row_and_col(8)
        self.assertEqual(n_rows, 3)
        self.assertEqual(n_cols, 3)

    def test_convert_index_to_row_and_col_square(self):
        # fill rows first
        rows = [0, 0, 1, 1]
        cols = [0, 1, 0, 1]
        n_rows, n_cols = get_num_row_and_col(len(rows))
        for index in range(len(rows)):
            row, col = convert_index_to_row_and_col(index, n_rows, n_cols)
            self.assertEqual(rows[index], row)
            self.assertEqual(cols[index], col)

    def test_convert_index_to_row_and_col_with_empty_axis(self):
        # fill rows first
        rows = [0, 0, 0, 1, 1, 1, 2, 2]
        cols = [0, 1, 2, 0, 1, 2, 0, 1]
        n_rows, n_cols = get_num_row_and_col(len(rows))
        for index in range(len(rows)):
            row, col = convert_index_to_row_and_col(index, n_rows, n_cols)
            self.assertEqual(rows[index], row)
            self.assertEqual(cols[index], col)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
