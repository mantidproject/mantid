from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class ScaleTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(ScaleTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import scale
        self.alg = scale

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.random.rand(10, 10, 10)

    def test_not_executed(self):
        sample = self.generate_images()
        from copy import deepcopy
        control = deepcopy(sample)
        err_msg = "TEST NOT EXECUTED :: Running scale with size {0}, mode {1} and order {2} changed the data!"

        # bad params
        scale = None
        mode = 'nearest'
        result = self.alg.execute(sample, scale, mode, self.h)
        npt.assert_equal(result, control)

        scale = -1
        result = self.alg.execute(sample, scale, mode, self.h)
        npt.assert_equal(result, control)

        scale = 0
        result = self.alg.execute(sample, scale, mode, self.h)
        npt.assert_equal(result, control)

        scale = -0
        result = self.alg.execute(sample, scale, mode, self.h)
        npt.assert_equal(result, control)

    def test_executed(self):
        self.do_execute(self.h)

    def test_executed_no_helper(self):
        self.do_execute(None)

    def do_execute(self, helper):
        sample = self.generate_images()

        scale = 2.  # twice the size
        expected_shape = int(sample.shape[1] * scale)
        mode = 'nearest'
        result = self.alg.execute(sample, scale, mode, helper)
        npt.assert_equal(result.shape[1], expected_shape)
        npt.assert_equal(result.shape[2], expected_shape)

        scale = 5.  # five times the size
        expected_shape = int(sample.shape[1] * scale)
        result = self.alg.execute(sample, scale, mode, helper)
        npt.assert_equal(result.shape[1], expected_shape)
        npt.assert_equal(result.shape[2], expected_shape)

if __name__ == '__main__':
    unittest.main()
