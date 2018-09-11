from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid
from isis_powder.routines import focus
import unittest


class ISISPowderFocusCropTest(unittest.TestCase):
    def test_crop_before(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [0, 0, 10, 30, 2000, 80, 50, 40, 30, 25, 30]
        e = [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y, DataE=e)
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws)
        y_compare = [30, 2000, 80, 50, 40, 30, 25, 30]
        self.assertSequenceEqual(y_compare, test_ws.readY(0))

    def test_crop_after(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [50, 100, 300, 500, 2000, 80, 50, 0, 0, 0, 0]
        e = [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y, DataE=e)
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws)
        y_compare = [50, 100, 300, 500, 2000, 80, 50]
        self.assertSequenceEqual(y_compare, test_ws.readY(0))

    def test_crop_both(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [0, 0, 10, 30, 2000, 80, 50, 0, 0, 0, 0]
        e = [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y, DataE=e)
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws)
        y_compare = [30, 2000, 80, 50]
        self.assertSequenceEqual(y_compare, test_ws.readY(0))

    def test_no_crop(self):
        x = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        y = [1, 5, 10, 30, 20, 80, 50, 40, 20, 10, 1]
        e = [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5]
        test_ws = mantid.CreateWorkspace(DataX=x, DataY=y, DataE=e)
        test_ws = focus._crop_spline_to_percent_of_max(test_ws, test_ws)
        y_compare = [1, 5, 10, 30, 20, 80, 50, 40, 20, 10, 1]
        self.assertSequenceEqual(y_compare, test_ws.readY(0))


if __name__ == '__main__':
    unittest.main()
