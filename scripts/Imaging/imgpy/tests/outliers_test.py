from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th
from core.filters import outliers


class OutliersTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(OutliersTest, self).__init__(*args, **kwargs)

    def test_not_executed_no_threshold(self):
        images, control = th.gen_img_shared_array_and_copy()
        # invalid thresholds
        threshold = None
        radius = 8
        result = outliers.execute(images, threshold, radius, cores=1)
        npt.assert_equal(result, control)

    def test_not_executed_bad_threshold(self):
        images, control = th.gen_img_shared_array_and_copy()
        radius = 8
        threshold = 0
        result = outliers.execute(images, threshold, radius, cores=1)
        npt.assert_equal(result, control)

    def test_not_executed_bad_threshold2(self):
        images, control = th.gen_img_shared_array_and_copy()
        radius = 8
        threshold = -42
        result = outliers.execute(images, threshold, radius, cores=1)
        npt.assert_equal(result, control)

    def test_not_executed_no_radius(self):
        images, control = th.gen_img_shared_array_and_copy()
        radius = 8
        threshold = 42
        radius = None
        result = outliers.execute(images, threshold, radius, cores=1)
        npt.assert_equal(result, control)

    def test_executed(self):
        images, control = th.gen_img_shared_array_and_copy()
        radius = 8
        threshold = 0.1
        result = outliers.execute(images, threshold, radius, cores=1)
        th.assert_not_equals(result, control)

    def test_executed_no_helper(self):
        images, control = th.gen_img_shared_array_and_copy()

        threshold = 0.1
        radius = 8
        result = outliers.execute(images, threshold, radius, cores=1)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)


if __name__ == '__main__':
    unittest.main()
