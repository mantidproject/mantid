from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class GaussianTest(unittest.TestCase):
    """
    Surprisingly sequential Gaussian seems to outperform parallel Gaussian on very small data.
    This does not scale and parallel execution is always faster on any reasonably sized data (e.g. 143,512,512)
    """

    def __init__(self, *args, **kwargs):
        super(GaussianTest, self).__init__(*args, **kwargs)

        from core.filters import gaussian
        self.alg = gaussian

    def test_not_executed(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = None
        mode = None
        order = None
        result = self.alg.execute(images, size, mode, order)
        npt.assert_equal(result, control)

    def test_executed_parallel(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        order = 1
        result = self.alg.execute(images, size, mode, order)
        th.assert_not_equals(result, control)

    def test_executed_no_helper_parallel(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        order = 1
        result = self.alg.execute(images, size, mode, order)
        th.assert_not_equals(result, control)

    def test_executed_no_helper_seq(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        order = 1

        th.switch_mp_off()
        result = self.alg.execute(images, size, mode, order)
        th.switch_mp_on()

        th.assert_not_equals(result, control)


if __name__ == '__main__':
    unittest.main()
