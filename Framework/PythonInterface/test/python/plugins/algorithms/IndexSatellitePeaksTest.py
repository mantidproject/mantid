from mantid.simpleapi import mtd, Load, IndexSatellitePeaks
import unittest
import numpy as np
import numpy.testing as npt


class IndexSatellitePeaksTest(unittest.TestCase):

    def setUp(self):
        # Need to set the random seed because the scipy kmeans algorithm
        # randomly initilizes the starting centroids. This can lead to a
        # different but equivilent indexing.
        np.random.seed(1)

    def tearDown(self):
        mtd.clear()

    def test_exec(self):
        expected_values = np.array([1, 1, -1, -1])
        peaks = Load("refine_satellites_fixed_q_test.nxs")
        indexed_peaks = IndexSatellitePeaks(peaks, tolerance=0.1, NumOfQs=2)
        index_values = np.array(indexed_peaks.column("m1"))

        npt.assert_array_equal(index_values, expected_values)
        self.assertRaises(RuntimeError, indexed_peaks.column, "m2")


if __name__ == "__main__":
    unittest.main()
