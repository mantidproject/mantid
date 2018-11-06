# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid
from isis_powder.routines import focus
import unittest


class ISISPowderFocusCropTest(unittest.TestCase):
    def test_crop_before(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [0, 0, 10, 30, 2000, 80, 50, 40, 30, 25, 30]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y)
        # search entire range (0 to 100) for max
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws, test_ws, 0, 100)
        y_compare = [30, 2000, 80, 50, 40, 30, 25, 30]
        result = test_ws.readY(0)
        for compare, val in zip(y_compare, result):
            self.assertEqual(compare, val)

    def test_crop_after(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [50, 100, 300, 500, 2000, 80, 50, 0, 0, 0, 0]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y)
        # search entire range (0 to 100) for max
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws, test_ws, 0, 100)
        y_compare = [50, 100, 300, 500, 2000, 80, 50]
        result = test_ws.readY(0)
        for compare, val in zip(y_compare, result):
            self.assertEqual(compare, val)

    def test_crop_both(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [0, 0, 10, 30, 2000, 80, 50, 0, 0, 0, 0]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y)
        # search entire range (0 to 100) for max
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws, test_ws, 0, 100)
        y_compare = [30, 2000, 80, 50]
        result = test_ws.readY(0)
        for compare, val in zip(y_compare, result):
            self.assertEqual(compare, val)

    def test_no_crop(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [1, 5, 10, 30, 20, 80, 50, 40, 20, 10, 1]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y)
        # search entire range (0 to 100) for max
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws, test_ws, 0, 100)
        y_compare = [1, 5, 10, 30, 20, 80, 50, 40, 20, 10, 1]
        result = test_ws.readY(0)
        for compare, val in zip(y_compare, result):
            self.assertEqual(compare, val)

    def test_no_crop_subrange(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [1, 5, 10, 30, 20, 80, 50, 40, 20, 10, 10000]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y)
        # search from 0 to 20 x for max
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws, test_ws, 0, 20)
        y_compare = [1, 5, 10, 30, 20, 80, 50, 40, 20, 10, 10000]
        result = test_ws.readY(0)
        for compare, val in zip(y_compare, result):
            self.assertEqual(compare, val)

    def test_crop_subrange(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [1, 50, 2000, 30, 20, 80, 50, 40, 20, 10, 1]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y)
        # search from 0 to 30 x for max
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws, test_ws, 0, 30)
        y_compare = [50, 2000, 30, 20, 80, 50, 40]
        result = test_ws.readY(0)
        for compare, val in zip(y_compare, result):
            self.assertEqual(compare, val)


if __name__ == '__main__':
    unittest.main()
