from mantid.simpleapi import mtd, Load, IndexSatellitePeaks
import unittest
import numpy as np
import numpy.testing as npt


class IndexSatellitePeaksTest(unittest.TestCase):

    def setUp(self):
        # Need to set the random seed because the scipy kmeans algorithm
        # randomly initilizes the starting centroids.
        np.random.seed(100)
        self._nuclear_peaks = Load("WISH_peak_hkl_small.nxs", OutputWorkspace="nuclear_peaks")
        self._peaks = Load("refine_satellites_fixed_q_test.nxs", OutputWorkspace="peaks")

    def tearDown(self):
        mtd.clear()

    def test_exec_with_cluster_threshold(self):
        expected_values = np.array([-1, -1, 1, 1])
        indexed_peaks = IndexSatellitePeaks(self._nuclear_peaks, self._peaks,
                                            Tolerance=0.1,
                                            ClusterThreshold=1.5, NumOfQs=-1)
        index_values = np.array(indexed_peaks.column("m1"))

        npt.assert_array_equal(index_values, expected_values)
        self.assertRaises(RuntimeError, indexed_peaks.column, "m2")

    def test_exec_with_number_of_qs(self):
        expected_values = np.array([-1, -1, 1, 1])
        indexed_peaks = IndexSatellitePeaks(self._nuclear_peaks, self._peaks,
                                            Tolerance=0.1, NumOfQs=2)
        index_values = np.array(indexed_peaks.column("m1"))

        npt.assert_array_equal(index_values, expected_values)
        self.assertRaises(RuntimeError, indexed_peaks.column, "m2")

if __name__ == "__main__":
    unittest.main()
