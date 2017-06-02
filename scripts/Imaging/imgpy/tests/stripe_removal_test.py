from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th
from core.filters import stripe_removal


class StripeRemovalTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(StripeRemovalTest, self).__init__(*args, **kwargs)

    def test_not_executed(self):
        images, control = th.gen_img_shared_array_and_copy()

        wf = None
        ti = None
        sf = None
        result = stripe_removal.execute(images, wf, ti, sf)
        npt.assert_equal(result, control)

    def test_executed_wf(self):
        images, control = th.gen_img_shared_array_and_copy()

        wf = ["level=1"]
        ti = None
        sf = None
        result = stripe_removal.execute(images, wf, ti, sf)
        th.assert_not_equals(result, control)

    def test_executed_ti(self):
        images, control = th.gen_img_shared_array_and_copy()

        wf = None
        ti = ['nblock=2']
        sf = None
        result = stripe_removal.execute(images, wf, ti, sf)
        th.assert_not_equals(result, control)

    def test_executed_sf(self):
        images, control = th.gen_img_shared_array_and_copy()

        wf = None
        ti = None
        sf = ['size=5']
        result = stripe_removal.execute(images, wf, ti, sf)
        th.assert_not_equals(result, control)


if __name__ == '__main__':
    unittest.main()
