from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class HelperTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(HelperTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        self.h = Helper(r)

    def test_readme_caching(self):
        readme = []
        self.h.set_readme(readme)
        self.h.tomo_print("Testing verbosity at 0")
        # cache output even when verbosity is 0
        assert len(self.h._readme) > 0

    def test_pstop_raises_if_no_pstart(self):
        npt.assert_raises(ValueError, self.h.pstop, "dwad")


if __name__ == '__main__':
    unittest.main()
