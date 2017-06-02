from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th
from core.filters import rebin


class RebinTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(RebinTest, self).__init__(*args, **kwargs)

    def test_not_executed_rebin_none(self):
        images, control = th.gen_img_shared_array_and_copy()
        val = None
        mode = 'nearest'
        result = rebin.execute(images, val, mode)
        npt.assert_equal(result, control)

    def test_not_executed_rebin_negative(self):
        images, control = th.gen_img_shared_array_and_copy()
        mode = 'nearest'
        val = -1
        result = rebin.execute(images, val, mode)
        npt.assert_equal(result, control)

    def test_not_executed_rebin_zero(self):
        images, control = th.gen_img_shared_array_and_copy()
        mode = 'nearest'
        val = 0
        result = rebin.execute(images, val, mode)
        npt.assert_equal(result, control)

    def test_executed_par_2(self):
        self.do_execute(2.)

    def test_executed_par_5(self):
        self.do_execute(5.)

    def test_executed_seq_2(self):
        th.switch_mp_off()
        self.do_execute(2.)
        th.switch_mp_on()

    def test_executed_seq_5(self):
        th.switch_mp_off()
        self.do_execute(5.)
        th.switch_mp_on()

    def do_execute(self, val=2.):
        images = th.gen_img_shared_array()
        mode = 'nearest'

        expected_x = int(images.shape[1] * val)
        expected_y = int(images.shape[2] * val)
        result = rebin.execute(images, val, mode)
        npt.assert_equal(result.shape[1], expected_x)
        npt.assert_equal(result.shape[2], expected_y)


if __name__ == '__main__':
    unittest.main()
