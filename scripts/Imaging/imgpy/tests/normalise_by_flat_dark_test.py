from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th
from core.filters import normalise_by_flat_dark


class NormaliseByFlatDarkTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(NormaliseByFlatDarkTest, self).__init__(*args, **kwargs)

    def test_not_executed_empty_params(self):
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        # empty params
        result = normalise_by_flat_dark.execute(images)
        npt.assert_equal(result, control)

    def test_not_executed_no_dark(self):
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        # no dark
        normalise_by_flat_dark.execute(images, flat[0])
        th.assert_equals(images, control)

    def test_not_executed_no_flat(self):
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        # no flat
        normalise_by_flat_dark.execute(images, None, dark[0])
        th.assert_equals(images, control)

    def test_not_executed_bad_flat(self):
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        # bad flat
        npt.assert_raises(ValueError, normalise_by_flat_dark.execute, images,
                          flat[0], dark)

    def test_not_executed_bad_dark(self):
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.gen_img_shared_array()[0]
        dark = th.gen_img_shared_array()[0]

        # bad dark
        npt.assert_raises(ValueError, normalise_by_flat_dark.execute, images,
                          flat, dark[0])

    def test_real_result_par(self):
        self.do_real_result()

    def test_real_result(self):
        th.switch_mp_off()
        self.do_real_result()
        th.switch_mp_on()

    def do_real_result(self):
        # the calculation here was designed on purpose to have a value
        # below the np.clip in normalise_by_flat_dark
        # the operation is (sample - dark) / (flat - dark)
        sample = th.gen_img_shared_array()
        sample[:] = 846.
        flat = th.gen_img_shared_array()[0]
        flat[:] = 306.
        dark = th.gen_img_shared_array()[0]
        dark[:] = 6.
        import numpy as np
        expected = np.full(sample.shape, 2.8)

        # we dont want anything to be cropped out
        res = normalise_by_flat_dark.execute(sample, flat, dark)
        npt.assert_almost_equal(res, expected, 7)

    def test_clip_works(self):
        # the calculation here was designed on purpose to have a value
        # ABOVE the np.clip in normalise_by_flat_dark
        # the operation is (sample - dark) / (flat - dark)
        sample = th.gen_img_shared_array()
        sample[:] = 846.
        flat = th.gen_img_shared_array()[0]
        flat[:] = 42.
        dark = th.gen_img_shared_array()[0]
        dark[:] = 6.
        import numpy as np
        expected = np.full(sample.shape, 3.)

        # we dont want anything to be cropped out
        res = normalise_by_flat_dark.execute(sample, flat, dark)
        npt.assert_equal(res, expected)


if __name__ == '__main__':
    unittest.main()
