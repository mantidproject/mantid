# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Globals import CurrentTab
from mantid.simpleapi import CreateSampleWorkspace, CreatePeaksWorkspace, AddPeak
from mantid.dataobjects import PeaksWorkspace
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
        mock_detector_info = MagicMock()
        mock_detector_info.detectorIDs.return_value = np.array(detector_ids)
        mock_detector_info.indexOf.side_effect = lambda det_id: detector_ids.index(det_id)
        mock_workspace.detectorInfo.return_value = mock_detector_info
        mock_workspace.componentInfo.return_value = MagicMock()
        mock_workspace.dataY.return_value = MagicMock()
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
    ) -> tuple[FullInstrumentViewModel, MagicMock]:
        mock_ws = self._create_mock_workspace(detector_ids)
        self._setup_mocks(detector_ids, spectrum_no, monitors, det_mask)
        model = FullInstrumentViewModel(mock_ws)
        model.setup()
        model._is_selected_in_tree = np.ones(len(model._detector_ids), dtype=bool)
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
        self.assertEqual(model._counts_limits, model.full_counts_limits)

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
        model.update_point_picked_detectors(1, False)
        np.testing.assert_equal(model._detector_is_picked, [False, True, False])

    def test_clear_point_picked_detectors(self):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([True, True, True])
        model._point_picked_detectors = np.array([False, True, True])
        model.clear_point_picked_detectors()
        np.testing.assert_equal(model._point_picked_detectors, [False, False, False])
        np.testing.assert_equal(model._detector_is_picked, [True, False, False])

    def test_update_point_picked_detectors_expand_to_parent_subtree(self):
        model, mock_workspace = self._setup_model([10, 11, 12, 13])
        component_info = mock_workspace.componentInfo.return_value
        component_info.hasParent.return_value = True
        component_info.parent.side_effect = lambda detector_info_index: 100 if detector_info_index in (1, 2) else 200
        component_info.detectorsInSubtree.side_effect = lambda parent: np.array([1, 2]) if parent == 100 else np.array([3])

        model._is_valid = np.array([True, True, True, True])
        model._is_masked = np.array([False, False, False, False])
        model._is_selected_in_tree = np.array([True, True, True, True])

        model.update_point_picked_detectors(1, expand_to_parent_subtree=True)
        np.testing.assert_equal(model._detector_is_picked, [False, True, True, False])
        np.testing.assert_equal(model._point_picked_detectors, [False, True, True, False])

    def test_expand_pickable_mask_to_parent_subtrees(self):
        model, mock_workspace = self._setup_model([10, 11, 12, 13])
        component_info = mock_workspace.componentInfo.return_value
        component_info.hasParent.return_value = True
        component_info.parent.side_effect = lambda detector_info_index: 100 if detector_info_index in (1, 2) else 200
        component_info.detectorsInSubtree.side_effect = lambda parent: np.array([1, 2]) if parent == 100 else np.array([3])

        model._is_valid = np.array([True, True, True, True])
        model._is_masked = np.array([False, False, False, False])
        model._is_selected_in_tree = np.array([True, True, True, True])

        expanded = model.expand_pickable_mask_to_parent_subtrees(np.array([False, True, False, False]))
        np.testing.assert_equal(expanded, [False, True, True, False])

    def test_peak_picking_enabled_off_by_default(self):
        model, _ = self._setup_model([1, 2, 3])
        self.assertFalse(model.peak_picking_enabled())

    def test_peak_picking_enabled_after_turn_on(self):
        model, _ = self._setup_model([1, 2, 3])
        model.turn_on_single_point_picking()
        self.assertTrue(model.peak_picking_enabled())

    def test_peak_picking_disabled_after_turn_off(self):
        model, _ = self._setup_model([1, 2, 3])
        model.turn_on_single_point_picking()
        model.turn_off_single_point_picking()
        self.assertFalse(model.peak_picking_enabled())

    def test_turn_on_single_point_picking_caches_and_clears(self):
        model, _ = self._setup_model([1, 2, 3])
        model._point_picked_detectors = np.array([True, False, True])
        model.turn_on_single_point_picking()
        np.testing.assert_equal(model._point_picked_detectors, [False, False, False])
        np.testing.assert_equal(model._point_picked_detectors_cached, [True, False, True])

    def test_turn_off_single_point_picking_restores_cached(self):
        model, _ = self._setup_model([1, 2, 3])
        model._point_picked_detectors = np.array([True, False, True])
        model._detector_is_picked = np.array([True, False, True])
        model.turn_on_single_point_picking()
        model.turn_off_single_point_picking()
        np.testing.assert_equal(model._point_picked_detectors, [True, False, True])
        np.testing.assert_equal(model._detector_is_picked, [True, False, True])

    def test_update_point_picked_detectors_peak_picking_on_selects_single(self):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, False, False])
        model.turn_on_single_point_picking()
        model.update_point_picked_detectors(0, False)
        np.testing.assert_equal(model._detector_is_picked, [True, False, False])
        np.testing.assert_equal(model._point_picked_detectors, [True, False, False])
        # Picking another detector clears the previous one
        model.update_point_picked_detectors(1, False)
        np.testing.assert_equal(model._detector_is_picked, [False, True, False])
        np.testing.assert_equal(model._point_picked_detectors, [False, True, False])

    def test_detectors_with_no_spectra(self):
        self._setup_mocks([1, 20, 300, 400], monitors=np.array(["no", "no", "n/a", "yes"]))
        mock_workspace = self._create_mock_workspace([1, 20, 300, 400])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200]
        model = FullInstrumentViewModel(mock_workspace)
        model.setup()
        np.testing.assert_array_equal(model._detector_ids, [1, 20, 300, 400])
        np.testing.assert_array_equal(model._is_valid, [True, True, False, False])
        np.testing.assert_array_equal(model._counts, [100, 200, 0, 0])

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_calculate_spherical_projection(self, mock_projection):
        self._run_projection_test(mock_projection, ProjectionType.SPHERICAL_Y)

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_calculate_cylindrical_projection(self, mock_projection):
        self._run_projection_test(mock_projection, ProjectionType.CYLINDRICAL_Y)

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_calculate_side_by_side_projection(self, mock_projection):
        self._run_projection_test(mock_projection, ProjectionType.SIDE_BY_SIDE)

    def _run_projection_test(self, mock_projection_constructor, projection_option):
        model, _ = self._setup_model([1, 2, 3])
        mock_projection = MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_constructor.return_value = mock_projection
        model._projection_type = projection_option
        points = model._calculate_projection()
        mock_projection_constructor.assert_called_once()
        self.assertTrue(all(all(point == [1, 2, 0]) for point in points))

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_flip_z_negates_z_in_projection(self, mock_projection_cls):
        """When flip_z is True, detector positions passed to projection should have negated Z coordinates"""
        model, _ = self._setup_model([1, 2, 3])
        mock_projection = MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_cls.return_value = mock_projection
        original_positions = model._detector_positions_3d.copy()
        model._projection_type = ProjectionType.CYLINDRICAL_Y
        model.flip_z = True
        model._calculate_projection()
        call_args = mock_projection_cls.call_args
        passed_positions = call_args.kwargs["detector_positions"]
        expected_positions = original_positions.copy()
        expected_positions[:, 2] *= -1
        np.testing.assert_array_equal(passed_positions, expected_positions)

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_flip_z_false_passes_original_positions(self, mock_projection_cls):
        """When flip_z is False, detector positions passed to projection should be unchanged"""
        model, _ = self._setup_model([1, 2, 3])
        mock_projection = MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_cls.return_value = mock_projection
        original_positions = model._detector_positions_3d.copy()
        model._projection_type = ProjectionType.CYLINDRICAL_Y
        model.flip_z = False
        model._calculate_projection()
        call_args = mock_projection_cls.call_args
        passed_positions = call_args.kwargs["detector_positions"]
        np.testing.assert_array_equal(passed_positions, original_positions)

    def test_flip_z_same_value_preserves_cache(self):
        """Setting flip_z to its current value should not clear the cache"""
        model, _ = self._setup_model([1, 2, 3])
        original_flip_z = model.flip_z
        model._cached_projection_objects["dummy_key"] = "cached_value"
        model.flip_z = original_flip_z
        self.assertEqual(len(model._cached_projection_objects), 1)

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_flip_z_uses_separate_cache_keys(self, mock_projection_cls):
        """Flipped and non-flipped projections should use different cache keys"""
        model, _ = self._setup_model([1, 2, 3])
        mock_projection = MagicMock()
        mock_projection.positions.return_value = [[1, 2], [1, 2], [1, 2]]
        mock_projection_cls.return_value = mock_projection
        model._projection_type = ProjectionType.CYLINDRICAL_Y
        model.flip_z = False
        model._calculate_projection()
        self.assertEqual(mock_projection_cls.call_count, 1)
        model.flip_z = True
        model._calculate_projection()
        self.assertEqual(mock_projection_cls.call_count, 2)

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
        np.testing.assert_array_equal(model.pickable_detector_ids, expected_ids)

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
        peaks_workspaces = model.get_workspaces_in_ads_of_type(PeaksWorkspace)
        self.assertEqual(1, len(peaks_workspaces))
        self.assertEqual(mock_peaks_workspace, peaks_workspaces[0])

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

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_cached_projection_objects_empty(self, mock_projection_constructor):
        model, _ = self._setup_model([1, 2, 3])
        model.projection_type = ProjectionType.CYLINDRICAL_X
        mock_projection = MagicMock(positions=MagicMock(return_value=np.array([[1, 2], [1, 2], [1, 2]])))
        mock_projection_constructor.return_value = mock_projection
        model._cached_projection_objects = {}
        positions = model._calculate_projection()
        cache_key = model._cache_key_for_projection(ProjectionType.CYLINDRICAL_X)
        np.testing.assert_almost_equal(positions, np.array([[1, 2, 0], [1, 2, 0], [1, 2, 0]]))
        self.assertEqual(mock_projection, model._cached_projection_objects[cache_key])

    @mock.patch("instrumentview.FullInstrumentViewModel.Projection")
    def test_cached_projection_objects_already_cached(self, mock_projection_constructor):
        model, _ = self._setup_model([1, 2, 3])
        model.projection_type = ProjectionType.CYLINDRICAL_X
        cache_key = model._cache_key_for_projection(ProjectionType.CYLINDRICAL_X)
        mock_projection = MagicMock(positions=MagicMock(return_value=np.array([[1, 2], [1, 2], [1, 2]])))
        model._cached_projection_objects = {cache_key: mock_projection}
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

    def test_mask_ws(self):
        model, _ = self._setup_model([1, 2, 3])
        model._mask_ws = MagicMock()
        _ = model.mask_ws
        model._mask_ws.dataY.assert_called()

    def test_roi_ws(self):
        model, _ = self._setup_model([1, 2, 3])
        model._roi_ws = MagicMock()
        _ = model.roi_ws
        model._roi_ws.dataY.assert_called()

    def test_add_mask(self):
        model, _ = self._setup_model([1, 2, 3])
        model._cached_masks_map = {}
        model._is_masked_in_ws = np.array([True, True, False])
        model._is_valid = np.array([True, True, True])
        model._is_masked = np.array([True, False, False])
        model.add_new_detector_key([True, True], CurrentTab.Masking)
        np.testing.assert_array_equal(list(model._cached_masks_map.values())[0], np.array([True, True, True]))

    def test_roi_mask(self):
        model, _ = self._setup_model([1, 2, 3])
        model._cached_rois_map = {}
        model._is_valid = np.array([True, True, True])
        model._detector_is_picked = np.array([True, False, False])
        model.add_new_detector_key([True, True, False], CurrentTab.Grouping)
        np.testing.assert_array_equal(list(model._cached_rois_map.values())[0], np.array([True, True, False]))

    def test_get_boolean_masks_from_workspaces_in_ads_grouping(self):
        det_ids = [1, 2, 3, 4, 5]
        model, _ = self._setup_model(det_ids)
        # Mock mask ws with one column of zeros and ones
        mock_data_y = np.array([0, 0, 1, 2, 2])
        mock_ws = MagicMock(
            extractY=MagicMock(return_value=mock_data_y),
            getDetectorIDsOfGroup=MagicMock(side_effect=lambda i: np.array(det_ids)[mock_data_y == i]),
        )
        # name() is used internally in the mock object
        mock_ws.configure_mock(**{"name.return_value": "mock_ws"})
        model.get_workspaces_in_ads_of_type = MagicMock(return_value=[mock_ws])
        boolean_mask = model._get_boolean_masks_from_workspaces_in_ads(["mock_ws_1", "mock_ws_2", "other_key"], CurrentTab.Grouping)
        np.testing.assert_allclose(boolean_mask, [[False, False, True, False, False], [False, False, False, True, True]])

    def test_get_boolean_masks_from_workspaces_in_ads_masking(self):
        det_ids = [1, 2, 3, 4, 5]
        model, _ = self._setup_model(det_ids)
        # Mock mask ws with one column of zeros and ones
        mock_data_y = np.array([0, 0, 1, 1, 1])
        mock_ws = MagicMock(
            extractY=MagicMock(return_value=mock_data_y),
            getMaskedDetectors=MagicMock(return_value=np.array(det_ids)[mock_data_y == 1]),
        )
        # name() is used internally in the mock object
        mock_ws.configure_mock(**{"name.return_value": "mock_ws"})
        model.get_workspaces_in_ads_of_type = MagicMock(return_value=[mock_ws])
        boolean_mask = model._get_boolean_masks_from_workspaces_in_ads(["mock_ws", "other_key"], CurrentTab.Masking)
        np.testing.assert_allclose(boolean_mask, [[False, False, True, True, True]])

    def test_apply_detector_mask(self):
        model, _ = self._setup_model([1, 2, 3])
        # All detectors picked, mask should unpick them
        model._detector_is_picked = np.array([True, True, True])
        model._cached_masks_map = {"Mask 1": np.array([True, False, False]), "Mask 2": np.array([False, True, False])}
        model.apply_detector_items(["Mask 1", "Mask 2"], CurrentTab.Masking)
        np.testing.assert_array_equal(model._is_masked, np.array([True, True, False]))

    def test_apply_detector_roi(self):
        model, _ = self._setup_model([1, 2, 3])
        model._point_picked_detectors = np.array([True, False, False])
        model._cached_rois_map = {"1": np.array([False, False, True]), "2": np.array([False, True, False])}
        model.apply_detector_items(["1", "2"], CurrentTab.Grouping)
        np.testing.assert_array_equal(model._detector_is_picked, np.array([True, True, True]))

    def test_apply_detector_mask_empty(self):
        model, _ = self._setup_model([1, 2, 3])
        model._is_masked_in_ws = np.array([True, False, False])
        model.apply_detector_items([], CurrentTab.Masking)
        np.testing.assert_array_equal(model._is_masked, np.array([True, False, False]))
        np.testing.assert_array_equal(model._detector_is_picked, np.array([False, False, False]))

    def test_apply_detector_roi_empty(self):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([True, False, False])
        model._point_picked_detectors = np.array([False, True, False])
        model._cached_rois_map = {"1": np.array([False, False, True]), "2": np.array([False, True, False])}
        model.apply_detector_items([], CurrentTab.Grouping)
        np.testing.assert_array_equal(model._detector_is_picked, np.array([False, True, False]))

    @mock.patch("instrumentview.FullInstrumentViewModel.CloneWorkspace")
    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractMaskToTable")
    def test_save_mask_workspace_to_ads(self, mock_extract_to_table, mock_clone):
        model, _ = self._setup_model([1, 2, 3])
        model.save_workspace_to_ads(CurrentTab.Masking)
        mock_extract_to_table.assert_called_once()
        mock_clone.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.CloneWorkspace")
    @mock.patch("instrumentview.FullInstrumentViewModel.ExtractMaskToTable")
    def test_save_roi_workspace_to_ads(self, mock_extract_to_table, mock_clone):
        model, _ = self._setup_model([1, 2, 3])
        model.save_workspace_to_ads(CurrentTab.Grouping)
        mock_extract_to_table.assert_called_once()
        mock_clone.assert_called_once()

    @mock.patch("instrumentview.FullInstrumentViewModel.SaveMask")
    def test_save_xml_mask(self, mock_save_mask):
        model, _ = self._setup_model([1, 2, 3])
        model.save_mask_to_xml("file")
        mock_save_mask.assert_called_with(model._mask_ws, OutputFile="file.xml")

    @mock.patch("instrumentview.FullInstrumentViewModel.SaveCalFile")
    def test_save_cal_mask(self, mock_save_cal_file):
        model, _ = self._setup_model([1, 2, 3])
        model.save_mask_to_cal("file")
        mock_save_cal_file.assert_called_with(MaskWorkspace=model._mask_ws, Filename="file.cal")

    def test_clear_masks(self):
        model, _ = self._setup_model([1, 2, 3])
        model._cached_masks_map = {"1": 1, "2": 2}
        model.clear_stored_keys(CurrentTab.Masking)
        self.assertEqual(model._cached_masks_map, {})

    def test_clear_rois(self):
        model, _ = self._setup_model([1, 2, 3])
        model._cached_rois_map = {"1": 1, "2": 2}
        model.clear_stored_keys(CurrentTab.Grouping)
        self.assertEqual(model._cached_rois_map, {})

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_delete_all_peaks_no_matching_detector_ids_no_removal(self, mock_wdp_cls, mock_ads):
        """No groups with a detector_id in picked_detector_ids -> no removePeaks calls."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1_wdp = MagicMock()
        ws1_wdp.detector_peaks = [
            DetectorPeaks([self._create_peak(1, 99)]),  # not selected
            DetectorPeaks([self._create_peak(2, 100)]),  # not selected
        ]
        mock_wdp_cls.return_value = ws1_wdp
        mock_ws1 = MagicMock()
        mock_ads.retrieve.return_value = mock_ws1
        model.delete_peaks_on_all_selected_detectors(["ws1"])
        mock_ws1.removePeaks.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_delete_all_peaks_removes_peaks_for_multiple_selected_detectors_in_same_workspace(self, mock_wdp_cls, mock_ads):
        """Aggregates peak indices from all matching detector groups in a workspace."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1_wdp = MagicMock()
        ws1_wdp.detector_peaks = [
            DetectorPeaks([self._create_peak(10, 7), self._create_peak(11, 7)]),
            DetectorPeaks([self._create_peak(20, 8)]),
            DetectorPeaks([self._create_peak(999, 99)]),  # should be ignored
        ]
        mock_wdp_cls.return_value = ws1_wdp
        mock_ws1 = MagicMock()
        mock_ads.retrieve.return_value = mock_ws1
        model.delete_peaks_on_all_selected_detectors(["ws1"])
        # Order preserves traversal: group(7) then group(8)
        mock_ws1.removePeaks.assert_called_once_with([10, 11, 20])

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_delete_all_peaks_aggregates_per_workspace_independently(self, mock_wdp_cls, mock_ads):
        """Each workspace gets its own set of aggregated indices."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        ws1_wdp = MagicMock()
        ws1_wdp.detector_peaks = [
            DetectorPeaks([self._create_peak(1, 7), self._create_peak(2, 7)]),
            DetectorPeaks([self._create_peak(3, 8)]),
        ]
        ws2_wdp = MagicMock()
        ws2_wdp.detector_peaks = [
            DetectorPeaks([self._create_peak(100, 8), self._create_peak(200, 8)]),
        ]
        mock_wdp_cls.side_effect = lambda name, _ws, _specnos: {"ws1": ws1_wdp, "ws2": ws2_wdp}[name]
        mock_ws1 = MagicMock()
        mock_ws2 = MagicMock()
        mock_ads.retrieve.side_effect = lambda name: {"ws1": mock_ws1, "ws2": mock_ws2}[name]
        model.delete_peaks_on_all_selected_detectors(["ws1", "ws2"])
        mock_ws1.removePeaks.assert_called_once_with([1, 2, 3])
        mock_ws2.removePeaks.assert_called_once_with([100, 200])

    def test_delete_all_peaks_no_selected_workspaces_no_crash(self):
        """With no selected workspaces, method exits quietly."""
        model, _ = self._setup_model([1, 7, 8])
        model._detector_is_picked = [False, True, True]
        # Should not raise
        model.delete_peaks_on_all_selected_detectors([])

    def _create_peak(self, peak_index: int, detector_id: int, x: float = 1.0) -> Peak:
        return Peak(detector_id, detector_id, peak_index, (0, 0, 0), x, x, x, x)

    def test_no_selected_workspaces_no_overlay_no_removal(self):
        """When there are no selected workspaces or overlay entries, nothing is removed."""
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = [True, False, False]
        # No workspace exists to assert removePeak calls; just ensure method didn’t crash.
        self.assertEqual(None, model.delete_peak(5.0, []))

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_no_matching_detector_peaks_no_removal(self, mock_wdp_cls, mock_ads):
        """If overlay contains no groups with the picked detector_id, nothing is removed."""
        model, _ = self._setup_model([7])
        model._detector_is_picked = [True]
        # Only detector_id=3 in overlay; model expects detector_id=7
        ws1_wdp = MagicMock()
        ws1_wdp.detector_peaks = [DetectorPeaks([self._create_peak(10, 3, 1.0), self._create_peak(11, 3, 2.0)])]
        mock_wdp_cls.return_value = ws1_wdp
        mock_ws1 = MagicMock()
        mock_ads.retrieve.return_value = mock_ws1
        model.delete_peak(2.2, ["ws1"])
        mock_ws1.removePeak.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_removes_closest_peak_in_single_workspace(self, mock_wdp_cls, mock_ads):
        """Selects and removes the closest peak within a single workspace."""
        model, _ = self._setup_model([7])
        model._spectrum_nos = np.array([7])
        model._detector_is_picked = [True]
        # Peaks at 1.0 (idx=100), 2.0 (idx=101), 10.0 (idx=102); click at 2.2 -> closest is 2.0
        ws1_wdp = MagicMock()
        ws1_wdp.detector_peaks = [
            DetectorPeaks([self._create_peak(100, 7, 1.0), self._create_peak(101, 7, 2.0), self._create_peak(102, 7, 10.0)])
        ]
        mock_wdp_cls.return_value = ws1_wdp
        mock_ws1 = MagicMock()
        mock_ads.retrieve.return_value = mock_ws1
        model.delete_peak(2.2, ["ws1"])
        mock_ws1.removePeak.assert_called_once_with(101)

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_selects_workspace_with_overall_min_distance(self, mock_wdp_cls, mock_ads):
        """Among multiple workspaces, chooses the peak with the smallest distance overall."""
        model, _ = self._setup_model([1, 2, 3, 7])
        model._spectrum_nos = np.array([1, 2, 3, 7])
        model._detector_is_picked = [False, False, False, True]
        # ws1 closest distance = |2.5 - 2.2| = 0.3 (peak_index=201)
        # ws2 closest distance = |2.3 - 2.2| = 0.1 (peak_index=301) -> ws2 should be chosen
        ws1_wdp = MagicMock()
        ws1_wdp.detector_peaks = [DetectorPeaks([self._create_peak(201, 7, 2.5), self._create_peak(202, 7, 100.0)])]
        ws2_wdp = MagicMock()
        ws2_wdp.detector_peaks = [DetectorPeaks([self._create_peak(301, 7, 2.3), self._create_peak(302, 7, 50.0)])]
        mock_wdp_cls.side_effect = lambda name, _ws, _spec_nos: {"ws1": ws1_wdp, "ws2": ws2_wdp}[name]
        mock_ws1 = MagicMock()
        mock_ws2 = MagicMock()
        mock_ads.retrieve.side_effect = lambda name: {"ws1": mock_ws1, "ws2": mock_ws2}[name]
        model.delete_peak(2.2, ["ws1", "ws2"])
        mock_ws2.removePeak.assert_called_once_with(301)
        mock_ws1.removePeak.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.CreatePeaksWorkspace")
    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    def test_get_peaks_workspace_for_adding_new_peak(self, mock_ads, mock_create_peaks_ws):
        model, _ = self._setup_model([1, 2, 3])
        mock_ads.doesExist.return_value = False
        result = model._get_peaks_workspace_for_adding_new_peak([])
        mock_create_peaks_ws.assert_called_once_with(model._workspace, 0, OutputWorkspace=model._instrument_view_peaks_ws_name)
        self.assertEqual(result, model._instrument_view_peaks_ws_name)
        mock_create_peaks_ws.reset_mock()
        mock_ads.doesExist.reset_mock()

        result = model._get_peaks_workspace_for_adding_new_peak(["my_peaks_ws"])
        mock_create_peaks_ws.assert_not_called()
        self.assertEqual(result, "my_peaks_ws")

        mock_ads.doesExist.return_value = True
        result = model._get_peaks_workspace_for_adding_new_peak(["my_peaks_ws1", "my_peaks_ws2"])
        mock_ads.doesExist.assert_called_once_with(model._instrument_view_peaks_ws_name)
        self.assertEqual(result, model._instrument_view_peaks_ws_name)
        mock_ads.doesExist.reset_mock()

        mock_ads.doesExist.return_value = False
        result = model._get_peaks_workspace_for_adding_new_peak(["my_peaks_ws1", "my_peaks_ws2"])
        mock_create_peaks_ws.assert_called_once_with(model._workspace, 0, OutputWorkspace=model._instrument_view_peaks_ws_name)
        self.assertEqual(result, model._instrument_view_peaks_ws_name)

    @mock.patch("instrumentview.FullInstrumentViewModel.AddPeak")
    @mock.patch("instrumentview.FullInstrumentViewModel.FullInstrumentViewModel._get_peaks_workspace_for_adding_new_peak")
    def test_add_peak(self, mock_peaks_ws_for_adding, mock_add_peak):
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([False, False, True])
        mock_peaks_ws_for_adding.return_value = "my_peaks_ws"
        peak_x = 1500
        ws = model.add_peak(peak_x, ["my_peaks_ws"])
        mock_add_peak.assert_called_once_with("my_peaks_ws", model._workspace, peak_x, 3)
        self.assertEqual("my_peaks_ws", ws)

    def test_component_tree_no_matching_detector_ids(self):
        """If no detector_table_indices match, all values become True."""
        component_indices = np.array([1, 2])  # detectorIDs selected: 200, 300 (not in _detector_ids)
        model, mock_ws = self._setup_model([100, 101, 102, 103])
        mock_ws.detectorInfo().detectorIDs.return_value = np.array([100, 200, 300])
        model.component_tree_indices_selected(component_indices)

        np.testing.assert_array_equal(model._is_selected_in_tree, np.array([True, True, True, True]))

    def test_component_tree_some_matching_detector_ids(self):
        """Only matching detector IDs should be set True."""
        component_indices = np.array([0])  # selects detectorID 100 (index 0)
        model, mock_ws = self._setup_model([100, 101, 102, 103])
        mock_ws.detectorInfo().detectorIDs.return_value = np.array([100, 200, 300])
        model.component_tree_indices_selected(component_indices)

        expected = np.array([True, False, False, False])
        np.testing.assert_array_equal(model._is_selected_in_tree, expected)

    def test_component_tree_component_indices_out_of_range(self):
        """Indices >= len(detector_ids) should be ignored."""
        component_indices = np.array([0, 5, 10])  # 5 and 10 ignored
        model, mock_ws = self._setup_model([100, 101, 102, 103])
        mock_ws.detectorInfo().detectorIDs.return_value = np.array([100, 200, 300])
        model.component_tree_indices_selected(component_indices)

        expected = np.array([True, False, False, False])
        np.testing.assert_array_equal(model._is_selected_in_tree, expected)

    def test_component_tree_empty_component_indices(self):
        """Empty input should cause no matches -> all True."""
        component_indices = np.array([])
        model, mock_ws = self._setup_model([100, 101, 102, 103])
        mock_ws.detectorInfo().detectorIDs.return_value = np.array([100, 200, 300])
        model.component_tree_indices_selected(component_indices)

        expected = np.array([True, True, True, True])
        np.testing.assert_array_equal(model._is_selected_in_tree, expected)

    def test_component_tree_all_components_matching(self):
        """Multiple matching IDs should mark multiple positions True."""
        component_indices = np.array([0, 0])  # duplicates fine
        model, mock_ws = self._setup_model([100, 101, 102, 103])
        mock_ws.detectorInfo().detectorIDs.return_value = np.array([100, 200, 300])
        model.component_tree_indices_selected(component_indices)

        expected = np.array([True, False, False, False])
        np.testing.assert_array_equal(model._is_selected_in_tree, expected)

    def test_component_tree_all_not_valid(self):
        """All selected detectors invalid should result in whole tree shown"""
        component_indices = np.array([0, 1])  # duplicates fine
        model, mock_ws = self._setup_model([100, 101, 102, 103])
        mock_ws.detectorInfo().detectorIDs.return_value = np.array([100, 101, 300])
        model._is_valid = np.array([False, False, True, True])
        model.component_tree_indices_selected(component_indices)
        expected = np.array([True] * 4)
        np.testing.assert_array_equal(model._is_selected_in_tree, expected)

    def test_calculate_and_set_full_integration_range_common_bins(self):
        """Test that _calculate_and_set_full_integration_range uses the first valid workspace index for common bins."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.isCommonBins.return_value = True
        mock_workspace.dataX.return_value = np.array([10.0, 20.0, 30.0])
        # Only include detectors 1 and 2 (indices 0, 1)
        valid_mask = np.array([True, True, False])
        model._calculate_and_set_full_integration_range(valid_mask)
        self.assertEqual(model.integration_limits, (10.0, 30.0))
        self.assertEqual(model.full_integration_limits, (10.0, 30.0))
        # Should call dataX with the first workspace index from valid_mask
        mock_workspace.dataX.assert_called_with(0)

    def test_calculate_and_set_full_integration_range_ragged(self):
        """Test that _calculate_and_set_full_integration_range returns min/max across valid spectra for ragged workspaces."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = True
        data_x = {0: np.array([5.0, 10.0, 15.0]), 1: np.array([1.0, 20.0, 50.0]), 2: np.array([100.0, 200.0])}
        mock_workspace.readX.side_effect = lambda i: data_x[i]
        # Only include detectors 0 and 2 (skip detector 1 which has range 1-50)
        valid_mask = np.array([True, False, True])
        model._calculate_and_set_full_integration_range(valid_mask)
        self.assertEqual(model.integration_limits, (5.0, 200.0))
        self.assertEqual(model.full_integration_limits, (5.0, 200.0))

    def test_calculate_and_set_full_integration_range_non_ragged_non_common(self):
        """Test that _calculate_and_set_full_integration_range uses extractX for non-ragged, non-common bins."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.isCommonBins.return_value = False
        mock_workspace.extractX.return_value = np.array([[1.0, 2.0, 3.0], [10.0, 20.0, 30.0], [5.0, 15.0, 50.0]])
        valid_mask = np.array([True, True, True])
        model._calculate_and_set_full_integration_range(valid_mask)
        self.assertEqual(model.integration_limits, (1.0, 50.0))
        self.assertEqual(model.full_integration_limits, (1.0, 50.0))

    def test_calculate_and_set_full_integration_range_excludes_masked_detectors_ragged(self):
        """Test that masking detectors with a wider x range narrows the integration range on a ragged workspace."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = True
        # Detector 1 has range 1-500, detectors 0 and 2 have range 10-100
        data_x = {0: np.array([10.0, 50.0, 100.0]), 1: np.array([1.0, 250.0, 500.0]), 2: np.array([20.0, 60.0, 90.0])}
        mock_workspace.readX.side_effect = lambda i: data_x[i]
        all_valid = np.array([True, True, True])
        model._calculate_and_set_full_integration_range(all_valid)
        self.assertEqual(model.integration_limits, (1.0, 500.0))
        # Now exclude detector 1 (simulating it being masked)
        exclude_det_1 = np.array([True, False, True])
        model._calculate_and_set_full_integration_range(exclude_det_1)
        self.assertEqual(model.integration_limits, (10.0, 100.0))
        self.assertEqual(model.full_integration_limits, (10.0, 100.0))

    def test_calculate_and_set_full_integration_range_excludes_masked_detectors_non_ragged(self):
        """Test that masking detectors with a wider x range narrows the integration range on a non-ragged workspace."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.isCommonBins.return_value = False
        # Detector 0 has range 1-500, detector 2 has range 5-50
        mock_workspace.extractX.return_value = np.array([[1.0, 250.0, 500.0], [10.0, 50.0, 100.0], [5.0, 25.0, 50.0]])
        all_valid = np.array([True, True, True])
        model._calculate_and_set_full_integration_range(all_valid)
        self.assertEqual(model.integration_limits, (1.0, 500.0))
        # Now exclude detector 0
        exclude_det_0 = np.array([False, True, True])
        model._calculate_and_set_full_integration_range(exclude_det_0)
        self.assertEqual(model.integration_limits, (5.0, 100.0))
        self.assertEqual(model.full_integration_limits, (5.0, 100.0))

    def test_public_calculate_and_set_full_integration_range_uses_is_pickable(self):
        """Test that calculate_and_set_full_integration_range uses is_pickable (excluding masked detectors)."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = True
        # Detector 0: 1-500, Detector 1: 10-100, Detector 2: 20-90
        data_x = {0: np.array([1.0, 250.0, 500.0]), 1: np.array([10.0, 50.0, 100.0]), 2: np.array([20.0, 60.0, 90.0])}
        mock_workspace.readX.side_effect = lambda i: data_x[i]
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [100, 200]
        # Mask detector 0 so only detectors 1 and 2 are pickable
        model._is_masked = np.array([True, False, False])
        model._is_valid = np.array([True, True, True])
        model._is_selected_in_tree = np.ones(3, dtype=bool)
        model.calculate_and_set_full_integration_range()
        # Should only consider workspace indices 1 and 2
        self.assertEqual(model.full_integration_limits, (10.0, 100.0))
        self.assertEqual(model.integration_limits, (10.0, 100.0))

    def test_public_calculate_and_set_full_integration_range_updates_counts(self):
        """Test that calculate_and_set_full_integration_range triggers an integration range update (via integration_limits setter)."""
        model, mock_workspace = self._setup_model([1, 2, 3])
        mock_workspace.isRaggedWorkspace.return_value = False
        mock_workspace.isCommonBins.return_value = True
        mock_workspace.dataX.return_value = np.array([10.0, 20.0, 30.0])
        mock_workspace.getIntegratedCountsForWorkspaceIndices.return_value = [50, 150, 250]
        model.calculate_and_set_full_integration_range()
        # The setter calls update_integration_range which calls getIntegratedCountsForWorkspaceIndices
        mock_workspace.getIntegratedCountsForWorkspaceIndices.assert_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_overlay_arguments_filters_nonexistent_workspaces(self, mock_wdp_cls, mock_ads):
        """Workspaces that don't exist in ADS are filtered out."""
        model, _ = self._setup_model([1, 2, 3])
        mock_ads.doesExist.side_effect = lambda name: name == "ws1"
        mock_wdp = MagicMock()
        mock_wdp.get_positions_and_labels.return_value = (np.array([[0, 0, 0]]), ["label1"])
        mock_wdp_cls.return_value = mock_wdp

        positions, labels, ws_names = model.get_peak_overlay_arguments(["ws1", "ws_gone"])

        self.assertEqual(ws_names, ["ws1"])
        self.assertEqual(len(positions), 1)
        self.assertEqual(len(labels), 1)

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_overlay_arguments_empty_list(self, mock_wdp_cls, mock_ads):
        """Empty input returns empty results."""
        model, _ = self._setup_model([1, 2, 3])

        positions, labels, ws_names = model.get_peak_overlay_arguments([])

        self.assertEqual(positions, [])
        self.assertEqual(labels, [])
        self.assertEqual(ws_names, [])
        mock_wdp_cls.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_overlay_arguments_multiple_workspaces(self, mock_wdp_cls, mock_ads):
        """Returns separate positions and labels per workspace."""
        model, _ = self._setup_model([1, 2, 3])
        mock_ads.doesExist.return_value = True

        ws1_wdp = MagicMock()
        ws1_wdp.get_positions_and_labels.return_value = (np.array([[1, 1, 1]]), ["hkl_1"])
        ws2_wdp = MagicMock()
        ws2_wdp.get_positions_and_labels.return_value = (np.array([[2, 2, 2], [3, 3, 3]]), ["hkl_2", "hkl_3"])
        mock_wdp_cls.side_effect = lambda name, _ws, _spec: {"ws1": ws1_wdp, "ws2": ws2_wdp}[name]

        positions, labels, ws_names = model.get_peak_overlay_arguments(["ws1", "ws2"])

        self.assertEqual(ws_names, ["ws1", "ws2"])
        np.testing.assert_array_equal(positions[0], np.array([[1, 1, 1]]))
        np.testing.assert_array_equal(positions[1], np.array([[2, 2, 2], [3, 3, 3]]))
        self.assertEqual(labels[0], ["hkl_1"])
        self.assertEqual(labels[1], ["hkl_2", "hkl_3"])

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_lineplot_overlay_arguments_filters_nonexistent_workspaces(self, mock_wdp_cls, mock_ads):
        """Workspaces that don't exist in ADS are filtered out."""
        model, _ = self._setup_model([1, 2, 3])
        mock_ads.doesExist.side_effect = lambda name: name == "ws1"
        mock_wdp = MagicMock()
        mock_wdp.get_x_values_and_labels.return_value = ([1.5], ["label1"])
        mock_wdp_cls.return_value = mock_wdp

        x_vals, labels, ws_names = model.get_peak_lineplot_overlay_arguments("TOF", ["ws1", "ws_gone"])

        self.assertEqual(ws_names, ["ws1"])
        self.assertEqual(len(x_vals), 1)
        self.assertEqual(len(labels), 1)

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_lineplot_overlay_arguments_empty_list(self, mock_wdp_cls, mock_ads):
        """Empty input returns empty results."""
        model, _ = self._setup_model([1, 2, 3])

        x_vals, labels, ws_names = model.get_peak_lineplot_overlay_arguments("TOF", [])

        self.assertEqual(x_vals, [])
        self.assertEqual(labels, [])
        self.assertEqual(ws_names, [])
        mock_wdp_cls.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_lineplot_overlay_arguments_multiple_workspaces(self, mock_wdp_cls, mock_ads):
        """Returns separate x-values and labels per workspace."""
        model, _ = self._setup_model([1, 2, 3])
        mock_ads.doesExist.return_value = True

        ws1_wdp = MagicMock()
        ws1_wdp.get_x_values_and_labels.return_value = ([100.0], ["hkl_1"])
        ws2_wdp = MagicMock()
        ws2_wdp.get_x_values_and_labels.return_value = ([200.0, 300.0], ["hkl_2", "hkl_3"])
        mock_wdp_cls.side_effect = lambda name, _ws, _spec: {"ws1": ws1_wdp, "ws2": ws2_wdp}[name]

        x_vals, labels, ws_names = model.get_peak_lineplot_overlay_arguments("dSpacing", ["ws1", "ws2"])

        self.assertEqual(ws_names, ["ws1", "ws2"])
        self.assertEqual(x_vals[0], [100.0])
        self.assertEqual(x_vals[1], [200.0, 300.0])
        self.assertEqual(labels[0], ["hkl_1"])
        self.assertEqual(labels[1], ["hkl_2", "hkl_3"])

    @mock.patch("instrumentview.FullInstrumentViewModel.AnalysisDataService")
    @mock.patch("instrumentview.FullInstrumentViewModel.WorkspaceDetectorPeaks")
    def test_get_peak_lineplot_overlay_arguments_passes_unit(self, mock_wdp_cls, mock_ads):
        """The unit argument is forwarded to get_x_values_and_labels."""
        model, _ = self._setup_model([1, 2, 3])
        model._detector_is_picked = np.array([True, False, False])
        mock_ads.doesExist.return_value = True
        mock_wdp = MagicMock()
        mock_wdp.get_x_values_and_labels.return_value = ([], [])
        mock_wdp_cls.return_value = mock_wdp

        model.get_peak_lineplot_overlay_arguments("Wavelength", ["ws1"])

        mock_wdp.get_x_values_and_labels.assert_called_once_with("Wavelength", model.picked_spectrum_nos)


if __name__ == "__main__":
    unittest.main()
