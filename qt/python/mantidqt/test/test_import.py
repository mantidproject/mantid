from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest


class ImportTest(unittest.TestCase):

    def test_import(self):
        import mantidqt # noqa

if __name__ == "__main__":
    unittest.main()
