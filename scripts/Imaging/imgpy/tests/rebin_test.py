from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class RebinTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(RebinTest, self).__init__(*args, **kwargs)

        from filters import rebin
        self.alg = rebin

    def test_not_executed(self):
        images, control = th.gen_img_shared_array_and_copy()

        # bad params
        rebin = None
        mode = 'nearest'
        result = self.alg.execute(images, rebin, mode)
        npt.assert_equal(result, control)

        rebin = -1
        result = self.alg.execute(images, rebin, mode)
        npt.assert_equal(result, control)

        rebin = 0
        result = self.alg.execute(images, rebin, mode)
        npt.assert_equal(result, control)

        rebin = -0
        result = self.alg.execute(images, rebin, mode)
        npt.assert_equal(result, control)

    def test_executed_par(self):
        self.do_execute()

    def test_executed_seq(self):
        th.switch_mp_off()
        self.do_execute()
        th.switch_mp_on()

    def do_execute(self):
        images = th.gen_img_shared_array()

        rebin = 2.  # twice the size
        expected_x = int(images.shape[1] * rebin)
        expected_y = int(images.shape[2] * rebin)
        mode = 'nearest'
        result = self.alg.execute(images, rebin, mode)
        npt.assert_equal(result.shape[1], expected_x)
        npt.assert_equal(result.shape[2], expected_y)

        rebin = 5.  # five times the size
        expected_x = int(images.shape[1] * rebin)
        expected_y = int(images.shape[2] * rebin)

        result = self.alg.execute(images, rebin, mode)
        npt.assert_equal(result.shape[1], expected_x)
        npt.assert_equal(result.shape[2], expected_y)


if __name__ == '__main__':
    unittest.main()
