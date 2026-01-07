# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace, CreatePeaksWorkspace, AddPeak
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Peaks.Peak import Peak
import unittest
from unittest import mock
from unittest.mock import MagicMock
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


class PeaksWorkspaceMock(MagicMock):
    pass


class TestFullInstrumentViewModel(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._ws = CreateSampleWorkspace(OutputWorkspace="TestFullInstrumentViewModel", XUnit="TOF")

    def setUp(self) -> None:
        table_patcher = mock.patch("instrumentview.FullInstrumentViewModel.CreateDetectorTable")
        self.addCleanup(table_patcher.stop)
        self._mock_table = MagicMock()
        mock_table = table_patcher.start()
        mock_table.return_value = self._mock_table

        mask_patcher = mock.patch("instrumentview.FullInstrumentViewModel.ExtractMask")
        self.addCleanup(mask_patcher.stop)
        self._mock_extract_mask = MagicMock()
        self._mock_extract_mask = mask_patcher.start()

    def _setup_mocks(
        self,
        detector_ids: list[int],
        spectrum_no: np.ndarray = np.array([]),
        monitors: np.ndarray = np.array([]),
        det_mask: np.ndarray = np.array([]),
    ):
        if monitors.size == 0:
            monitors = np.array(["no" for _ in detector_ids])

        if spectrum_no.size == 0:
            spectrum_no = np.array([i for i in range(len(detector_ids))])

        if det_mask.size == 0:
            det_mask = np.zeros(len(detector_ids))

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

        mock_mask_ws = MagicMock(extractY=MagicMock(return_value=det_mask))
        self._mock_extract_mask.return_value = mock_mask_ws, []
        return

    def _create_mock_workspace(self, detector_ids: list[int]):
        mock_workspace = MagicMock()
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.isCommonBins.return_value = False
        mock_workspace.detectorInfo.return_value = MagicMock()
        mock_workspace.componentInfo.return_value = MagicMock()
        mock_workspace.getNumberHistograms.return_value = len(detector_ids)
        mock_workspace.extractX.return_value = np.tile(np.arange(len(detector_ids)), (len(detector_ids), 1))
        mock_workspace.readX.return_value = np.arange(len(detector_ids))
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100 * i for i in detector_ids]
        return mock_workspace

    def _setup_model(
        self,
        detector_ids: list[int],
        spectrum_no: np.ndarray = np.array([]),
        monitors: np.ndarray = np.array([]),
        det_mask: np.ndarray = np.array([]),
    ):
        mock_ws = self._create_mock_workspace(detector_ids)
        self._setup_mocks(detector_ids, spectrum_no, monitors, det_mask)
        model = FullInstrumentViewModel(mock_ws)
        model.setup()
        model._workspace_x_unit = "dSpacing"
        return model, mock_ws

    def test_update_integration_range(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        integrated_spectra = [1, 20, 100]
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = integrated_spectra
        model.update_integration_range((200, 10000), False)
        model._workspace.getIntegratedCountsForWorkspaceIndices.assert_called()
        self.assertEqual(min(integrated_spectra), model._counts_limits[0])
        self.assertEqual(max(integrated_spectra), model._counts_limits[1])

    @mock.patch("instrumentview.FullInstrumentViewModel.DetectorInfo")
    def test_picked_detectors_info_text(self, det_info_mock):
        model, mock_workspace = self._setup_model([1, 20, 300])
        mock_workspace.getDetector.side_effect = lambda i: MagicMock(
            getName=mock.Mock(return_value=str(i)), getFullName=mock.Mock(return_value=f"Full_{i}")
        )
        model._is_valid = np.array([False, True, True])
        model._detector_is_picked = np.array([False, False, True])
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
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, False, False])
        model.negate_picked_visibility(np.array([False, True, True]))
        np.testing.assert_equal(model._detector_is_picked, [False, True, True])

    def test_clear_all_picked_detectors(self):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, True, True])
        model.clear_all_picked_detectors()
        np.testing.assert_equal(model._detector_is_picked, [False, False, False])

    def test_detectors_with_no_spectra(self):
        self._setup_mocks([1, 20, 300, 400], monitors=np.array(["no", "no", "n/a", "yes"]))
        mock_workspace = self._create_mock_workspace([1, 20, 300, 400])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200]
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model._detector_ids, [1, 20, 300, 400])
        np.testing.assert_array_equal(model._is_valid, [True, True, False, False])
        np.testing.assert_array_equal(model._counts, [100, 200, 0, 0])

    @mock.patch("instrumentview.FullInstrumentViewModel.SphericalProjection")
    def test_calculate_spherical_projection(self, mock_spherical_projection):
        self._run_projection_test(mock_spherical_projection, ProjectionType.SPHERICAL_Y)

    @mock.patch("instrumentview.FullInstrumentViewModel.CylindricalProjection")
    def test_calculate_cylindrical_projection(self, mock_cylindrical_projection):
        self._run_projection_test(mock_cylindrical_projection, ProjectionType.CYLINDRICAL_Y)

    @mock.patch("instrumentview.FullInstrumentViewModel.SideBySide")
    def test_calculate_side_by_side_projection(self, mock_side_by_side):
        self._run_projection_test(mock_side_by_side, ProjectionType.SIDE_BY_SIDE)

    def _run_projection_test(self, mock_projection_constructor, projection_option):
        model, _ = self._setup_model([1, 2, 3])
        mock_projection = MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_constructor.return_value = mock_projection
        model._projection_type = projection_option
        points = model._calculate_projection()
        mock_projection_constructor.assert_called_once()
        self.assertTrue(all(all(point == [1, 2, 0]) for point in points))

    def test_sample_position(self):
        expected_position = np.array([1.0, 2.0, 1.0])
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.componentInfo().samplePosition.return_value = expected_position
        model.setup()
        np.testing.assert_array_equal(model.sample_position, expected_position)

    def test_detector_positions(self):
        self._setup_mocks([1, 2, 3])
        mock_workspace = self._create_mock_workspace([1, 2, 3])
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model.detector_positions, [[0, 0, 0], [1, 1, 1], [2, 2, 2]])

    def test_picked_detector_ids(self):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, False, True])
        self.assertEqual(model.picked_detector_ids, [3])

    def test_picked_workspace_indices(self):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, False, True])
        self.assertEqual(model.picked_workspace_indices, [2])  # Indices in mock start at 0

    def test_source_position(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        expected_position = np.array([0.5, 1.0, 0.2])
        mock_workspace.componentInfo().sourcePosition.return_value = expected_position
        model.setup()
        np.testing.assert_array_equal(model._source_position, expected_position)

    def test_detector_counts(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        expected_counts = np.array([100, 200, 300])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = expected_counts
        model.setup()
        np.testing.assert_array_equal(model.detector_counts, expected_counts)

    def test_detector_ids(self):
        expected_ids = [1, 2, 3]
        model, _ = self._setup_model(expected_ids)
        np.testing.assert_array_equal(model.detector_ids, expected_ids)

    def test_counts_limits(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200, 300]
        model.setup()
        self.assertEqual(model.counts_limits[0], 100)
        self.assertEqual(model.counts_limits[1], 300)

    def test_integration_limits_ws_with_common_bins(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isCommonBins.return_value = True
        mock_workspace.dataX.return_value = np.array([1, 2, 3])
        model.setup()
        self.assertEqual(model.integration_limits, (1, 3))

    def test_integration_limits_on_ragged_workspace(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = True
        data_x = {0: np.array([1, 2, 3]), 1: np.array([10, 20, 30, 40]), 2: np.array([10, 20, 30, 40, 50])}
        mock_workspace.readX.side_effect = lambda i: data_x[i]
        model.setup()
        self.assertEqual(model.integration_limits, (1, 50))

    def test_integration_limits_on_non_ragged_workspace(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.extractX.return_value = np.array([[1, 2, 3], [10, 20, 30], [10, 20, 50]])
        model.setup()
        self.assertEqual(model.integration_limits, (1, 50))

    def test_monitor_positions(self):
        self._setup_mocks([1, 2, 3], monitors=np.array(["yes", "no", "yes"]))
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
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_picked_workspace_indices.return_value = [1, 2]
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
        model, _ = self._setup_model([1, 2, 3])
        mock_picked_workspace_indices.return_value = []
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
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isCommonBins.return_value = False
        mock_picked_workspace_indices.return_value = [1, 2]
        mock_extract_spectra.return_value = mock_workspace
        mock_convert_units.return_value = mock_workspace
        mock_sum_spectra.return_value = mock_workspace
        mock_rebin.return_value = mock_workspace
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
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isCommonBins.return_value = True
        mock_picked_workspace_indices.return_value = [1, 2]
        mock_extract_spectra.return_value = mock_workspace
        mock_convert_units.return_value = mock_workspace
        mock_sum_spectra.return_value = mock_workspace
        mock_rebin.return_value = mock_workspace
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
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_picked_workspace_indices.return_value = [2]
        mock_extract_spectra.return_value = mock_workspace
        mock_convert_units.return_value = mock_workspace
        mock_sum_spectra.return_value = mock_workspace
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
        model, _ = self._setup_model([1, 2, 3])
        mock_picked_workspace_indices.return_value = [1, 2]
        model.extract_spectra_for_line_plot("TOF", False)
        mock_convert_units.assert_called_once()
        mock_extract_spectra.assert_called_once()
        model.save_line_plot_workspace_to_ads()
        mock_ads.addOrReplace.assert_called_once()

    def test_has_no_unit(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.getAxis.return_value = MagicMock()
        mock_workspace.getAxis(0).getUnit.return_value = MagicMock()
        mock_workspace.getAxis(0).getUnit().unitID.return_value = "Empty"
        model.setup()
        self.assertEqual(False, model.has_unit)

    def test_has_unit(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.getAxis.return_value = MagicMock()
        mock_workspace.getAxis(0).getUnit.return_value = MagicMock()
        mock_workspace.getAxis(0).getUnit().unitID.return_value = "Wavelength"
        model.setup()
        self.assertEqual(True, model.has_unit)

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    def test_peaks_workspaces_in_ads(self, mock_ads):
        mock_ads_instance = mock_ads.Instance()
        mock_peaks_workspace = PeaksWorkspaceMock()
        instrument = "MyFirstInstrument"
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.getInstrument().getFullName.return_value = instrument
        mock_peaks_workspace.getInstrument().getFullName.return_value = instrument
        mock_ads_instance.retrieveWorkspaces.return_value = [mock_peaks_workspace, mock_workspace]
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
        model._spectrum_nos = np.array([2])
        model._is_valid = np.array([True])
        model._is_masked = np.array([False])
        peaks = model.peak_overlay_points()
        self.assertEqual(1, len(peaks))
        detector_peak = peaks["peaks_ws"][0]
        self.assertEqual(100, detector_peak.detector_id)
        self.assertEqual("[0, 0, 0] x 2", detector_peak.label)
        np.testing.assert_almost_equal(np.array([0, 0, 5.0]), detector_peak.location)

    def test_peaks_with_spectrum_multiple_detectors(self):
        # We want to test the case where a peaks workspace has a
        # detector ID that is not in the model. This can happen when
        # an instrument has multiple detector IDs per spectrum. The
        # spectrum number should be used instead
        mock_ws = self._create_mock_workspace([1, 2, 3, 4])
        test_detector_id = 4
        test_spectrum_no = 1

        # Detector ID 4 is workspace index 2
        def mock_getIndicesFromDetectorIDs(ids):
            return [2]

        mock_ws.getIndicesFromDetectorIDs = mock_getIndicesFromDetectorIDs
        model = FullInstrumentViewModel(mock_ws)
        mock_peaks_ws = MagicMock()
        mock_peaks_ws.toDict.return_value = {
            "DetID": [test_detector_id],
            "h": [2],
            "k": [2],
            "l": [2],
            "DSpacing": [10],
            "Wavelength": [10],
            "TOF": [10],
        }
        mock_peaks_ws.name.return_value = "mock_peaks_ws"

        model._selected_peaks_workspaces = [mock_peaks_ws]
        # Only read three detectors, not ID 4
        model._detector_ids = np.array([1, 2, 3])
        # Everything on spectrum number 1
        model._spectrum_nos = np.array([test_spectrum_no, test_spectrum_no, test_spectrum_no])
        model._is_valid = np.array([True, True, True])
        model._is_masked = np.array([False, False, False])
        peaks = model.peak_overlay_points()
        # Should get one peak with detector ID 4 and spectrum
        # number 1
        self.assertEqual(1, len(peaks))
        detector_peak = peaks[mock_peaks_ws.name()][0]
        self.assertEqual(1, len(detector_peak.peaks))
        self.assertEqual(test_detector_id, detector_peak.detector_id)
        self.assertEqual("(2, 2, 2)", detector_peak.label)
        self.assertEqual(test_spectrum_no, detector_peak.spectrum_no)
        single_peak = detector_peak.peaks[0]
        self.assertEqual(test_detector_id, single_peak.detector_id)
        self.assertEqual(test_spectrum_no, single_peak.spectrum_no)

    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    def test_relative_detector_angle_no_picked(self, mock_picked_detector_ids):
        model, mock_workspace = self._setup_model([10, 11, 12])
        mock_detector_info = mock_workspace.detectorInfo()
        mock_detector_info.azimuthal.return_value = 0.1
        with self.assertRaisesRegex(RuntimeError, ".*two detectors are selected"):
            model.relative_detector_angle()
        mock_picked_detector_ids.assert_called_once()

    @mock.patch.object(FullInstrumentViewModel, "picked_detector_ids", new_callable=mock.PropertyMock)
    def test_relative_detector_angle_two_picked(self, mock_picked_detector_ids):
        model, mock_workspace = self._setup_model([10, 11, 12])
        mock_detector_info = mock_workspace.detectorInfo()
        mock_detector_info.azimuthal.return_value = 0.1

        def mock_index_of(idx):
            return idx

        mock_detector_info.indexOf = mock_index_of

        def mock_two_theta(idx):
            return 0.3 if idx == 10 else 0.8

        mock_detector_info.twoTheta = mock_two_theta
        mock_picked_detector_ids.return_value = [10, 11]
        angle = model.relative_detector_angle()
        np.testing.assert_allclose(14.324, angle, rtol=0.001)
        mock_picked_detector_ids.assert_called_once()
        self.assertEqual(2, mock_detector_info.azimuthal.call_count)

    def test_calculate_q_lab_direction(self):
        model, mock_ws = self._setup_model([10])
        mock_detector_info = mock_ws.detectorInfo()
        mock_detector_info.azimuthal.return_value = 0.1
        mock_detector_info.twoTheta.return_value = 0.8
        q_lab = model._calculate_q_lab_direction(10)
        np.testing.assert_allclose([-0.91646, -0.091953, 0.389418], q_lab, rtol=1e-5)

    def test_actual_q_lab_calc(self):
        detector_info = self._ws.detectorInfo()
        peaks_ws = CreatePeaksWorkspace(self._ws, 1)
        detector_id = int(detector_info.detectorIDs()[-1])
        AddPeak(peaks_ws, self._ws, DetectorID=detector_id, TOF=10000)
        peak = peaks_ws.getPeak(1)
        q_lab = np.array(peak.getQLabFrame())
        q_lab_direction = q_lab / np.linalg.norm(q_lab)
        model = FullInstrumentViewModel(self._ws)
        iv_qlab = model._calculate_q_lab_direction(detector_id)
        np.testing.assert_allclose(q_lab_direction, iv_qlab, rtol=1e-5)

    @mock.patch("instrumentview.FullInstrumentViewModel.CylindricalProjection")
    def test_cached_projections_map_empty(self, mock_projection_constructor):
        model, _ = self._setup_model([1, 2, 3])
        model.projection_type = ProjectionType.CYLINDRICAL_X
        mock_projection = MagicMock(positions=MagicMock(return_value=np.array([[1, 2], [1, 2], [1, 2]])))
        mock_projection_constructor.return_value = mock_projection
        model._cached_projections_map = {}
        positions = model._calculate_projection()
        np.testing.assert_almost_equal(positions, np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]]))
        np.testing.assert_almost_equal(
            model._cached_projections_map[ProjectionType.CYLINDRICAL_X.name], np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]])
        )

    @mock.patch("instrumentview.FullInstrumentViewModel.CylindricalProjection")
    def test_cached_projections_map_already_cached(self, mock_projection_constructor):
        model, _ = self._setup_model([1, 2, 3])
        model.projection_type = ProjectionType.CYLINDRICAL_X
        model._cached_projections_map = {ProjectionType.CYLINDRICAL_X.name: np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]])}
        positions = model._calculate_projection()
        np.testing.assert_almost_equal(positions, np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]]))
        mock_projection_constructor.assert_not_called()

    def test_is_pickable(self):
        model, _ = self._setup_model([1, 2, 3])
        model._is_valid = np.array([False, True, True])
        model._is_masked = np.array([False, False, True])
        np.testing.assert_array_equal(model.is_pickable, np.array([False, True, False]))

    def test_get_default_projection_index_and_options_3D(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.getInstrument = MagicMock(return_value=MagicMock(getDefaultView=MagicMock(return_value="3D")))
        index, projection_options = model.get_default_projection_index_and_options()
        self.assertEqual(projection_options[index], ProjectionType.THREE_D)

    def test_get_default_projection_index_and_options_non_3D(self):
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.getInstrument = MagicMock(return_value=MagicMock(getDefaultView=MagicMock(return_value="SPHERICAL_X")))
        index, projection_options = model.get_default_projection_index_and_options()
        self.assertEqual(projection_options[index], ProjectionType.SPHERICAL_X)

    def test_is_2d_projection_false(self):
        model, _ = self._setup_model([1, 2, 3])
        model._projection_type = ProjectionType.THREE_D
        self.assertEqual(model.is_2d_projection, False)

    def test_is_2d_projection_true(self):
        model, _ = self._setup_model([1, 2, 3])
        model._projection_type = ProjectionType.SPHERICAL_X
        self.assertEqual(model.is_2d_projection, True)

    def test_detector_positions_3D(self):
        model, _ = self._setup_model([1, 2, 3])
        expected_positions = np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]])
        model._detector_positions_3d = expected_positions
        model._projection_type = ProjectionType.THREE_D
        model._is_valid = np.array([True, True, True])
        model._is_masked = np.array([True, False, False])
        np.testing.assert_array_equal(model.detector_positions, expected_positions[1:])

    @mock.patch.object(FullInstrumentViewModel, "_calculate_projection")
    def test_detector_positions_non_3D(self, mock_calc_projection):
        model, _ = self._setup_model([1, 2, 3])
        expected_positions = np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]])
        mock_calc_projection.return_value = expected_positions
        model._projection_type = ProjectionType.SPHERICAL_X
        model._is_valid = np.array([True, True, True])
        model._is_masked = np.array([True, False, False])
        np.testing.assert_array_equal(model.detector_positions, expected_positions[1:])

    def test_add_mask(self):
        model, _ = self._setup_model([1, 2, 3])
        model._cached_masks_map = {}
        model._is_masked_in_ws = np.array([True, True, False])
        model._is_valid = np.array([True, True, True])
        model._is_masked = np.array([True, False, False])
        model.add_new_detector_mask([True, True])
        np.testing.assert_array_equal(model._cached_masks_map["Mask 1"], np.array([True, True, True]))

    def test_apply_detector_mask(self):
        model, _ = self._setup_model([1, 2, 3])
        # All detectors picked, mask should unpick them
        model._detector_is_picked = np.array([True, True, True])
        model._cached_masks_map = {"Mask 1": np.array([True, False, False]), "Mask 2": np.array([False, True, False])}
        model.apply_detector_masks(["Mask 1", "Mask 2"])
        np.testing.assert_array_equal(model._is_masked, np.array([True, True, False]))
        np.testing.assert_array_equal(model._detector_is_picked, np.array([False, False, True]))

    def test_apply_detector_mask_empty(self):
        model, _ = self._setup_model([1, 2, 3])
        model._is_masked_in_ws = np.array([True, False, False])
        model.apply_detector_masks([])
        np.testing.assert_array_equal(model._is_masked, np.array([True, False, False]))
        np.testing.assert_array_equal(model._detector_is_picked, np.array([False, False, False]))

    @mock.patch("instrumentview.FullInstrumentViewModel.CloneWorkspace")
    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractMaskToTable")
    def test_save_mask_workspace_to_ads(self, mock_extract_to_table, mock_clone):
        model, _ = self._setup_model([1, 2, 3])
        model.save_mask_workspace_to_ads()
        mock_extract_to_table.assert_called_once()
        mock_clone.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.SaveMask")
    def test_save_xml_mask(self, mock_save_mask):
        model, _ = self._setup_model([1, 2, 3])
        model.save_xml_mask("file")
        mock_save_mask.assert_called_with(model._mask_ws, OutputFile="file.xml")

    def test_masked_spectrum_peak_not_included(self):
        model = FullInstrumentViewModel(self._ws)
        peaks_ws = CreatePeaksWorkspace(self._ws, 2)
        model._selected_peaks_workspaces = [peaks_ws]
        model._detector_ids = np.array([100])
        model._spectrum_nos = np.array([2])
        model._is_valid = np.array([True])
        model._is_masked = np.array([True])
        peaks = model.peak_overlay_points()
        self.assertEqual(0, len(peaks[peaks_ws.name()]))

    def test_delete_all_peaks_skips_workspaces_not_in_overlay(self):
        """Workspaces missing from overlay mapping are ignored."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeaks = MagicMock()
        ws2 = MagicMock()
        ws2.name.return_value = "ws2"
        ws2.removePeaks = MagicMock()
        model._selected_peaks_workspaces = [ws1, ws2]
        # Only ws1 present in overlay
        overlay = {"ws1": [DetectorPeaks([self._create_peak(10, 7), self._create_peak(11, 7)])]}
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peaks_on_all_selected_detectors()
        ws1.removePeaks.assert_called_once_with([10, 11])
        ws2.removePeaks.assert_not_called()

    def test_delete_all_peaks_no_matching_detector_ids_no_removal(self):
        """No groups with a detector_id in picked_detector_ids -> no removePeaks calls."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeaks = MagicMock()
        model._selected_peaks_workspaces = [ws1]
        overlay = {
            "ws1": [
                DetectorPeaks([self._create_peak(1, 99)]),  # not selected
                DetectorPeaks([self._create_peak(2, 100)]),  # not selected
            ]
        }
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peaks_on_all_selected_detectors()
        ws1.removePeaks.assert_not_called()

    def test_delete_all_peaks_removes_peaks_for_multiple_selected_detectors_in_same_workspace(self):
        """Aggregates peak indices from all matching detector groups in a workspace."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeaks = MagicMock()
        model._selected_peaks_workspaces = [ws1]
        # Matching detectors: 7 and 8; non-matching 99
        overlay = {
            "ws1": [
                DetectorPeaks([self._create_peak(10, 7), self._create_peak(11, 7)]),
                DetectorPeaks([self._create_peak(20, 8)]),
                DetectorPeaks([self._create_peak(999, 99)]),  # should be ignored
            ]
        }
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peaks_on_all_selected_detectors()
        # Order preserves traversal: group(7) then group(8)
        ws1.removePeaks.assert_called_once_with([10, 11, 20])

    def test_delete_all_peaks_aggregates_per_workspace_independently(self):
        """Each workspace gets its own set of aggregated indices."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeaks = MagicMock()
        ws2 = MagicMock()
        ws2.name.return_value = "ws2"
        ws2.removePeaks = MagicMock()
        model._selected_peaks_workspaces = [ws1, ws2]
        overlay = {
            "ws1": [
                DetectorPeaks([self._create_peak(1, 7), self._create_peak(2, 7)]),
                DetectorPeaks([self._create_peak(3, 8)]),
            ],
            "ws2": [
                DetectorPeaks([self._create_peak(100, 8), self._create_peak(200, 8)]),
            ],
        }
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peaks_on_all_selected_detectors()
        ws1.removePeaks.assert_called_once_with([1, 2, 3])
        ws2.removePeaks.assert_called_once_with([100, 200])

    def test_delete_all_peaks_no_selected_workspaces_no_crash(self):
        """With no selected workspaces, method exits quietly."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        model._selected_peaks_workspaces = []
        model.peak_overlay_points = MagicMock(return_value={})
        # Should not raise
        model.delete_peaks_on_all_selected_detectors()
        model.peak_overlay_points.assert_called_once()

    def _create_peak(self, peak_index: int, detector_id: int, x: float = 1.0) -> Peak:
        return Peak(detector_id, 0, np.zeros(3), peak_index, (0, 0, 0), x, x, x, x)

    def test_no_selected_workspaces_no_overlay_no_removal(self):
        """When there are no selected workspaces or overlay entries, nothing is removed."""
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = [True, False, False]
        model._selected_peaks_workspaces = []
        model.peak_overlay_points = MagicMock(return_value={})
        model.delete_peak(5.0)
        # No workspace exists to assert removePeak calls; just ensure method didn’t crash.
        model.peak_overlay_points.assert_called_once()

    def test_no_matching_detector_peaks_no_removal(self):
        """If overlay contains no groups with the picked detector_id, nothing is removed."""
        model, _ = self._setup_model([7])
        model._detector_is_picked = [True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeak = MagicMock()
        model._selected_peaks_workspaces = [ws1]
        # Only detector_id=3 in overlay; model expects detector_id=7
        overlay = {"ws1": [DetectorPeaks([self._create_peak(10, 3, 1.0), self._create_peak(11, 3, 2.0)])]}
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peak(2.2)
        ws1.removePeak.assert_not_called()

    def test_removes_closest_peak_in_single_workspace(self):
        """Selects and removes the closest peak within a single workspace."""
        model, _ = self._setup_model([7])
        model._detector_is_picked = [True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeak = MagicMock()
        model._selected_peaks_workspaces = [ws1]
        # Peaks at 1.0 (idx=100), 2.0 (idx=101), 10.0 (idx=102); click at 2.2 -> closest is 2.0
        overlay = {
            "ws1": [DetectorPeaks([self._create_peak(100, 7, 1.0), self._create_peak(101, 7, 2.0), self._create_peak(102, 7, 10.0)])]
        }
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peak(2.2)
        ws1.removePeak.assert_called_once_with(101)

    def test_selects_workspace_with_overall_min_distance(self):
        """Among multiple workspaces, chooses the peak with the smallest distance overall."""
        model, _ = self._setup_model([1, 2, 3, 7])
        model._detector_is_picked = [False, False, False, True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeak = MagicMock()
        ws2 = MagicMock()
        ws2.name.return_value = "ws2"
        ws2.removePeak = MagicMock()
        model._selected_peaks_workspaces = [ws1, ws2]
        # ws1 closest distance = |2.5 - 2.2| = 0.3 (peak_index=201)
        # ws2 closest distance = |2.3 - 2.2| = 0.1 (peak_index=301) -> ws2 should be chosen
        overlay = {
            "ws1": [DetectorPeaks([self._create_peak(201, 7, 2.5), self._create_peak(202, 7, 100.0)])],
            "ws2": [DetectorPeaks([self._create_peak(301, 7, 2.3), self._create_peak(302, 7, 50.0)])],
        }
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peak(2.2)
        ws2.removePeak.assert_called_once_with(301)
        ws1.removePeak.assert_not_called()

    def test_skips_workspaces_not_in_overlay(self):
        """Workspaces not present in peak_overlay_points mapping are ignored."""
        model, _ = self._setup_model([7])
        model._detector_is_picked = [True]
        ws1 = MagicMock()
        ws1.name.return_value = "ws1"
        ws1.removePeak = MagicMock()
        ws3 = MagicMock()
        ws3.name.return_value = "ws3"
        ws3.removePeak = MagicMock()

        model._selected_peaks_workspaces = [ws1, ws3]
        overlay = {
            "ws1": [DetectorPeaks([self._create_peak(10, 7, 1.9)])]
            # ws3 missing from overlay
        }
        model.peak_overlay_points = MagicMock(return_value=overlay)
        model.delete_peak(2.0)
        ws1.removePeak.assert_called_once_with(10)
        ws3.removePeak.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.CreatePeaksWorkspace")
    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    def test_peaks_workspace_for_adding_new_peak(self, mock_ads, mock_create_peaks_ws):
        model, _ = self._setup_model([1, 2, 3])
        mock_ads_instance = mock_ads.Instance()
        mock_ads_instance.doesExist.return_value = False
        model._peaks_workspace_for_adding_new_peak([])
        mock_create_peaks_ws.assert_called_once_with(
            model._workspace, 0, OutputWorkspace=model._instrument_view_peaks_ws_name, StoreInADS=False
        )
        mock_create_peaks_ws.reset_mock()
        mock_ads_instance.doesExist.reset_mock()

        model._peaks_workspace_for_adding_new_peak(["my_peaks_ws"])
        mock_create_peaks_ws.assert_not_called()
        mock_ads_instance.retrieveWorkspaces.assert_called_once_with(["my_peaks_ws"])
        mock_ads_instance.retrieveWorkspaces.reset_mock()

        mock_ads_instance.doesExist.return_value = True
        model._peaks_workspace_for_adding_new_peak(["my_peaks_ws1", "my_peaks_ws2"])
        mock_ads_instance.doesExist.assert_called_once_with(model._instrument_view_peaks_ws_name)
        mock_ads_instance.doesExist.reset_mock()

        mock_ads_instance.doesExist.return_value = False
        model._peaks_workspace_for_adding_new_peak(["my_peaks_ws1", "my_peaks_ws2"])
        mock_create_peaks_ws.assert_called_once_with(
            model._workspace, 0, OutputWorkspace=model._instrument_view_peaks_ws_name, StoreInADS=False
        )

    @mock.patch("instrumentview.FullInstrumentViewModel.AddPeak")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel._peaks_workspace_for_adding_new_peak")
    def test_add_peak(self, mock_peaks_ws_for_adding, mock_add_peak):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, False, True])
        peaks_ws = mock_peaks_ws_for_adding()
        peaks_ws.name.return_value = "my_peaks_ws"
        peak_x = 1500
        ws = model.add_peak(peak_x, MagicMock())
        mock_add_peak.assert_called_once_with(peaks_ws, model._workspace, peak_x, 3)
        self.assertEqual("my_peaks_ws", ws)


if __name__ == "__main__":
    unittest.main()
