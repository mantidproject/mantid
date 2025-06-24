# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
import unittest
from unittest import mock
import numpy as np


class MockPosition:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def getSpherical(self):
        return [self.x, self.y, self.z]

    def __array__(self):
        return np.array([self.x, self.y, self.z])


class TestFullInstrumentViewModel(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewModel", XUnit="TOF")

    def _create_mock_workspace(self, detector_ids: list[int]):
        mock_workspace = mock.MagicMock()
        mock_workspace.detectorInfo.return_value = mock.MagicMock()
        mock_workspace.componentInfo.return_value = mock.MagicMock()
        mock_workspace.getNumberHistograms.return_value = len(detector_ids)
        mock_workspace.detectorInfo().detectorIDs.return_value = detector_ids
        mock_workspace.detectorInfo().isMonitor.return_value = False
        mock_workspace.detectorInfo().position.side_effect = lambda i: MockPosition(i, i, i)
        mock_workspace.dataX.return_value = list(range(len(detector_ids)))
        mock_workspace.getDetectorIDToWorkspaceIndexMap.return_value = {id: i for i, id in enumerate(detector_ids)}
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100 * i for i in detector_ids]
        return mock_workspace

    def test_union_with_current_bin_min_max(self):
        model = FullInstrumentViewModel(self._ws)
        current_min = model._bin_min
        current_max = model._bin_max
        model._union_with_current_bin_min_max(current_min - 1)
        self.assertEqual(model._bin_min, current_min - 1)
        model._union_with_current_bin_min_max(current_min)
        self.assertEqual(model._bin_min, current_min - 1)
        self.assertEqual(model._bin_max, current_max)
        model._union_with_current_bin_min_max(current_max + 1)
        self.assertEqual(model._bin_max, current_max + 1)

    def test_update_time_of_flight_range(self):
        model = FullInstrumentViewModel(self._ws)
        integrated_spectra = list(range(len(self._ws.spectrumInfo())))
        model._workspace = mock.MagicMock()
        model._workspace.getIntegratedCountsForWorkspaceIndices.return_value = integrated_spectra
        model.update_time_of_flight_range(200, 10000, False)
        model._workspace.getIntegratedCountsForWorkspaceIndices.assert_called_once()
        self.assertEqual(min(integrated_spectra), model._data_min)
        self.assertEqual(max(integrated_spectra), model._data_max)

    # TODO: Add test for picked_detectors_info_text
    # TODO: Add tests for other picking methods

    def test_detectors_with_no_spectra(self):
        mock_workspace = self._create_mock_workspace([1, 20, 300])
        mock_workspace.getDetectorIDToWorkspaceIndexMap.return_value = {1: 0, 20: 1, 300: -1}
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200]
        model = FullInstrumentViewModel(mock_workspace)
        np.testing.assert_array_equal(model._detector_ids, [1, 20, 300])
        np.testing.assert_array_equal(model._is_valid, [True, True, False])
        np.testing.assert_array_equal(model._counts, [100, 200, 0])

    @mock.patch("instrumentview.Projections.spherical_projection.spherical_projection")
    def test_calculate_spherical_projection(self, mock_spherical_projection):
        self._run_projection_test(mock_spherical_projection, True)

    @mock.patch("instrumentview.Projections.cylindrical_projection.cylindrical_projection")
    def test_calculate_cylindrical_projection(self, mock_cylindrical_projection):
        self._run_projection_test(mock_cylindrical_projection, False)

    def _run_projection_test(self, mock_projection_constructor, is_spherical):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        mock_projection = mock.MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_constructor.return_value = mock_projection
        points = model.calculate_projection(is_spherical, axis=[0, 1, 0])
        mock_projection_constructor.assert_called_once()
        self.assertTrue(all(all(point == [1, 2, 0]) for point in points))

    def test_sample_position(self):
        expected_position = np.array([1.0, 2.0, 1.0])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.componentInfo().samplePosition.return_value = expected_position
        model = FullInstrumentViewModel(mock_workspace)
        np.testing.assert_array_equal(model.sample_position(), expected_position)

    def test_source_position(self):
        expected_position = np.array([0.5, 1.0, 0.2])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.componentInfo().sourcePosition.return_value = expected_position
        model = FullInstrumentViewModel(mock_workspace)
        np.testing.assert_array_equal(model._source_position, expected_position)

    def test_detector_counts(self):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        expected_counts = np.array([100, 200, 300])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = expected_counts
        model = FullInstrumentViewModel(mock_workspace)
        expected_counts = np.array([100, 200, 300])
        np.testing.assert_array_equal(model.detector_counts(), expected_counts)

    def test_detector_ids(self):
        expected_ids = np.array([1, 2, 3])
        mock_workspace = self._create_mock_workspace(expected_ids)
        model = FullInstrumentViewModel(mock_workspace)
        np.testing.assert_array_equal(model.detector_ids(), expected_ids)

    def test_data_limits(self):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200, 300]
        model = FullInstrumentViewModel(mock_workspace)
        self.assertEqual(model.data_limits()[0], 100)
        self.assertEqual(model.data_limits()[1], 300)

    def test_bin_limits(self):
        bins = [0, 1, 2]
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.dataX.return_value = bins
        model = FullInstrumentViewModel(mock_workspace)
        self.assertEqual(model.bin_limits()[0], bins[0])
        self.assertEqual(model.bin_limits()[1], bins[-1])

    def test_monitor_positions(self):
        def mock_is_monitor(index):
            return index % 2 == 0

        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.detectorInfo().isMonitor = mock_is_monitor
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100]
        model = FullInstrumentViewModel(mock_workspace)
        monitor_positions = model.monitor_positions()
        self.assertEqual(len(monitor_positions), 2)
