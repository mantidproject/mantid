# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
import unittest
from unittest import mock


class TestFullInstrumentViewModel(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewModel", XUnit="TOF")

    def setUp(self):
        self._model = FullInstrumentViewModel(self._ws)

    def test_union_with_current_bin_min_max(self):
        current_min = self._model._bin_min
        current_max = self._model._bin_max
        self._model._union_with_current_bin_min_max(current_min - 1)
        self.assertEqual(self._model._bin_min, current_min - 1)
        self._model._union_with_current_bin_min_max(current_min)
        self.assertEqual(self._model._bin_min, current_min - 1)
        self.assertEqual(self._model._bin_max, current_max)
        self._model._union_with_current_bin_min_max(current_max + 1)
        self.assertEqual(self._model._bin_max, current_max + 1)

    def test_update_time_of_flight_range(self):
        integrated_spectra = list(range(len(self._ws.spectrumInfo())))
        self._model._workspace = mock.MagicMock()
        self._model._workspace.getIntegratedCountsForWorkspaceIndices.return_value = integrated_spectra
        self._model.update_time_of_flight_range(200, 10000, False)
        self._model._workspace.getIntegratedCountsForWorkspaceIndices.assert_called_once()
        self.assertEqual(min(integrated_spectra), self._model._data_min)
        self.assertEqual(max(integrated_spectra), self._model._data_max)

    # TODO: Add test for picked_detectors_info_text
    # TODO: Add tests for other picking methods

    @mock.patch("instrumentview.Projections.spherical_projection.spherical_projection")
    def test_calculate_spherical_projection(self, mock_spherical_projection):
        self._run_projection_test(mock_spherical_projection, True)

    @mock.patch("instrumentview.Projections.cylindrical_projection.cylindrical_projection")
    def test_calculate_cylindrical_projection(self, mock_cylindrical_projection):
        self._run_projection_test(mock_cylindrical_projection, False)

    def _run_projection_test(self, mock_projection_constructor, is_spherical):
        mock_projection = mock.MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2]]
        mock_projection_constructor.return_value = mock_projection
        points = self._model.calculate_projection(is_spherical, axis=[0, 1, 0])
        mock_projection_constructor.assert_called_once()
        self.assertTrue(all(all(point == [1, 2, 0]) for point in points))
