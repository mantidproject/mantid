from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class MedianTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(MedianTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        from filters import median_filter
        self.alg = median_filter

        self.h = Helper(r)

    def test_not_executed(self):
        images, control = th.gen_img_shared_array_and_copy()

        err_msg = "TEST NOT EXECUTED :: Running median_filter with size {0}, mode {1} and order {2} changed the data!"

        size = None
        mode = None
        result = self.alg.execute(images, size, mode, h=self.h)
        npt.assert_equal(result, control)

    def test_executed_parallel(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        result = self.alg.execute(images, size, mode, h=self.h)
        th.assert_not_equals(images, control)

    def test_executed_no_helper_parallel(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        result = self.alg.execute(images, size, mode)
        th.assert_not_equals(images, control)

    def test_executed_seq(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        th.switch_mp_off()
        result = self.alg.execute(images, size, mode, h=self.h)
        th.switch_mp_on()
        th.assert_not_equals(images, control)

    def test_executed_no_helper_seq(self):
        images, control = th.gen_img_shared_array_and_copy()

        size = 3
        mode = 'reflect'
        th.switch_mp_off()
        result = self.alg.execute(images, size, mode)
        th.switch_mp_on()
        th.assert_not_equals(images, control)


if __name__ == '__main__':
    unittest.main()
