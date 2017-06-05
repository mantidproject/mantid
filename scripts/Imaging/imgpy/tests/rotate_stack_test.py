from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class RotateStackTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(RotateStackTest, self).__init__(*args, **kwargs)

        from filters import rotate_stack
        self.alg = rotate_stack

    def test_not_executed(self):
        # only works on square images
        images, control = th.gen_img_shared_array_and_copy((10, 10, 10))
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        # empty params
        result = self.alg.execute(images, None)[0]
        npt.assert_equal(result, control)

    def test_executed_par(self):
        self.do_execute()

    def test_executed_seq(self):
        th.switch_mp_off()
        self.do_execute()
        th.switch_mp_on()

    def do_execute(self):
        # only works on square images
        images, control = th.gen_img_shared_array_and_copy((10, 10, 10))
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        rotation = 1  # once clockwise
        images[:, 0, 0] = 42  # set all images at 0,0 to 42
        result = self.alg.execute(images, rotation)[0]
        h = result.shape[1]
        w = result.shape[2]
        npt.assert_equal(result[:, 0, w - 1], 42.0)


if __name__ == '__main__':
    unittest.main()
