from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class CutOffTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(CutOffTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import outliers
        self.alg = outliers

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.full((10, 10, 10), 1., dtype=np.float32)

    def test_not_executed(self):
        images = self.generate_images()
        control = self.generate_images()
        err_msg = "TEST NOT EXECUTED :: Running cut off with level {0} changed the data!"

        outliers_level = None
        result = self.alg.execute(images, outliers_level, self.h)
        npt.assert_equal(
            result, control, err_msg=err_msg.format(outliers_level))

        outliers_level = 0
        result = self.alg.execute(images, outliers_level, self.h)
        npt.assert_equal(
            result, control, err_msg=err_msg.format(outliers_level))

        outliers_level = -0.1
        result = self.alg.execute(images, outliers_level, self.h)
        npt.assert_equal(
            result, control, err_msg=err_msg.format(outliers_level))

    def test_executed(self):
        images = self.generate_images()
        control = self.generate_images()
        control[3, :, :] = 0.1

        images[3, :, :] = 0.1
        outliers_level = 0.4
        result = self.alg.execute(images, outliers_level, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        images = self.generate_images()
        images[3, :, :] = 0.1
        outliers_level = 0.001
        result = self.alg.execute(images, outliers_level, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

    def test_executed_no_helper(self):
        images = self.generate_images()
        control = self.generate_images()
        control[3, :, :] = 0.1

        images[3, :, :] = 0.1
        outliers_level = 0.4
        result = self.alg.execute(images, outliers_level)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        images = self.generate_images()
        images[3, :, :] = 0.1
        outliers_level = 0.001
        result = self.alg.execute(images, outliers_level)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)


if __name__ == '__main__':
    unittest.main()
