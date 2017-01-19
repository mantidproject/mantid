from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class TestClass(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(TestClass, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        self.h = Helper(r)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.full((10, 10, 10), 1., dtype=np.float32)

    def test_sample(self):
        pass

if __name__ == '__main__':
    unittest.main()
