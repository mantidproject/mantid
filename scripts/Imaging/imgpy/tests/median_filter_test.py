from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class MedianTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(MedianTest, self).__init__(*args, **kwargs)

        from filters import median_filter
        self.alg = median_filter

    def test_not_executed(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = None
        mode = None
        result = self.alg.execute(images, size, mode)
        npt.assert_equal(result, control)

    def test_executed_no_helper_parallel(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        result = self.alg.execute(images, size, mode)
        th.assert_not_equals(result, control)

    def test_executed_no_helper_seq(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        th.switch_mp_off()
        result = self.alg.execute(images, size, mode)
        th.switch_mp_on()
        th.assert_not_equals(result, control)


if __name__ == '__main__':
    unittest.main()
