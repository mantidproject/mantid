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
        peak1 = Peak(1, location, (4, 4, 4), 100, 10)
        peak2 = Peak(1, location, (20, 4, 4), 1000, 5)
        detector_peaks = DetectorPeaks([peak1, peak2])
        self.assertEqual(1, detector_peaks.detector_id)
        np.testing.assert_almost_equal(location, detector_peaks.location)
        self.assertTrue("[4, 4, 4],[20, 4, 4]", detector_peaks.label)


if __name__ == "__main__":
    unittest.main()
