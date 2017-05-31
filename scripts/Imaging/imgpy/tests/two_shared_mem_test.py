from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


def add_inplace(first_shared, second_shared, add_arg):
    # it's not the same as 
    # first_shared[:] += second_shared + add_arg
    # as then the check fails versus the expected one
    first_shared[:] = first_shared + second_shared + add_arg


def return_from_func(first_shared, second_shared, add_arg):
    return first_shared[:] + second_shared[:] + add_arg


class TwoSharedMemTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(TwoSharedMemTest, self).__init__(*args, **kwargs)

    def test_fwd_func_inplace(self):
        # create data as shared array
        img = th.gen_img_shared_array()
        img2nd, orig_2nd = th.gen_img_shared_array_and_copy()

        # make sure it hasnt changed the original array
        expected = img + img2nd + 5
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]

        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            add_inplace, fwd_function=ptsm.inplace_fwd_func, add_arg=5)

        # execute parallel
        ptsm.execute(img, img2nd, f, name="Inplace test")

        # compare results
        th.assert_equals(img, expected)
        th.assert_equals(img2nd, orig_2nd)

    def test_fwd_func_second_2d(self):
        # create data as shared array
        img = th.gen_img_shared_array()
        img2nd, orig_2nd = th.gen_img_shared_array_and_copy()

        img2nd = img2nd[0]

        # make sure it hasnt changed the original array
        expected = img + img2nd + 5
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]
        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            add_inplace,
            fwd_function=ptsm.inplace_fwd_func_second_2d,
            add_arg=5)
        # execute parallel
        ptsm.execute(img, img2nd, f, name="Second 2D test")
        # compare results
        th.assert_equals(img, expected)
        th.assert_equals(img2nd, orig_2nd[0])

    def test_fwd_func_return_to_first(self):
        # create data as shared array
        img = th.gen_img_shared_array()
        img2nd, orig_2nd = th.gen_img_shared_array_and_copy()

        # make sure it hasnt changed the original array
        expected = img + img2nd + 5
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]
        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            return_from_func,
            fwd_function=ptsm.fwd_func_return_to_first,
            add_arg=5)
        # execute parallel
        res1, res2 = ptsm.execute(img, img2nd, f, name="Return to first test")
        # compare results
        th.assert_equals(res1, expected)
        th.assert_equals(res2, orig_2nd)

    def test_fwd_func_return_to_second(self):
        # create data as shared array
        img, orig_img = th.gen_img_shared_array_and_copy()
        img2nd = th.gen_img_shared_array()

        # make sure it hasnt changed the original array
        expected = img + img2nd + 5
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]
        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            return_from_func,
            fwd_function=ptsm.fwd_func_return_to_second,
            add_arg=5)
        # execute parallel
        res1, res2 = ptsm.execute(img, img2nd, f, name="Return to second test")
        # compare results
        th.assert_equals(res2, expected)
        th.assert_equals(res1, orig_img)

# ------------------------- FAIL CASES -----------------------

    def test_fail_with_normal_array_fwd_func_inplace(self):
        # create data as normal nd array
        img = th.gen_img_numpy_rand()
        orig_img = th.deepcopy(img)
        img2nd = th.gen_img_numpy_rand()
        orig_img2nd = th.deepcopy(img2nd)

        # get the expected as usual
        expected = img + img2nd

        # make sure it hasnt changed the original array
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]

        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(add_inplace, add_arg=5)
        # execute parallel
        ptsm.execute(img, img2nd, f, name="Fail Inplace test")
        # compare results
        th.assert_not_equals(img, expected)
        th.assert_not_equals(img2nd, expected)
        th.assert_equals(img, orig_img)
        th.assert_equals(img2nd, orig_img2nd)

    def test_fail_with_normal_array_fwd_func_second_2d(self):
        # create data as normal nd array
        img = th.gen_img_numpy_rand()
        orig_img = th.deepcopy(img)
        img2nd = th.gen_img_numpy_rand()
        orig_img2nd = th.deepcopy(img2nd)

        img2nd = img2nd[0]

        # get the expected as usual
        expected = img + img2nd

        # make sure it hasnt changed the original array
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]

        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            add_inplace,
            fwd_function=ptsm.inplace_fwd_func_second_2d,
            add_arg=5)
        # execute parallel
        ptsm.execute(img, img2nd, f, name="Fail Second 2D test")
        # compare results
        th.assert_not_equals(img, expected)
        th.assert_not_equals(img2nd, expected)
        th.assert_equals(img, orig_img)
        th.assert_equals(img2nd, orig_img2nd[0])

    def test_fail_with_normal_array_return_to_first(self):
        # create data as normal nd array
        img = th.gen_img_numpy_rand()
        img2nd = th.gen_img_numpy_rand()

        # get the expected as usual
        expected = img + img2nd

        # make sure it hasnt changed the original array
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]

        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            return_from_func,
            fwd_function=ptsm.fwd_func_return_to_first,
            add_arg=5)
        # execute parallel
        orig_2nd = th.deepcopy(img2nd)
        res1, res2 = ptsm.execute(
            img, img2nd, f, name="Fail Return to first test")
        # compare results
        th.assert_equals(res1, img)
        th.assert_equals(res2, img2nd)
        th.assert_not_equals(res1, expected)

    def test_fail_with_normal_array_return_to_second(self):
        """
        This test does not use shared arrays and will not change the data.
        This behaviour is intended and is 
        """
        # create data as normal nd array
        img = th.gen_img_numpy_rand()
        img2nd = th.gen_img_numpy_rand()

        # get the expected as usual
        expected = img + img2nd

        # make sure it hasnt changed the original array
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]

        # create partial
        from parallel import two_shared_mem as ptsm
        f = ptsm.create_partial(
            return_from_func,
            fwd_function=ptsm.fwd_func_return_to_second,
            add_arg=5)
        # execute parallel
        orig_1st = th.deepcopy(img)
        res1, res2 = ptsm.execute(
            img, img2nd, f, name="Fail Return to second test")
        # compare results
        th.assert_equals(res1, img)
        th.assert_equals(res2, img2nd)
        th.assert_not_equals(res2, expected)

if __name__ == '__main__':
    unittest.main()
