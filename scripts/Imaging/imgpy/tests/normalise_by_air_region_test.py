from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class NormaliseByAirRegionTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(NormaliseByAirRegionTest, self).__init__(*args, **kwargs)

        from filters import normalise_by_air_region
        self.alg = normalise_by_air_region

    def test_not_executed_no_crop_before_norm(self):
        # Duplicated code because 'nose.main()' failed to run it properly
        crop = False
        images, control = th.gen_img_shared_array_and_copy()

        # empty params
        air = None
        roi = None
        result = self.alg.execute(images, air, roi, crop)
        npt.assert_equal(result, control)

        # invalid air region type
        air = '[1,1,1,1]'
        roi = [1, 1, 8, 8]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

        # invalid roi region type
        roi = '[1,2,3,4]'
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

        # invalid data shape
        import numpy as np
        images = np.arange(100).reshape(10, 10)
        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

    def test_not_executed_with_crop_before_norm(self):
        # Duplicated code because 'nose.main()' failed to run it properly
        crop = True
        images, control = th.gen_img_shared_array_and_copy()

        # empty params
        air = None
        roi = None
        result = self.alg.execute(images, air, roi, crop)
        npt.assert_equal(result, control)

        # invalid air region type
        air = '[1,1,1,1]'
        roi = [1, 1, 8, 8]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

        # invalid air region coords
        air = [13, 13, 13, 31]
        roi = [1, 1, 8, 8]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

        # invalid roi region type
        roi = '[1,2,3,4]'
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

        # invalid data type
        import numpy as np
        images = np.arange(100).reshape(10, 10)
        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, self.alg.execute, images, air, roi, crop)

    def test_executed_par(self):
        self.do_execute()

    def test_executed_seq(self):
        th.switch_mp_off()
        self.do_execute()
        th.switch_mp_on()

    def do_execute(self):
        images, control = th.gen_img_shared_array_and_copy()

        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        crop = True
        result = self.alg.execute(images, air, roi, crop)
        th.assert_not_equals(result, control)

        images, control = th.gen_img_shared_array_and_copy()

        roi = [1, 1, 8, 8]
        air = [3, 3, 4, 4]
        crop = False
        result = self.alg.execute(images, air, roi, crop)
        th.assert_not_equals(result, control)


if __name__ == '__main__':
    unittest.main()
