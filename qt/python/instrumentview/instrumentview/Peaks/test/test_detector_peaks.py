# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Peaks.Peak import Peak
from instrumentview.Peaks.WorkspaceDetectorPeaks import WorkspaceDetectorPeaks
from unittest import mock
import numpy as np
import unittest


class TestDetectorPeaks(unittest.TestCase):
    def test_detector_peaks(self):
        peak1 = Peak(1, 0, (4, 4, 4), 100, 10, 10, 10)
        peak2 = Peak(1, 1, (20, 4, 4), 1000, 5, 5, 5)
        detector_peaks = DetectorPeaks([peak1, peak2])
        self.assertEqual(1, detector_peaks.detector_id)
        self.assertEqual("[4, 4, 4] x 2", detector_peaks.label)

    def test_highest_d_spacing_used_for_label(self):
        peak1 = Peak(1, 0, (4, 4, 4), 100, 10, 5, 5)
        peak2 = Peak(1, 1, (20, 4, 4), 1000, 50, 5, 5)
        detector_peaks = DetectorPeaks([peak1, peak2])
        self.assertEqual(1, detector_peaks.detector_id)
        self.assertEqual("[20, 4, 4] x 2", detector_peaks.label)


class TestWorkspaceDetectorPeaks(unittest.TestCase):
    def _create_workspace_detector_peaks(self, detector_peaks: list[DetectorPeaks]) -> WorkspaceDetectorPeaks:
        """Create a WorkspaceDetectorPeaks with pre-set detector_peaks, bypassing ADS."""
        with mock.patch("instrumentview.Peaks.WorkspaceDetectorPeaks.AnalysisDataService") as mock_ads:
            mock_pws = mock.MagicMock()
            mock_pws.toDict.return_value = {"DetID": [], "h": [], "k": [], "l": [], "TOF": [], "DSpacing": [], "Wavelength": []}
            mock_ads.retrieve.return_value = mock_pws
            wdp = WorkspaceDetectorPeaks("dummy")
        wdp.detector_peaks = detector_peaks
        return wdp

    def test_get_positions_empty(self):
        wdp = self._create_workspace_detector_peaks([])
        positions = wdp.get_positions(np.array([[0, 0, 0]]), np.array([1]))
        self.assertEqual(0, len(positions))

    def test_get_positions(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 10, 10)
        peak2 = Peak(20, 1, (0, 1, 0), 200, 20, 20, 20)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        detector_positions = np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]])
        detector_ids = np.array([10, 20, 30])
        positions = wdp.get_positions(detector_positions, detector_ids)
        np.testing.assert_array_equal(positions, [[1, 1, 1], [2, 2, 2]])

    def test_get_x_values(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 5, 2)
        peak2 = Peak(20, 1, (0, 1, 0), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        x_values = wdp.get_x_values("TOF", [10, 20])
        self.assertEqual([100, 200], x_values)

    def test_get_x_values_filters_by_picked_detector_ids(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 5, 2)
        peak2 = Peak(20, 1, (0, 1, 0), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        x_values = wdp.get_x_values("TOF", [10])
        self.assertEqual([100], x_values)

    def test_get_x_values_empty_when_no_matching_detectors(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 5, 2)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1])])
        x_values = wdp.get_x_values("TOF", [99])
        self.assertEqual([], x_values)

    def test_get_labels_picked(self):
        peak1 = Peak(10, 0, (1, 2, 3), 100, 10, 5, 2)
        peak2 = Peak(20, 1, (4, 5, 6), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        labels = wdp.get_labels_picked([10, 20])
        self.assertEqual(["(1, 2, 3)", "(4, 5, 6)"], labels)

    def test_get_labels_picked_filters_by_detector_ids(self):
        peak1 = Peak(10, 0, (1, 2, 3), 100, 10, 5, 2)
        peak2 = Peak(20, 1, (4, 5, 6), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        labels = wdp.get_labels_picked([20])
        self.assertEqual(["(4, 5, 6)"], labels)


if __name__ == "__main__":
    unittest.main()
