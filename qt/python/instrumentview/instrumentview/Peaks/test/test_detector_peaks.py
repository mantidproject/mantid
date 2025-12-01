# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Peaks.Peak import Peak
import numpy as np
import unittest


class TestDetectorPeaks(unittest.TestCase):
    def test_detector_peaks(self):
        location = np.array([1, 2, 3.0])
        peak1 = Peak(1, 5, location, (4, 4, 4), 100, 10, 10, 10)
        peak2 = Peak(1, 5, location, (20, 4, 4), 1000, 5, 5, 5)
        detector_peaks = DetectorPeaks([peak1, peak2])
        self.assertEqual(1, detector_peaks.detector_id)
        np.testing.assert_almost_equal(location, detector_peaks.location)
        self.assertEqual("[4, 4, 4] x 2", detector_peaks.label)

    def test_highest_d_spacing_used_for_label(self):
        location = np.array([1, 2, 3.0])
        peak1 = Peak(1, 5, location, (4, 4, 4), 100, 10, 5, 5)
        peak2 = Peak(1, 5, location, (20, 4, 4), 1000, 50, 5, 5)
        detector_peaks = DetectorPeaks([peak1, peak2])
        self.assertEqual(1, detector_peaks.detector_id)
        np.testing.assert_almost_equal(location, detector_peaks.location)
        self.assertEqual("[20, 4, 4] x 2", detector_peaks.label)


if __name__ == "__main__":
    unittest.main()
