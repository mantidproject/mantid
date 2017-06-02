from __future__ import (absolute_import, division, print_function)
import unittest
import numpy as np
import numpy.testing as npt
from tests import test_helper as th
from core.filters import normalise_by_air_region


class NormaliseByAirRegionTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(NormaliseByAirRegionTest, self).__init__(*args, **kwargs)

    def test_not_executed_empty_params(self):
        images, control = th.gen_img_shared_array_and_copy()

        air = None
        result = normalise_by_air_region.execute(images, air)
        npt.assert_equal(result, control)

    def test_not_executed_invalid_shape(self):
        images, control = th.gen_img_shared_array_and_copy()

        images = np.arange(100).reshape(10, 10)
        air = [3, 3, 4, 4]
        npt.assert_raises(ValueError, normalise_by_air_region.execute, images,
                          air)

    def test_executed_par(self):
        self.do_execute()

    def test_executed_seq(self):
        th.switch_mp_off()
        self.do_execute()
        th.switch_mp_on()

    def do_execute(self):
        images, control = th.gen_img_shared_array_and_copy()

        air = [3, 3, 4, 4]
        result = normalise_by_air_region.execute(images, air)
        th.assert_not_equals(result, control)


if __name__ == '__main__':
    unittest.main()
