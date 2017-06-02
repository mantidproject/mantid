from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
import helper as h


class HelperTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(HelperTest, self).__init__(*args, **kwargs)

    def test_readme_caching(self):
        readme = []
        h.set_readme(readme)
        h.tomo_print("Testing verbosity at 0")
        # cache output even when verbosity is 0
        assert len(h._readme) > 0

    def test_pstop_raises_if_no_pstart(self):
        npt.assert_raises(ValueError, h.pstop, "dwad")


if __name__ == '__main__':
    unittest.main()
