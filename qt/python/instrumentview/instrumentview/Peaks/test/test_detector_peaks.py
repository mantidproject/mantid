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
        peak1 = Peak(1, 0, 0, (4, 4, 4), 100, 10, 10, 10)
        peak2 = Peak(1, 0, 1, (20, 4, 4), 1000, 5, 5, 5)
        detector_peaks = DetectorPeaks([peak1, peak2])
        self.assertEqual(1, detector_peaks.detector_id)
        self.assertEqual("[4, 4, 4] x 2", detector_peaks.label)

    def test_highest_d_spacing_used_for_label(self):
        peak1 = Peak(1, 0, 0, (4, 4, 4), 100, 10, 5, 5)
        peak2 = Peak(1, 0, 1, (20, 4, 4), 1000, 50, 5, 5)
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
            wdp = WorkspaceDetectorPeaks("dummy", mock_workspace, np.array([]))
        wdp.detector_peaks = detector_peaks
        return wdp

    def test_get_positions_and_labels_empty(self):
        wdp = self._create_workspace_detector_peaks([])
        positions_and_labels = wdp.get_positions_and_labels(np.array([[0, 0, 0]]), np.array([1]))
        # Should still return a tuple with two items
        self.assertEqual(2, len(positions_and_labels))

    def test_get_positions_and_labels(self):
        peak1 = Peak(10, 1, 0, (1, 0, 0), 100, 10, 10, 10)
        peak2 = Peak(20, 2, 1, (0, 1, 0), 200, 20, 20, 20)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        detector_positions = np.array([[1, 1, 1], [2, 2, 2], [3, 3, 3]])
        spectrum_nos = np.array([1, 2, 3])
        positions, labels = wdp.get_positions_and_labels(detector_positions, spectrum_nos)
        np.testing.assert_array_equal(positions, [[1, 1, 1], [2, 2, 2]])
        self.assertEqual(["(1, 0, 0)", "(0, 1, 0)"], labels)

    def test_get_x_values_and_labels(self):
        peak1 = Peak(10, 1, 0, (1, 0, 0), 100, 10, 5, 2)
        peak2 = Peak(20, 2, 1, (0, 1, 0), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        x_values, labels = wdp.get_x_values_and_labels("TOF", [1, 2])
        self.assertEqual([100, 200], x_values)
        self.assertEqual(["(1, 0, 0)", "(0, 1, 0)"], labels)

    def test_get_x_values_and_labels_filters_by_picked_spectrum_numbers(self):
        peak1 = Peak(10, 1, 0, (1, 0, 0), 100, 10, 5, 2)
        peak2 = Peak(20, 2, 1, (0, 1, 0), 200, 20, 15, 4)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1]), DetectorPeaks([peak2])])
        x_values, labels = wdp.get_x_values_and_labels("TOF", [1])
        self.assertEqual([100], x_values)
        self.assertEqual(["(1, 0, 0)"], labels)

    def test_get_x_values_and_labels_empty_when_no_matching_spectrum_numbers(self):
        peak1 = Peak(10, 1, 0, (1, 0, 0), 100, 10, 5, 2)
        wdp = self._create_workspace_detector_peaks([DetectorPeaks([peak1])])
        x_values, labels = wdp.get_x_values_and_labels("TOF", [99])
        self.assertEqual([], x_values)
        self.assertEqual([], labels)

    @mock.patch("instrumentview.Peaks.WorkspaceDetectorPeaks.AnalysisDataService")
    def test_peaks_with_spectrum_multiple_detectors(self, peaks_mock_ads):
        # We want to test the case where a peaks workspace has a
        # detector ID that is not in the model. This can happen when
        # an instrument has multiple detector IDs per spectrum. The
        # spectrum number should be used instead
        test_detector_id = 4
        test_spectrum_no = 1

        # Detector ID 4 is workspace index 2
        def mock_getIndicesFromDetectorIDs(ids):
            return [2]

        mock_ws = mock.MagicMock()
        mock_ws.getIndicesFromDetectorIDs = mock_getIndicesFromDetectorIDs

        mock_peaks_ws = mock.MagicMock()
        mock_peaks_ws.toDict.return_value = {
            "DetID": [test_detector_id],
            "h": [2],
            "k": [2],
            "l": [2],
            "DSpacing": [10],
            "Wavelength": [10],
            "TOF": [10],
        }
        peaks_mock_ads.retrieve.return_value = mock_peaks_ws

        # Everything on spectrum number 1
        spectrum_nos = np.array([test_spectrum_no, test_spectrum_no, test_spectrum_no])
        # Should get one peak with detector ID 4 and spectrum
        # number 1
        wdp = WorkspaceDetectorPeaks("dummy", mock_ws, spectrum_nos)
        self.assertEqual(1, len(wdp.detector_peaks))
        detector_peak = wdp.detector_peaks[0]
        self.assertEqual(1, len(detector_peak.peaks))
        self.assertEqual(test_detector_id, detector_peak.detector_id)
        self.assertEqual("(2, 2, 2)", detector_peak.label)
        self.assertEqual(test_spectrum_no, detector_peak.spectrum_no)
        single_peak = detector_peak.peaks[0]
        self.assertEqual(test_detector_id, single_peak.detector_id)
        self.assertEqual(test_spectrum_no, single_peak.spectrum_no)


if __name__ == "__main__":
    unittest.main()
