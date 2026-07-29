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
            mock_workspace = mock.MagicMock()
            mock_workspace.getIndicesFromDetectorIDs.return_value = np.array([], dtype=int)
            wdp = WorkspaceDetectorPeaks("dummy", "TOF", (0, 1e10))
        wdp.detector_peaks = detector_peaks
        return wdp

    def test_get_positions_and_labels_empty(self):
        wdp = self._create_workspace_detector_peaks([])
        positions_and_labels = wdp.get_positions_and_labels(np.array([[0, 0, 0]]), np.array([1]))
        # Should still return a tuple with two items
        self.assertEqual(2, len(positions_and_labels))

    def test_get_positions_and_labels(self):
        peak1 = Peak(1, 0, (1, 0, 0), 100, 10, 10, 10)
        peak2 = Peak(2, 1, (0, 1, 0), 200, 20, 20, 20)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        detector_positions = np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]])
        detector_ids = np.array([1, 2, 3])
        positions, labels = wdp.get_positions_and_labels(detector_positions, detector_ids)
        np.testing.assert_array_equal(positions, [[1, 1, 1], [2, 2, 2]])
        self.assertEqual(["(1, 0, 0)", "(0, 1, 0)"], labels)

    def test_get_x_values_and_labels(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 5, 2)
        peak2 = Peak(20, 1, (0, 1, 0), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        peaks = wdp.get_x_values_and_labels([10, 20])
        self.assertEqual([100, 200], [peak.tof for peak in peaks])
        self.assertEqual(["(1, 0, 0)", "(0, 1, 0)"], [peak.label for peak in peaks])

    def test_get_x_values_and_labels_filters_by_picked_detector_ids(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 5, 2)
        peak2 = Peak(20, 1, (0, 1, 0), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        peaks = wdp.get_x_values_and_labels([10])
        self.assertEqual([100], [peak.tof for peak in peaks])
        self.assertEqual(["(1, 0, 0)"], [peak.label for peak in peaks])

    def test_get_x_values_and_labels_empty_when_no_matching_spectrum_numbers(self):
        peak1 = Peak(10, 0, (1, 0, 0), 100, 10, 5, 2)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1])])
        peaks = wdp.get_x_values_and_labels([99])
        self.assertEqual([], peaks)

    def test_get_positions_and_labels_peak_id_larger_than_all_detector_ids(self):
        # Peak detector ID (999) is larger than every entry in detector_ids.
        # np.searchsorted would return an out-of-bounds index; the valid mask must
        # filter it out so the result is empty rather than raising an IndexError.
        peak = Peak(999, 0, (1, 0, 0), 100, 10, 10, 10)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak])])
        detector_positions = np.array([[1, 1, 1], [2, 2, 2]])
        detector_ids = np.array([1, 2])
        positions, labels = wdp.get_positions_and_labels(detector_positions, detector_ids)
        self.assertEqual(0, len(positions))

    @mock.patch("instrumentview.Peaks.WorkspaceDetectorPeaks.AnalysisDataService")
    def test_peaks_read_from_ads_workspace(self, peaks_mock_ads):
        test_detector_id = 4

        mock_peaks_ws = mock.MagicMock()
        mock_peaks_ws.toDict.return_value = {
            "DetID": [4],
            "h": [2],
            "k": [2],
            "l": [2],
            "DSpacing": [10],
            "Wavelength": [10],
            "TOF": [10],
        }
        peaks_mock_ads.retrieve.return_value = mock_peaks_ws

        wdp = WorkspaceDetectorPeaks("dummy", "TOF", (0, 100))
        self.assertEqual(1, len(wdp.detector_peaks))
        detector_peak = wdp.detector_peaks[0]
        self.assertEqual(1, len(detector_peak.peaks))
        self.assertEqual(test_detector_id, detector_peak.detector_id)
        self.assertEqual("(2, 2, 2)", detector_peak.label)
        single_peak = detector_peak.peaks[0]
        self.assertEqual(test_detector_id, single_peak.detector_id)

    @mock.patch("instrumentview.Peaks.WorkspaceDetectorPeaks.AnalysisDataService")
    def test_peaks_outside_limits_are_excluded(self, peaks_mock_ads):
        mock_peaks_ws = mock.MagicMock()
        mock_peaks_ws.toDict.return_value = {
            "DetID": [1, 2],
            "h": [1, 2],
            "k": [0, 0],
            "l": [0, 0],
            "DSpacing": [5, 5],
            "Wavelength": [5, 5],
            "TOF": [50, 200],
        }
        peaks_mock_ads.retrieve.return_value = mock_peaks_ws

        wdp = WorkspaceDetectorPeaks("dummy", "TOF", (0, 100))
        self.assertEqual(1, len(wdp.detector_peaks))
        self.assertEqual(1, wdp.detector_peaks[0].detector_id)


if __name__ == "__main__":
    unittest.main()
