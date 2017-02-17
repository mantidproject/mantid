from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


def return_from_func(first_shared, add_arg):
    return first_shared[:] + add_arg


class ExclusiveMemTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(ExclusiveMemTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        self.h = Helper(r)

    def test_exec(self):
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
        from parallel import exclusive_mem as esm
        f = esm.create_partial(return_from_func, add_arg=add_arg)
        # execute parallel
        img = esm.execute(img, f, name="Exclusive mem test")
        # compare results
        th.assert_equals(img, expected)


if __name__ == '__main__':
    unittest.main()
