from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class CircularMaskTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(CircularMaskTest, self).__init__(*args, **kwargs)

        from core.filters import circular_mask
        self.alg = circular_mask

    def test_not_executed(self):
        # Check that the filter is not executed when:
        #     - no ratio is provided
        #     - 0 < ratio < 1 is false
        images, control = th.gen_img_shared_array_and_copy()

        ratio = 0
        mask_val = 0.
        result = self.alg.execute(images, ratio, mask_val)
        npt.assert_equal(result, control)

        ratio = 1
        result = self.alg.execute(images, ratio, mask_val)
        npt.assert_equal(result, control)

        ratio = -1
        result = self.alg.execute(images, ratio, mask_val)
        npt.assert_equal(result, control)

        ratio = None
        result = self.alg.execute(images, ratio, mask_val)
        npt.assert_equal(result, control)

    def test_executed_no_helper(self):
        images, control = th.gen_img_shared_array_and_copy()

        ratio = 0.001

        result = self.alg.execute(images, ratio)
        npt.assert_raises(AssertionError, npt.assert_array_equal, result,
                          control)

        # reset the input images
        images, control = th.gen_img_shared_array_and_copy()
        ratio = 0.994
        result = self.alg.execute(images, ratio)
        npt.assert_raises(AssertionError, npt.assert_array_equal, result,
                          control)


if __name__ == '__main__':
    unittest.main()
