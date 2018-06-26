from mantid.simpleapi import mtd
import unittest


class IndexSatellitePeaksTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def test_wrong_input(self):
        self.fail()


if __name__ == "__main__":
    unittest.main()
