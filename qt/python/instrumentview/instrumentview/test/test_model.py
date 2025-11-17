# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace, CreatePeaksWorkspace
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


class PeaksWorkspaceMock(mock.MagicMock):
    pass


class TestFullInstrumentViewModel(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewModel", XUnit="TOF")

    def setUp(self) -> None:
        patcher = mock.patch("instrumentview.FullInstrumentViewModel.CreateDetectorTable")
        self.addCleanup(patcher.stop)
        mock_create_det_table = patcher.start()
        self._mock_table = mock.MagicMock()
        mock_create_det_table.return_value = self._mock_table

    def _mock_detector_table(self, detector_ids: list[int], spectrum_no: np.ndarray = np.array([]), monitors: np.ndarray = np.array([])):
        if monitors.size == 0:
            monitors = np.array(["no" for _ in detector_ids])

        if spectrum_no.size == 0:
            spectrum_no = np.array([i for i in range(len(detector_ids))])

        table_columns = {
            "Detector ID(s)": np.array([int(id) for id in detector_ids]),
            "Position": np.array([[i, i, i] for i in range(len(detector_ids))]),
            "R": np.array([i for i in range(len(detector_ids))]),
            "Theta": np.array([i for i in range(len(detector_ids))]),
            "Phi": np.array([i for i in range(len(detector_ids))]),
            "Index": np.array([i for i in range(len(detector_ids))]),
            "Monitor": monitors,
            "Spectrum No": spectrum_no,
        }
        self._mock_table.columnArray.side_effect = lambda x: table_columns[x]
        return

    def _create_mock_workspace(self, detector_ids: list[int]):
        mock_workspace = mock.MagicMock()
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.isCommonBins.return_value = False
        mock_workspace.detectorInfo.return_value = mock.MagicMock()
        mock_workspace.componentInfo.return_value = mock.MagicMock()
        mock_workspace.getNumberHistograms.return_value = len(detector_ids)
        mock_workspace.extractX.return_value = np.tile(np.arange(len(detector_ids)), (len(detector_ids), 1))
        mock_workspace.readX.return_value = np.arange(len(detector_ids))
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100 * i for i in detector_ids]
        return mock_workspace

    def test_update_integration_range(self):
        self._mock_detector_table(list(range(self._ws.getNumberHistograms())))
        model = FullInstrumentViewModel(self._ws)
        model.setup()
        integrated_spectra = list(range((self._ws.getNumberHistograms())))
        mock_workspace = mock.MagicMock()
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = integrated_spectra
        model._workspace = mock_workspace
        model.update_integration_range((200, 10000), False)
        model._workspace.getIntegratedCountsForWorkspaceIndices.assert_called_once()
        self.assertEqual(min(integrated_spectra), model._counts_limits[0])
        self.assertEqual(max(integrated_spectra), model._counts_limits[1])

    @mock.patch("instrumentview.FullInstrumentViewModel.DetectorInfo")
    def test_picked_detectors_info_text(self, det_info_mock):
        self._mock_detector_table([1, 20, 300])
        mock_workspace = self._create_mock_workspace([1, 20, 300])
        mock_workspace.getDetector.side_effect = lambda i: mock.MagicMock(
            getName=mock.Mock(return_value=str(i)), getFullName=mock.Mock(return_value=f"Full_{i}")
        )
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        model._is_valid = np.array([False, True, True])
        model._detector_is_picked = np.array([False, True])
        model.picked_detectors_info_text()
        # Test each argument one by one because of array equality
        mock_args, mock_kwargs = det_info_mock.call_args
        self.assertEqual(mock_args[0], "2")
        self.assertEqual(mock_args[1], np.int64(300))
        self.assertEqual(mock_args[2], np.int64(2))
        np.testing.assert_equal(mock_args[3], [2, 2, 2])
        np.testing.assert_equal(mock_args[4], [2, 2, 2])
        self.assertEqual(mock_args[5], "Full_2")
        self.assertEqual(mock_args[6], 30000)

    def test_negate_picked_visibility(self):
        self._mock_detector_table(list(range(self._ws.getNumberHistograms())))
        model = FullInstrumentViewModel(self._ws)
        model.setup()
        model._detector_is_picked = np.array([False, False, False])
        model.negate_picked_visibility([1, 2])
        np.testing.assert_equal(model._detector_is_picked, [False, True, True])

    def test_clear_all_picked_detectors(self):
        self._mock_detector_table(list(range(self._ws.getNumberHistograms())))
        model = FullInstrumentViewModel(self._ws)
        model.setup()
        model._detector_is_picked = np.array([False, True, True])
        model.clear_all_picked_detectors()
        np.testing.assert_equal(model._detector_is_picked, [False, False, False])

    def test_detectors_with_no_spectra(self):
        self._mock_detector_table([1, 20, 300, 400], monitors=np.array(["no", "no", "n/a", "yes"]))
        mock_workspace = self._create_mock_workspace([1, 20, 300, 400])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200]
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model._detector_ids, [1, 20, 300, 400])
        np.testing.assert_array_equal(model._is_valid, [True, True, False, False])
        np.testing.assert_array_equal(model._counts, [100, 200, 0, 0])

    @mock.patch("instrumentview.FullInstrumentViewModel.SphericalProjection")
    def test_calculate_spherical_projection(self, mock_spherical_projection):
        self._run_projection_test(mock_spherical_projection, FullInstrumentViewModel._SPHERICAL_Y)

    @mock.patch("instrumentview.FullInstrumentViewModel.CylindricalProjection")
    def test_calculate_cylindrical_projection(self, mock_cylindrical_projection):
        self._run_projection_test(mock_cylindrical_projection, FullInstrumentViewModel._CYLINDRICAL_Y)

    @mock.patch("instrumentview.FullInstrumentViewModel.SideBySide")
    def test_calculate_side_by_side_projection(self, mock_side_by_side):
        self._run_projection_test(mock_side_by_side, FullInstrumentViewModel._SIDE_BY_SIDE)

    def _run_projection_test(self, mock_projection_constructor, projection_option):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        mock_projection = mock.MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_constructor.return_value = mock_projection
        points = model.calculate_projection(projection_option, axis=[0, 1, 0], positions=model.detector_positions)
        mock_projection_constructor.assert_called_once()
        self.assertTrue(all(all(point == [1, 2, 0]) for point in points))

    def test_sample_position(self):
        expected_position = np.array([1.0, 2.0, 1.0])
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.componentInfo().samplePosition.return_value = expected_position
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model.sample_position, expected_position)

    def test_detector_positions(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model.detector_positions, [[0, 0, 0], [1, 1, 1], [2, 2, 2]])

    def test_picked_detector_ids(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        model._is_valid = np.array([False, True, True])
        model._detector_is_picked = np.array([False, True])
        self.assertEqual(model.picked_detector_ids, [3])

    def test_picked_workspace_indices(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        model._is_valid = np.array([False, True, True])
        model._detector_is_picked = np.array([False, True])
        self.assertEqual(model.picked_workspace_indices, [2])

    def test_source_position(self):
        expected_position = np.array([0.5, 1.0, 0.2])
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.componentInfo().sourcePosition.return_value = expected_position
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model._source_position, expected_position)

    def test_detector_counts(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        expected_counts = np.array([100, 200, 300])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = expected_counts
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        expected_counts = np.array([100, 200, 300])
        np.testing.assert_array_equal(model.detector_counts, expected_counts)

    def test_detector_ids(self):
        expected_ids = [1, 2, 3]
        self._mock_detector_table(expected_ids)
        mock_workspace = self._create_mock_workspace(expected_ids)
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model.detector_ids, expected_ids)

    def test_counts_limits(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200, 300]
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        self.assertEqual(model.counts_limits[0], 100)
        self.assertEqual(model.counts_limits[1], 300)

    def test_integration_limits_ws_with_common_bins(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.isCommonBins.return_value = True
        mock_workspace.dataX.return_value = np.array([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        self.assertEqual(model.integration_limits, (1, 3))

    def test_integration_limits_on_ragged_workspace(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = True
        data_x = {0: np.array([1, 2, 3]), 1: np.array([10, 20, 30, 40]), 2: np.array([10, 20, 30, 40, 50])}
        mock_workspace.readX.side_effect = lambda i: data_x[i]
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        self.assertEqual(model.integration_limits, (1, 50))

    def test_integration_limits_on_non_ragged_workspace(self):
        self._mock_detector_table([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.extractX.return_value = np.array([[1, 2, 3], [10, 20, 30], [10, 20, 50]])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        self.assertEqual(model.integration_limits, (1, 50))

    def test_monitor_positions(self):
        self._mock_detector_table([1, 2, 3], monitors=np.array(["yes", "no", "yes"]))
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100]
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        monitor_positions = model.monitor_positions
        self.assertEqual(len(monitor_positions), 2)

    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ConvertUnits")
    @mock.patch.object(FullInstrumentViewModel, "picked_workspace_indices", new_callable=mock.PropertyMock)
    def test_extract_spectra_for_picked_detectors(self, mock_picked_workspace_indices, mock_convert_units, mock_extract_spectra):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_picked_workspace_indices.return_value = [1, 2]
        model = FullInstrumentViewModel(mock_workspace)
        model.extract_spectra_for_line_plot("TOF", False)
        mock_extract_spectra.assert_called_once_with(
            InputWorkspace=mock_workspace, WorkspaceIndexList=[1, 2], EnableLogging=False, StoreInADS=False
        )
        mock_convert_units.assert_called_once_with(
            InputWorkspace=mock_extract_spectra.return_value, target="TOF", EMode="Elastic", EnableLogging=False, StoreInADS=False
        )
        self.assertEqual(mock_convert_units.return_value, model.line_plot_workspace)

    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ConvertUnits")
    @mock.patch.object(FullInstrumentViewModel, "picked_workspace_indices", new_callable=mock.PropertyMock)
    def test_extract_spectra_no_picked_detectors(self, mock_picked_workspace_indices, mock_convert_units, mock_extract_spectra):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_picked_workspace_indices.return_value = []
        model = FullInstrumentViewModel(mock_workspace)
        model.extract_spectra_for_line_plot("Wavelength", True)
        self.assertIsNone(model.line_plot_workspace)
        mock_extract_spectra.assert_not_called()
        mock_convert_units.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.Rebin")
    @mock.patch("instrumentview.FullInstrumentViewModel.SumSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ConvertUnits")
    @mock.patch.object(FullInstrumentViewModel, "picked_workspace_indices", new_callable=mock.PropertyMock)
    def test_extract_spectra_sum(
        self, mock_picked_workspace_indices, mock_convert_units, mock_extract_spectra, mock_sum_spectra, mock_rebin
    ):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.isCommonBins.return_value = False
        mock_picked_workspace_indices.return_value = [1, 2]
        mock_extract_spectra.return_value = mock_workspace
        mock_convert_units.return_value = mock_workspace
        mock_sum_spectra.return_value = mock_workspace
        mock_rebin.return_value = mock_workspace
        model = FullInstrumentViewModel(mock_workspace)
        model.extract_spectra_for_line_plot("TOF", True)
        mock_extract_spectra.assert_called_once_with(
            InputWorkspace=mock_workspace, WorkspaceIndexList=[1, 2], EnableLogging=False, StoreInADS=False
        )
        mock_rebin.assert_called_once_with(InputWorkspace=mock_workspace, Params=[0, 1, 2], EnableLogging=False, StoreInADS=False)
        mock_sum_spectra.assert_called_once_with(InputWorkspace=mock_workspace, EnableLogging=False, StoreInADS=False)
        mock_convert_units.assert_called_once_with(
            InputWorkspace=mock_workspace, target="TOF", EMode="Elastic", EnableLogging=False, StoreInADS=False
        )

    @mock.patch("instrumentview.FullInstrumentViewModel.Rebin")
    @mock.patch("instrumentview.FullInstrumentViewModel.SumSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ConvertUnits")
    @mock.patch.object(FullInstrumentViewModel, "picked_workspace_indices", new_callable=mock.PropertyMock)
    def test_extract_spectra_sum_common_bins(
        self, mock_picked_workspace_indices, mock_convert_units, mock_extract_spectra, mock_sum_spectra, mock_rebin
    ):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.isCommonBins.return_value = True
        mock_picked_workspace_indices.return_value = [1, 2]
        mock_extract_spectra.return_value = mock_workspace
        mock_convert_units.return_value = mock_workspace
        mock_sum_spectra.return_value = mock_workspace
        mock_rebin.return_value = mock_workspace
        model = FullInstrumentViewModel(mock_workspace)
        model.extract_spectra_for_line_plot("TOF", True)
        mock_extract_spectra.assert_called_once_with(
            InputWorkspace=mock_workspace, WorkspaceIndexList=[1, 2], EnableLogging=False, StoreInADS=False
        )
        mock_rebin.assert_not_called()
        mock_sum_spectra.assert_called_once_with(InputWorkspace=mock_workspace, EnableLogging=False, StoreInADS=False)
        mock_convert_units.assert_called_once_with(
            InputWorkspace=mock_workspace, target="TOF", EMode="Elastic", EnableLogging=False, StoreInADS=False
        )

    @mock.patch("instrumentview.FullInstrumentViewModel.SumSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ConvertUnits")
    @mock.patch.object(FullInstrumentViewModel, "picked_workspace_indices", new_callable=mock.PropertyMock)
    def test_extract_spectra_sum_one_spectra(
        self, mock_picked_workspace_indices, mock_convert_units, mock_extract_spectra, mock_sum_spectra
    ):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_picked_workspace_indices.return_value = [2]
        mock_extract_spectra.return_value = mock_workspace
        mock_convert_units.return_value = mock_workspace
        mock_sum_spectra.return_value = mock_workspace
        model = FullInstrumentViewModel(mock_workspace)
        model.extract_spectra_for_line_plot("Wavelength", True)
        mock_extract_spectra.assert_called_once_with(
            InputWorkspace=mock_workspace, WorkspaceIndexList=[2], EnableLogging=False, StoreInADS=False
        )
        mock_workspace.applyBinEdgesFromAnotherWorkspace.assert_not_called()
        mock_sum_spectra.assert_not_called()
        mock_convert_units.assert_called_once_with(
            InputWorkspace=mock_workspace, target="Wavelength", EMode="Elastic", EnableLogging=False, StoreInADS=False
        )

    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractSpectra")
    @mock.patch("instrumentview.FullInstrumentViewModel.ConvertUnits")
    @mock.patch.object(FullInstrumentViewModel, "picked_workspace_indices", new_callable=mock.PropertyMock)
    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    def test_save_line_plot_workspace_to_ads(self, mock_ads, mock_picked_workspace_indices, mock_convert_units, mock_extract_spectra):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_picked_workspace_indices.return_value = [1, 2]
        model = FullInstrumentViewModel(mock_workspace)
        model.extract_spectra_for_line_plot("TOF", False)
        mock_convert_units.assert_called_once()
        mock_extract_spectra.assert_called_once()
        model.save_line_plot_workspace_to_ads()
        mock_ads.addOrReplace.assert_called_once()

    def test_has_no_unit(self):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.getAxis.return_value = mock.MagicMock()
        mock_workspace.getAxis(0).getUnit.return_value = mock.MagicMock()
        mock_workspace.getAxis(0).getUnit().unitID.return_value = "Empty"
        model = FullInstrumentViewModel(mock_workspace)
        self.assertEqual(False, model.has_unit)

    def test_has_unit(self):
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.getAxis.return_value = mock.MagicMock()
        mock_workspace.getAxis(0).getUnit.return_value = mock.MagicMock()
        mock_workspace.getAxis(0).getUnit().unitID.return_value = "Wavelength"
        model = FullInstrumentViewModel(mock_workspace)
        self.assertEqual(True, model.has_unit)

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    def test_peaks_workspaces_in_ads(self, mock_ads):
        mock_ads_instance = mock_ads.Instance()
        mock_peaks_workspace = PeaksWorkspaceMock()
        instrument = "MyFirstInstrument"
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        mock_workspace.getInstrument().getFullName.return_value = instrument
        mock_peaks_workspace.getInstrument().getFullName.return_value = instrument
        mock_ads_instance.retrieveWorkspaces.return_value = [mock_peaks_workspace, mock_workspace]
        model = FullInstrumentViewModel(mock_workspace)
        peaks_workspaces = model.peaks_workspaces_in_ads()
        self.assertEqual(1, len(peaks_workspaces))
        self.assertEqual(mock_peaks_workspace, peaks_workspaces[0])

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    def test_set_peaks_workspaces(self, mock_ads):
        mock_ads_instance = mock_ads.Instance()
        mock_peaks_workspace = PeaksWorkspaceMock()
        mock_ads_instance.retrieveWorkspaces.return_value = [mock_peaks_workspace]
        model = FullInstrumentViewModel(self._ws)
        names = ["a", "b"]
        model.set_peaks_workspaces(names)
        mock_ads_instance.retrieveWorkspaces.assert_called_once_with(names)
        self.assertEqual(mock_peaks_workspace, model._selected_peaks_workspaces[0])

    def test_peak_overlay_points(self):
        model = FullInstrumentViewModel(self._ws)
        peaks_ws = CreatePeaksWorkspace(self._ws, 2)
        model._selected_peaks_workspaces = [peaks_ws]
        model._detector_ids = np.array([100])
        model._is_valid = np.array([True])
        peaks = model.peak_overlay_points()
        self.assertEqual(1, len(peaks))
        detector_peak = peaks[0][0]
        self.assertEqual(100, detector_peak.detector_id)
        self.assertEqual("[0, 0, 0] x 2", detector_peak.label)
        np.testing.assert_almost_equal(np.array([0, 0, 5.0]), detector_peak.location)


if __name__ == "__main__":
    unittest.main()
