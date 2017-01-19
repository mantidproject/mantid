from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class RotateStackTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(RotateStackTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import rotate_stack
        self.alg = rotate_stack

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.random.rand(10, 10, 10), np.full((10, 10), 0.9), np.full((10, 10), 0.1)

    def test_not_executed(self):
        sample, flat, dark = self.generate_images()
        from copy import deepcopy
        control = deepcopy(sample)

        # empty params
        result = self.alg.execute(sample, None, self.h)[0]
        npt.assert_equal(result, control)

    def test_executed(self):
        self.do_execute(self.h)

    def test_executed_no_helper(self):
        self.do_execute(None)

    def do_execute(self, helper):
        sample, flat, dark = self.generate_images()

        rotation = 1  # once clockwise
        sample[:, 0, 0] = 42  # set all images at 0,0 to 42
        result = self.alg.execute(sample, rotation, h=helper)[0]
        h = result.shape[1]
        w = result.shape[2]
        npt.assert_equal(result[:, 0, w - 1], 42.0)

if __name__ == '__main__':
    unittest.main()
