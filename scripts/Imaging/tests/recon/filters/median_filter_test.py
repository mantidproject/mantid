from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class MedianTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(MedianTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import median_filter
        self.alg = median_filter

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.random.rand(10, 10, 10)

    def test_not_executed(self):
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)
        err_msg = "TEST NOT EXECUTED :: Running median_filter with size {0}, mode {1} and order {2} changed the data!"

        size = None
        mode = None
        result = self.alg.execute(images, size, mode, h=self.h)
        npt.assert_equal(result, control)

    def test_executed(self):
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)

        size = 3
        mode = 'reflect'
        result = self.alg.execute(images, size, mode, h=self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

    def test_executed_no_helper(self):
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)

        size = 3
        mode = 'reflect'
        result = self.alg.execute(images, size, mode)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)


if __name__ == '__main__':
    unittest.main()
