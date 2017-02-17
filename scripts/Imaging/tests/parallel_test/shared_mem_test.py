from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


def add_inplace(first_shared, add_arg=3):
    first_shared[:] = first_shared[:] + add_arg


def return_from_func(first_shared, add_arg):
    return first_shared[:] + add_arg


class SharedMemTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(SharedMemTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        self.h = Helper(r)

    def test_fwd_func_inplace(self):
        # create data as shared array
        img, orig = th.gen_img_shared_array_and_copy()
        add_arg = 5

        expected = img + add_arg
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]
        # create partial
        from parallel import shared_mem as psm
        f = psm.create_partial(
            add_inplace, fwd_function=psm.inplace_fwd_func, add_arg=add_arg)
        # execute parallel
        img = psm.execute(img, f, name="Inplace test")
        # compare results
        th.assert_equals(img, expected)

    def test_fwd_func(self):
        # create data as shared array
        img, orig = th.gen_img_shared_array_and_copy()
        add_arg = 5

        expected = img + add_arg
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]
        # create partial
        from parallel import shared_mem as psm
        f = psm.create_partial(
            return_from_func, fwd_function=psm.fwd_func, add_arg=add_arg)
        # execute parallel
        img = psm.execute(img, f, name="Fwd func test")
        # compare results
        th.assert_equals(img, expected)

# ------------------------- FAIL CASES -----------------------

    def test_fail_with_normal_array_fwd_func_inplace(self):
        # create data as normal nd array
        img = th.gen_img_numpy_rand()
        orig = th.deepcopy(img)
        add_arg = 5

        expected = img + add_arg

        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]

        # create partial
        from parallel import shared_mem as psm
        f = psm.create_partial(
            add_inplace, fwd_function=psm.inplace_fwd_func, add_arg=add_arg)
        # execute parallel
        res = psm.execute(img, f, name="Fail Inplace test")
        # compare results
        th.assert_not_equals(res, expected)
        th.assert_equals(img, orig)

    def test_fail_with_normal_array_fwd_func(self):
        # create data as shared array
        img = th.gen_img_numpy_rand()
        orig = th.deepcopy(img)
        add_arg = 5

        expected = img + add_arg
        assert expected[0, 0, 0] != img[0, 0, 0]
        assert expected[1, 0, 0] != img[1, 0, 0]
        assert expected[0, 4, 0] != img[0, 4, 0]
        assert expected[6, 0, 1] != img[6, 0, 1]
        # create partial
        from parallel import shared_mem as psm
        f = psm.create_partial(
            return_from_func, fwd_function=psm.fwd_func, add_arg=add_arg)
        # execute parallel
        res = psm.execute(img, f, name="Fwd func test")
        # compare results
        th.assert_not_equals(res, expected)
        th.assert_equals(img, orig)

if __name__ == '__main__':
    unittest.main()
