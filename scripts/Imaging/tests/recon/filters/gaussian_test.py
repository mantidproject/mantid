from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class GaussianTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(GaussianTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import gaussian
        self.alg = gaussian

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
        err_msg = "TEST NOT EXECUTED :: Running gaussian with size {0}, mode {1} and order {2} changed the data!"

        size = None
        mode = None
        order = None
        result = self.alg.execute(images, size, mode, order, h=self.h)
        npt.assert_equal(
            result, control, err_msg=err_msg.format(size, mode, order))

    def test_executed(self):
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)

        size = 3
        mode = 'reflect'
        order = 1
        result = self.alg.execute(images, size, mode, order, h=self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

    def test_executed_no_helper(self):
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)

        size = 3
        mode = 'reflect'
        order = 1
        result = self.alg.execute(images, size, mode, order)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)


if __name__ == '__main__':
    unittest.main()
