from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class NormaliseByFlatDarkTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(NormaliseByFlatDarkTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        from recon.filters import normalise_by_flat_dark
        self.alg = normalise_by_flat_dark

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.random.rand(10, 10, 10), np.full((10, 10), 0.9), np.full((10, 10), 0.1)

    def test_not_executed(self):
        sample, flat, dark = self.generate_images()
        from copy import deepcopy
        control = deepcopy(sample)
        err_msg = "TEST NOT EXECUTED :: Running normalise_by_flat_dark with size {0}, mode {1} and order {2} changed the data!"

        # empty params
        result = self.alg.execute(sample, h=self.h)
        npt.assert_equal(result, control)

        # bad flat nd array
        npt.assert_raises(ValueError, self.alg.execute,
                          sample, flat[0], h=self.h)

    def test_executed(self):
        self.do_execute(self.h)

    def test_executed_no_helper(self):
        self.do_execute(None)

    def do_execute(self, helper):
        sample, flat, dark = self.generate_images()
        from copy import deepcopy
        control = deepcopy(sample)
        import numpy as np

        result = self.alg.execute(sample, flat, dark, 0, 1.5, h=helper)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        control = np.full((10, 10, 10), 2.)
        # clip min, should return twos
        result = self.alg.execute(sample, flat, dark, 2., None, h=helper)
        npt.assert_equal(result, control)

        control = np.zeros(1000).reshape(10, 10, 10)
        # clip max, should also be zeros
        result = self.alg.execute(sample, flat, dark, -3, 0, h=helper)
        npt.assert_equal(result, control)

if __name__ == '__main__':
    unittest.main()
