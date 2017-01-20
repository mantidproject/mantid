from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class NormaliseByAirRegionTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(NormaliseByAirRegionTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import normalise_by_air_region
        self.alg = normalise_by_air_region

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.random.rand(10, 10, 10)

    def test_not_executed_no_crop(self):
        # Duplicated code because 'nose.main()' failed to run it properly
        crop = False
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)
        err_msg = "TEST NOT EXECUTED :: Running normalise_by_air_region with size {0}, mode {1} and order {2} changed the data!"

        # empty params
        air = None
        roi = None
        result = self.alg.execute(images, air, roi, crop, self.h)
        npt.assert_equal(result, control)

        # invalid air region type
        air = '[1,1,1,1]'
        roi = [1, 1, 8, 8]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

        # invalid roi region type
        roi = '[1,2,3,4]'
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

        # invalid data type
        import numpy as np
        images = np.arange(100).reshape(10, 10)
        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

    def test_not_executed_with_crop(self):
        # Duplicated code because 'nose.main()' failed to run it properly
        crop = True
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)

        # empty params
        air = None
        roi = None
        result = self.alg.execute(images, air, roi, crop, self.h)
        npt.assert_equal(result, control)

        # invalid air region type
        air = '[1,1,1,1]'
        roi = [1, 1, 8, 8]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

        # invalid air region coords
        air = [13, 13, 13, 31]
        roi = [1, 1, 8, 8]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

        # invalid roi region type
        roi = '[1,2,3,4]'
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

        # invalid data type
        import numpy as np
        images = np.arange(100).reshape(10, 10)
        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute,
                          images, air, roi, crop, self.h)

    def test_executed(self):
        self.do_execute(self.h)

    def test_executed_no_helper(self):
        self.do_execute(None)

    def do_execute(self, helper):
        images = self.generate_images()
        from copy import deepcopy
        control = deepcopy(images)

        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        crop = True
        result = self.alg.execute(images, air, roi, crop, helper)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        crop = False
        result = self.alg.execute(images, air, roi, crop, helper)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)


if __name__ == '__main__':
    unittest.main()
