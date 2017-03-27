from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class HelperTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(HelperTest, self).__init__(*args, **kwargs)

        import helper as h
        self.h = h

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
