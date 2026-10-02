# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Detectors import DetectorInfo
from instrumentview.Globals import CurrentTab
from instrumentview.Projections.Projection import Projection
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.ComponentSelectionUtils import detector_table_indices_for_parent_subtrees, get_beam_axis, reflect_points_in_axis
from instrumentview.Peaks.Peak import Peak
from instrumentview.Peaks.WorkspaceDetectorPeaks import WorkspaceDetectorPeaks

from mantid.dataobjects import Workspace2D, PeaksWorkspace, MaskWorkspace, GroupingWorkspace
from mantid.simpleapi import (
    CreateDetectorTable,
    ExtractSpectra,
    ConvertUnits,
    AnalysisDataService,
    SumSpectra,
    Rebin,
    ExtractMask,
    ExtractMaskToTable,
    SaveMask,
    SaveCalFile,
    MaskDetectors,
    CloneWorkspace,
    CreatePeaksWorkspace,
    AddPeak,
    CreateGroupingWorkspace,
    SaveDetectorsGrouping,
    DeleteWorkspace,
)
from mantid.api import MatrixWorkspace, WorkspaceUnitValidator
from mantid.kernel import logger
from pathlib import Path
import numpy as np
from enum import Enum
from typing import Optional


class PeakPickingStatus(Enum):
    On = 1
    Off = 2


class FullInstrumentViewModel:
    """Model for the Instrument View Window. Will calculate detector positions, indices, and integrated counts that give the colours"""

    MAX_DET_INFO_SHOWN = 3
    # Every conversion ConvertUnits does goes via TOF, so probing against TOF tells us
    # whether any of the units we offer will work. A workspace already in TOF needs a
    # different target, because converting to the unit it already has is a no-op.
    _PROBE_TARGET = "TOF"
    _PROBE_FALLBACK = "Wavelength"
    # How many times more bins than the spectrum with the most a summed line plot can have,
    # see _rebin_params_for_summing
    _MAX_SUMMED_BINS_FACTOR = 10

    _sample_position = np.array([0, 0, 0])
    _source_position = np.array([0, 0, 0])
    _beam_axis = np.array([0, 0, 1])
    line_plot_workspace = None
    line_plot_det_ids = np.array([], dtype=int)
    _lineplot_ws_in_base_units_not_summed = None
    _lineplot_ws_in_selected_units_not_summed = None
    _line_plot_workspace = None
    _lineplot_limits = None
    _workspace_x_unit: str
    _workspace_x_unit_display: str
    _peak_picking_status: PeakPickingStatus = PeakPickingStatus.Off
    _projection_type: ProjectionType = ProjectionType.THREE_D
    _flip_beam: bool = False
    _u_offset: float = 0.0

    def __init__(self, workspace: Workspace2D):
        """For the given workspace, calculate detector positions, the map from detector indices to workspace indices, and integrated
        counts. Optionally will draw detector geometry, e.g. rectangular bank or tube instead of points."""
        self._workspace = workspace

    def setup(self):
        self._cached_projection_objects = {}
        self._cached_masks_map = {}
        self._cached_rois_map = {}

        x_unit = self._workspace.getAxis(0).getUnit()
        self._workspace_x_unit = x_unit.unitID()
        self._workspace_x_unit_display = f"{str(x_unit.caption())} ({x_unit.symbol().ascii()})"
        self._selected_peaks_workspaces = []
        self._instrument_view_peaks_ws_name = f"instrument_view_peaks_{self._workspace.name()}"

        component_info = self._workspace.componentInfo()
        self._sample_position = np.array(component_info.samplePosition()) if component_info.hasSample() else np.zeros(3)
        has_source = component_info.hasSource()
        self._source_position = np.array(component_info.sourcePosition()) if has_source else np.array([0, 0, 0])
        self._root_position = np.array(component_info.position(0))
        self._beam_axis = get_beam_axis(self._workspace)

        detector_info_table = CreateDetectorTable(
            self._workspace, IncludeDetectorPosition=True, OneRowPerDetectorID=True, StoreInADS=False, EnableLogging=False
        )

        self._detector_ids = detector_info_table.columnArray("Detector ID(s)")
        r = detector_info_table.columnArray("R")
        theta = detector_info_table.columnArray("Theta")
        phi = detector_info_table.columnArray("Phi")
        self._spherical_positions = np.transpose(np.vstack([r, theta, phi]))
        self._detector_positions_3d = detector_info_table.columnArray("Position")
        self._workspace_indices = detector_info_table.columnArray("Index")
        # Array of strings 'yes', 'no' and 'n/a'
        self._is_monitor = detector_info_table.columnArray("Monitor")
        self._is_valid = (self._is_monitor != "yes") & (self._workspace_indices != -1)
        self._component_idxs = np.arange(len(self._detector_ids))
        # TODO: Add masked column to detector table
        self._is_masked_in_ws = np.array([self._workspace.detectorInfo().isMasked(i) for i, _ in enumerate(self._detector_ids)])

        # For computing current mask, detached from the permanent mask in ws
        self._is_masked = self._is_masked_in_ws
        self._is_selected_in_tree = np.ones_like(self._is_masked, dtype=bool)
        self._monitor_positions = self._detector_positions_3d[self._is_monitor == "yes"]

        # Initialise with zeros
        self._counts = np.zeros_like(self._detector_ids)
        self._counts_limits = (0, 0)
        self._detector_is_picked = np.full(len(self._detector_ids), False)
        self._current_detector_groupings = np.zeros_like(self._detector_ids)
        self._point_picked_detectors = np.full(len(self._detector_ids), False)
        # Sized for this workspace, as leaving peak picking restores it, and that can happen after
        # the workspace is replaced
        self._point_picked_detectors_cached = self._point_picked_detectors.copy()

        # Worked out on demand, see can_convert_units
        self._can_convert_units: Optional[bool] = None
        self._units_failed_to_convert: set[str] = set()

        self._integration_workspace = self._workspace.clone(StoreInADS=False)
        self._calculate_and_set_full_integration_range(self._is_valid)
        # Update counts with default total range
        self.update_integration_range(True)
        self.full_counts_limits = self._counts_limits

        self._sample_shape = self._get_sample_shape_from_workspace(self._workspace)
        self._transform = np.eye(4)
        self._transformed_detector_positions = self.detector_positions.copy()

        self._peaks_indices_in_detector_positions = np.array([], dtype=int)

    @property
    def workspace(self) -> Workspace2D:
        return self._workspace

    @property
    def has_unit(self) -> bool:
        """Whether the x axis has a unit that ConvertUnits would accept.

        Not the same as ``unitID() != "Empty"``: Label, Degrees, Temperature and AtomicDistance
        all derive from the Empty unit and are rejected too, despite their own unit IDs.
        """
        return not WorkspaceUnitValidator("").isValid(self._workspace)

    @property
    def can_convert_units(self) -> bool:
        """Whether this workspace's units can usefully be converted, see _probe_unit_conversion.

        Worked out on first use and cached until the next setup(), because probing costs a
        conversion and the views that never offer a unit choice should not pay for it.
        """
        if self._can_convert_units is None:
            self._can_convert_units = self._probe_unit_conversion()
        return self._can_convert_units

    @property
    def can_show_peaks(self) -> bool:
        """Whether peaks can be shown on this workspace.

        A peak is only drawn, or found again to delete, where it has a location in the workspace
        unit, so peaks overlaid on a workspace in any other unit, e.g. Energy, would not be seen.
        """
        return Peak.can_be_located_in(self.workspace_base_unit)

    @property
    def can_add_peaks(self) -> bool:
        """Whether peaks can be added to this workspace and then seen, see can_show_peaks.

        AddPeak converts the clicked position to time of flight, which needs units that can be
        converted.
        """
        return self.can_convert_units and self.can_show_peaks

    @property
    def workspace_base_unit(self) -> str:
        return self._workspace_x_unit

    @property
    def workspace_x_unit_display(self) -> str:
        return self._workspace_x_unit_display

    @property
    def sample_position(self) -> np.ndarray:
        return self._sample_position

    def _get_sample_shape_from_workspace(self, ws: Workspace2D) -> Optional[np.ndarray]:
        shape = ws.sample().getShape()
        return shape.getMesh() if shape.hasValidShape() else None

    @property
    def sample_shape(self) -> Optional[np.ndarray]:
        return self._sample_shape

    @property
    def all_detector_ids(self) -> np.ndarray:
        """All detector IDs (unfiltered), in the same order as CreateDetectorTable."""
        return self._detector_ids

    @property
    def pickable_detector_ids(self) -> np.ndarray:
        """Detector IDs for unmasked, non-monitor detectors."""
        return self._detector_ids[self.is_pickable]

    @property
    def masked_detector_ids(self) -> np.ndarray:
        """Detector IDs for masked (but valid, non-monitor) detectors."""
        return self._detector_ids[self._is_masked & self._is_valid]

    @property
    def monitor_positions(self) -> np.ndarray:
        return self._monitor_positions

    @property
    def is_pickable(self) -> np.ndarray:
        return ~self._is_masked & self._is_valid & self._is_selected_in_tree

    @property
    def picked_detector_mask(self) -> np.ndarray:
        return self._detector_is_picked[self.is_pickable]

    @property
    def point_picked_detectors(self) -> np.ndarray:
        """A copy of the mask over all detectors of those picked directly in the projection.

        A copy because the model picks into this array in place, so callers holding on to it as a
        snapshot would otherwise see it change underneath them.
        """
        return self._point_picked_detectors.copy()

    @property
    def picked_visibility(self) -> np.ndarray:
        """picked_detector_mask as the numeric scalars the renderers hand to VTK."""
        return self.picked_detector_mask.astype(int)

    @property
    def _is_picked_and_pickable(self) -> np.ndarray:
        """Mask over all detectors of those that are both pickable and currently selected.

        Unlike picked_detector_mask this has one entry per detector, so it indexes the
        per-detector arrays built in setup().
        """
        return self.is_pickable & self._detector_is_picked

    @property
    def picked_detector_ids(self) -> np.ndarray:
        return self._detector_ids[self._is_picked_and_pickable]

    @property
    def picked_workspace_indices(self) -> np.ndarray:
        return self._workspace_indices[self._is_picked_and_pickable]

    @property
    def picked_detector_positions_3d(self) -> np.ndarray:
        return self._detector_positions_3d[self._is_picked_and_pickable]

    @property
    def picked_spherical_positions(self) -> np.ndarray:
        return self._spherical_positions[self._is_picked_and_pickable]

    @property
    def picked_counts(self) -> np.ndarray:
        return self._counts[self._is_picked_and_pickable]

    @property
    def detector_counts(self) -> np.ndarray:
        return self._counts[self.is_pickable]

    @property
    def counts_limits(self) -> tuple[int, int]:
        return self._counts_limits

    @counts_limits.setter
    def counts_limits(self, limits) -> None:
        try:
            min, max = limits
            assert int(max) > int(min)
        except (ValueError, AssertionError):
            return
        self._counts_limits = limits

    @property
    def mask_ws(self) -> MatrixWorkspace:
        # TODO: Fix MaskDetectors so it doesn't require ws to be in ADS
        tmp_name = "_tmp"
        tmp = self._workspace.clone(StoreInADS=True, OutputWorkspace=tmp_name)
        try:
            tmp.maskDetectors(DetectorList=self._detector_ids[self._is_masked])
            mask_ws, _ = ExtractMask(tmp, StoreInADS=False)
            return mask_ws
        finally:
            if AnalysisDataService.doesExist(tmp_name):
                DeleteWorkspace(tmp_name)

    @property
    def roi_ws(self) -> MatrixWorkspace:
        # TODO: Fix MaskDetectors so it doesn't require ws to be in ADS
        tmp_name = "_tmp"
        tmp = self._workspace.clone(StoreInADS=True, OutputWorkspace=tmp_name)
        try:
            tmp.maskDetectors(DetectorList=self._detector_ids[~self._detector_is_picked])
            roi_ws, _ = ExtractMask(tmp, StoreInADS=False)
            return roi_ws
        finally:
            if AnalysisDataService.doesExist(tmp_name):
                DeleteWorkspace(tmp_name)

    @property
    def integration_limits(self) -> tuple[float, float]:
        return self._integration_limits

    @integration_limits.setter
    def integration_limits(self, limits) -> None:
        try:
            min, max = limits
            assert float(max) >= float(min)
        except (ValueError, AssertionError):
            return
        self._integration_limits = limits
        # Update the counts
        self.update_integration_range(entire_range=False)

    def update_integration_range(self, entire_range: bool = False) -> None:
        workspace_indices = self._workspace_indices[self.is_pickable]
        if len(workspace_indices) == 0:
            self._counts[self.is_pickable] = 0
            self._counts_limits = (0, 0)
            self.full_counts_limits = self._counts_limits
            return

        new_detector_counts = np.array(
            self._integration_workspace.getIntegratedCountsForWorkspaceIndices(
                workspace_indices,
                len(workspace_indices),
                float(self.integration_limits[0]),
                float(self.integration_limits[1]),
                entire_range,
            ),
            dtype=int,
        )
        self._counts_limits = (np.min(new_detector_counts), np.max(new_detector_counts))
        self.full_counts_limits = self._counts_limits
        self._counts[self.is_pickable] = new_detector_counts

    @property
    def lineplot_limits(self) -> tuple | None:
        return self._lineplot_limits

    @property
    def units_failed_to_convert(self) -> set[str]:
        """The units a conversion to has failed since setup(), and which are shown in the
        workspace unit instead."""
        return self._units_failed_to_convert

    def calculate_and_set_full_integration_range(self) -> None:
        self._calculate_and_set_full_integration_range(self.is_pickable)
        self.integration_limits = self.full_integration_limits

    def _calculate_and_set_full_integration_range(self, valid_indices: np.ndarray) -> None:
        workspace_indices = self._workspace_indices[valid_indices]
        self._integration_limits = self._extract_limits_from_workspace(self._integration_workspace, workspace_indices)
        self.full_integration_limits = self._integration_limits

    def _extract_limits_from_workspace(self, workspace, workspace_indices=None) -> tuple:
        if workspace_indices is None:
            workspace_indices = np.arange(0, workspace.getNumberHistograms())

        if len(workspace_indices) == 0:
            return (0, 0)

        if workspace.isRaggedWorkspace():
            first_last = np.array([workspace.x(int(i))[[0, -1]] for i in workspace_indices])
            limits = (np.min(first_last[:, 0]), np.max(first_last[:, 1]))
        elif workspace.isCommonBins():
            limits = tuple(workspace.x(int(workspace_indices[0]))[[0, -1]])
        else:
            data_x = workspace.extractX()[workspace_indices]
            limits = (np.min(data_x[:, 0]), np.max(data_x[:, -1]))

        if np.isfinite(limits).all():
            return limits
        return self._finite_limits_from_workspace(workspace, workspace_indices)

    @staticmethod
    def _finite_limits_from_workspace(workspace, workspace_indices) -> tuple:
        """The x range of the given spectra, leaving out any edges that are not finite.

        Converting units can turn an x value of zero into an infinite one, e.g. TOF into momentum
        transfer, and neither the sliders nor the plot axes can take infinite limits.
        """
        min_x = np.inf
        max_x = -np.inf
        for ws_index in workspace_indices:
            data_x = np.asarray(workspace.x(int(ws_index)))
            data_x = data_x[np.isfinite(data_x)]
            if data_x.size > 0:
                min_x = min(min_x, data_x.min())
                max_x = max(max_x, data_x.max())
        if min_x > max_x:
            return (0, 0)
        return (min_x, max_x)

    def _probe_unit_conversion(self) -> bool:
        """Whether ConvertUnits can usefully convert this workspace."""
        if not self.has_unit:
            return False

        valid_indices = self._workspace_indices[self._is_valid & ~self._is_masked_in_ws]
        if len(valid_indices) == 0:
            # Every detector is masked, so there is no spectrum whose conversion can be checked
            return False

        target = self._PROBE_TARGET if self._workspace_x_unit != self._PROBE_TARGET else self._PROBE_FALLBACK
        try:
            probe = ExtractSpectra(
                InputWorkspace=self._workspace,
                WorkspaceIndexList=[int(valid_indices[0])],
                EnableLogging=False,
                StoreInADS=False,
            )
            converted = ConvertUnits(InputWorkspace=probe, Target=target, EMode="Elastic", EnableLogging=False, StoreInADS=False)
        except (RuntimeError, ValueError) as e:
            logger.warning(f"Cannot convert the units of {self._workspace.name()}, unit selection disabled: {e}")
            return False

        # ConvertUnits masks the spectra it could not convert rather than failing, so a spectrum
        # it masked means the conversion succeeded but produced nothing worth showing.
        return not converted.spectrumInfo().isMasked(0)

    def _convert_units(self, workspace, unit) -> Optional[MatrixWorkspace]:
        """Convert workspace to unit, or None if that turns out not to be possible.

        _probe_unit_conversion should have ruled that out already. This is here so a failure
        can never escape into a Qt slot, where it would take the window down with it.
        """
        try:
            return ConvertUnits(InputWorkspace=workspace, Target=unit, EMode="Elastic", EnableLogging=False, StoreInADS=False)
        except (RuntimeError, ValueError) as e:
            logger.warning(f"Could not convert {self._workspace.name()} to {unit}, showing {self.workspace_base_unit} instead: {e}")
            self._units_failed_to_convert.add(unit)
            return None

    def set_integration_units(self, unit):
        converted = None
        if self.can_convert_units and unit != self.workspace_base_unit:
            converted = self._convert_units(self._workspace, unit)
        if converted is None:
            converted = self._workspace.clone(EnableLogging=False, StoreInADS=False)
        self._integration_workspace = converted

    def get_integration_units(self):
        return self._integration_workspace.getAxis(0).getUnit().unitID()

    def _detector_table_indices_for_parent_subtree(self, selected_indices: np.ndarray) -> np.ndarray:
        return detector_table_indices_for_parent_subtrees(
            selected_indices=selected_indices,
            component_idxs=self._component_idxs,
            component_info=self._workspace.componentInfo(),
            pickable_mask=self.is_pickable,
        )

    def expand_pickable_mask_to_parent_subtrees(self, pickable_mask: list[bool] | np.ndarray) -> np.ndarray:
        pickable_mask = np.array(pickable_mask, dtype=bool)
        pickable_table_indices = np.argwhere(self.is_pickable).flatten()
        if pickable_mask.size != pickable_table_indices.size:
            raise ValueError("pickable_mask must have one value per pickable detector")

        selected_pickable_indices = pickable_table_indices[pickable_mask]
        expanded_pickable_table_indices = self._detector_table_indices_for_parent_subtree(selected_pickable_indices)

        expanded_pickable_mask = np.zeros_like(pickable_mask, dtype=bool)
        if expanded_pickable_table_indices.size == 0:
            return expanded_pickable_mask

        expanded_pickable_mask[np.isin(pickable_table_indices, expanded_pickable_table_indices)] = True
        return expanded_pickable_mask

    def peak_picking_enabled(self) -> bool:
        return self._peak_picking_status == PeakPickingStatus.On

    def turn_on_single_point_picking(self) -> None:
        self._peak_picking_status = PeakPickingStatus.On
        self._point_picked_detectors_cached = self._point_picked_detectors.copy()
        self._point_picked_detectors[:] = False

    def turn_off_single_point_picking(self) -> None:
        self._peak_picking_status = PeakPickingStatus.Off
        self._point_picked_detectors = self._point_picked_detectors_cached
        # Assuming groupings have been turned off for while peak picking
        # Only need to update picked detectors with point picked
        # Restoring groupings will handle the rest
        self._detector_is_picked = self._point_picked_detectors

    def update_point_picked_detectors(self, index: int, pick_detector_with_peak, expand_to_parent_subtree: bool) -> None:
        if pick_detector_with_peak:
            index = self._get_index_of_closest_detector_with_peak(index)

        if self._peak_picking_status == PeakPickingStatus.Off:
            global_index = np.argwhere(self.is_pickable).flatten()[index]
            indices_to_update = np.array([global_index], dtype=int)

            if expand_to_parent_subtree:
                indices_to_update = self._detector_table_indices_for_parent_subtree(indices_to_update)

            new_selection_value = ~self._detector_is_picked[global_index]
            self._detector_is_picked[indices_to_update] = new_selection_value
            self._point_picked_detectors[indices_to_update] = new_selection_value

        elif self._peak_picking_status == PeakPickingStatus.On:
            global_index = np.argwhere(self.is_pickable)[index]
            self._detector_is_picked[:] = False
            self._detector_is_picked[global_index] = True
            self._point_picked_detectors[:] = False
            self._point_picked_detectors[global_index] = True

    def _build_detector_info_list(
        self,
        ws_indices: np.ndarray,
        det_ids: np.ndarray,
        xyz_positions: np.ndarray,
        spherical_positions: np.ndarray,
        counts: np.ndarray,
    ) -> list[DetectorInfo]:
        return [
            DetectorInfo(det.getName(), det_id, ws_index, xyz, spherical, det.getFullName(), int(count))
            for ws_index, det_id, xyz, spherical, count in zip(ws_indices, det_ids, xyz_positions, spherical_positions, counts, strict=True)
            for det in (self._workspace.getDetector(int(ws_index)),)
        ]

    def detector_info_text_for_workspace_index(self, picked_index: int) -> list[DetectorInfo]:

        pickable_indices = np.argwhere(self.is_pickable).flatten()
        index = [pickable_indices[picked_index]]

        return self._build_detector_info_list(
            self._workspace_indices[index],
            self._detector_ids[index],
            self._detector_positions_3d[index],
            self._spherical_positions[index],
            self._counts[index],
        )

    def clear_point_picked_detectors(self, detectors: Optional[np.ndarray] = None) -> None:
        """Deselect detectors picked directly in the projection.

        Pass a mask over all detectors to clear only those, leaving any picked since it was taken.
        """
        to_clear = self._point_picked_detectors if detectors is None else self._point_picked_detectors & detectors
        self._detector_is_picked[to_clear] = False
        self._point_picked_detectors[to_clear] = False

    def picked_detectors_info_text(self) -> list[DetectorInfo]:
        """For the specified detector, extract info that can be displayed in the View, and wrap it all up in a DetectorInfo class"""

        if len(self.picked_detector_ids) > self.MAX_DET_INFO_SHOWN:
            return []

        return self._build_detector_info_list(
            self.picked_workspace_indices,
            self.picked_detector_ids,
            self.picked_detector_positions_3d,
            self.picked_spherical_positions,
            self.picked_counts,
        )

    def get_projection_options(self) -> list[str]:
        return [p.value for p in ProjectionType]

    def get_default_projection(self) -> ProjectionType:
        possible_returns_map = {
            "3D": ProjectionType.THREE_D,
            "SPHERICAL_X": ProjectionType.SPHERICAL_X,
            "SPHERICAL_Y": ProjectionType.SPHERICAL_Y,
            "SPHERICAL_Z": ProjectionType.SPHERICAL_Z,
            "CYLINDRICAL_X": ProjectionType.CYLINDRICAL_X,
            "CYLINDRICAL_Y": ProjectionType.CYLINDRICAL_Y,
            "CYLINDRICAL_Z": ProjectionType.CYLINDRICAL_Z,
        }
        return possible_returns_map[self._workspace.instrumentDefaultView()]

    @property
    def projection_type(self):
        return self._projection_type

    @projection_type.setter
    def projection_type(self, value: str):
        self._projection_type = ProjectionType(value)

    @property
    def is_2d_projection(self) -> bool:
        if self._projection_type == ProjectionType.THREE_D:
            return False
        return True

    @property
    def detector_positions(self) -> np.ndarray:
        if self._projection_type == ProjectionType.THREE_D:
            return self._detector_positions_3d[self.is_pickable]
        return self._calculate_projection()[self.is_pickable]

    @property
    def transform(self) -> np.ndarray:
        return self._transform

    @transform.setter
    def transform(self, value: np.ndarray) -> None:
        self._transform = value
        self._transformed_detector_positions = self._transform_vectors_with_matrix(self.detector_positions)

    @property
    def transformed_detector_positions(self) -> np.ndarray:
        return self._transformed_detector_positions

    def _transform_vectors_with_matrix(self, points: np.ndarray, transform: Optional[np.ndarray] = None) -> np.ndarray:
        if points.size == 0:
            return points
        if transform is None:
            transform = self._transform

        # The transform is a 4x4 matrix while the points are 3D vectors,
        # so first append the homogeneous coordinate.
        transformed_points = np.hstack([points, np.ones((points.shape[0], 1))])
        transformed_points = transformed_points @ transform.T
        return transformed_points[:, :3]

    @property
    def masked_positions(self) -> np.ndarray:
        if self._projection_type == ProjectionType.THREE_D:
            return self._detector_positions_3d[(self._is_masked | ~self._is_selected_in_tree) & self._is_valid]
        return self._calculate_projection()[(self._is_masked | ~self._is_selected_in_tree) & self._is_valid]

    @property
    def flip_beam(self) -> bool:
        if self._projection_type in (ProjectionType.THREE_D, ProjectionType.SIDE_BY_SIDE):
            return False
        return self._flip_beam

    @flip_beam.setter
    def flip_beam(self, value: bool) -> None:
        self._flip_beam = value

    @property
    def u_offset(self) -> float:
        """The angle, in radians, that a 2D projection is rotated by about its axis."""
        if self._projection_type in (ProjectionType.THREE_D, ProjectionType.SIDE_BY_SIDE):
            return 0.0
        return self._u_offset

    @u_offset.setter
    def u_offset(self, value: float) -> None:
        self._u_offset = value

    def _cache_key_for_projection(self, projection_type: ProjectionType) -> str:
        return f"{projection_type.name}_flip_{self.flip_beam}"

    def _calculate_projection(self) -> np.ndarray:
        """Calculate the 2D projection with the specified axis. Can be either cylindrical or spherical."""
        cache_key = self._cache_key_for_projection(self._projection_type)
        if cache_key not in self._cached_projection_objects.keys():
            detector_positions = self._detector_positions_3d
            if self.flip_beam:
                detector_positions = reflect_points_in_axis(detector_positions, axis=self._beam_axis)

            projection = Projection(
                type=self._projection_type,
                workspace=self._workspace,
                detector_ids=self._detector_ids,
                sample_position=self._sample_position,
                root_position=self._root_position,
                detector_positions=detector_positions,
            )
            self._cached_projection_objects[cache_key] = projection

        projection = self._cached_projection_objects[cache_key]
        projection.set_u_offset(self.u_offset)
        projected_positions = np.zeros_like(self._detector_positions_3d)
        projected_positions[:, :2] = projection.positions()  # Assign only x and y coordinate
        return projected_positions

    @property
    def active_projection(self):
        """Return the active projection object for the current projection type."""
        if self._projection_type == ProjectionType.THREE_D:
            return None

        cache_key = self._cache_key_for_projection(self._projection_type)
        projection = self._cached_projection_objects.get(cache_key)
        if projection is None:
            self._calculate_projection()
            projection = self._cached_projection_objects.get(cache_key)
        if projection is not None:
            projection.set_u_offset(self.u_offset)
        return projection

    @staticmethod
    def _rebin_params_for_summing(workspace) -> Optional[list[float]]:
        """Rebin parameters covering every spectrum at its finest binning, or None if there are none.

        We have to loop over the spectra because otherwise ragged workspaces will have their bin
        edge vector truncated.

        Converting units can leave bin edges Rebin will not accept. An x axis that starts at zero
        maps to an infinite edge in momentum transfer or wavelength, and a detector in the path of
        the beam has no scattering angle, so in momentum transfer its every edge is zero. Infinite
        edges can simply be left out of the range, but a spectrum with no width left at all is one
        Rebin cannot bin whatever parameters it is given, so give up on the whole set instead.

        The finite edges either side of an infinite one can still be huge, so the finest width
        across that range could make for more bins than there is memory to hold. The width is
        widened to keep the count within _MAX_SUMMED_BINS_FACTOR times the most any spectrum had.
        """
        min_bin_edge = np.inf
        max_bin_edge = -np.inf
        min_bin_width = np.inf
        max_bin_count = 0
        for ws_index in range(workspace.getNumberHistograms()):
            bin_edges = np.asarray(workspace.x(ws_index))
            bin_edges = bin_edges[np.isfinite(bin_edges)]
            bin_widths = np.diff(bin_edges)
            bin_widths = bin_widths[bin_widths > 0]
            if bin_widths.size == 0:
                return None
            min_bin_edge = min(min_bin_edge, bin_edges.min())
            max_bin_edge = max(max_bin_edge, bin_edges.max())
            min_bin_width = min(min_bin_width, bin_widths.min())
            max_bin_count = max(max_bin_count, bin_widths.size)

        if not np.isfinite([min_bin_edge, max_bin_edge, min_bin_width]).all() or min_bin_edge >= max_bin_edge:
            return None

        max_summed_bins = FullInstrumentViewModel._MAX_SUMMED_BINS_FACTOR * max_bin_count
        min_bin_width = max(min_bin_width, (max_bin_edge - min_bin_edge) / max_summed_bins)

        return [float(min_bin_edge), float(min_bin_width), float(max_bin_edge)]

    def _sum_spectra_for_line_plot(self, workspace, unit: str):
        """Sum the extracted spectra onto one curve, or return them unsummed if that is not possible.

        Showing the spectra unsummed keeps a line plot on screen. There is nothing to be gained
        from failing outright when they cannot be put on a common binning.
        """
        try:
            if not workspace.isCommonBins():
                # Rebin the selected spectra onto the widest range at the finest binning, so that
                # they can be summed
                rebin_params = self._rebin_params_for_summing(workspace)
                if rebin_params is None:
                    logger.warning(
                        f"Cannot put the selected spectra of {self._workspace.name()} on a common binning "
                        f"in {unit}, so they are plotted unsummed."
                    )
                    return workspace
                workspace = Rebin(InputWorkspace=workspace, Params=rebin_params, EnableLogging=False, StoreInADS=False)
            return SumSpectra(InputWorkspace=workspace, EnableLogging=False, StoreInADS=False)
        except (RuntimeError, ValueError) as e:
            logger.warning(f"Could not sum the selected spectra in {unit}, so they are plotted unsummed: {e}")
            return workspace

    def extract_spectra_for_line_plot(self, unit: str, sum_spectra: bool, picked_indices: Optional[np.ndarray] = None) -> None:
        self._current_linplot_unit = unit

        if picked_indices is None:
            det_ids = np.unique(self.picked_detector_ids)
        else:
            pickable_indices = np.argwhere(self.is_pickable).flatten()
            det_ids = self._detector_ids[pickable_indices[picked_indices]]

        self.line_plot_det_ids = det_ids

        if len(det_ids) == 0:
            self.line_plot_workspace = None
            self._lineplot_ws_in_base_units_not_summed = None
            self._lineplot_ws_in_selected_units_not_summed = None
            self._lineplot_limits = None
            return

        self._lineplot_ws_in_base_units_not_summed = ExtractSpectra(
            InputWorkspace=self._workspace, DetectorList=det_ids, EnableLogging=False, StoreInADS=False
        )
        converted = None
        if self.can_convert_units and unit != self.workspace_base_unit:
            converted = self._convert_units(self._lineplot_ws_in_base_units_not_summed, unit)
        self._lineplot_ws_in_selected_units_not_summed = converted if converted is not None else self._lineplot_ws_in_base_units_not_summed

        tmp_ws = self._lineplot_ws_in_selected_units_not_summed
        if sum_spectra and len(det_ids) > 1:
            tmp_ws = self._sum_spectra_for_line_plot(tmp_ws, unit)

        self.line_plot_workspace = tmp_ws
        self._lineplot_limits = self._extract_limits_from_workspace(self.line_plot_workspace)

    def save_line_plot_workspace_to_ads(self) -> None:
        if self.line_plot_workspace is None or len(self.picked_workspace_indices) == 0:
            return
        name_exported_ws = f"instrument_view_selected_spectra_{self._workspace.name()}"
        AnalysisDataService.addOrReplace(name_exported_ws, self.line_plot_workspace)

    def get_peak_overlay_arguments(self, selected_peaks_workspaces: list[str]) -> tuple:
        selected_peaks_workspaces = [ws for ws in selected_peaks_workspaces if AnalysisDataService.doesExist(ws)]
        wrapped_workspaces = [
            WorkspaceDetectorPeaks(ws_name, self.get_integration_units(), self.integration_limits) for ws_name in selected_peaks_workspaces
        ]
        indices_and_labels_by_pws = [wws.get_peaks_indices_and_labels(self.pickable_detector_ids) for wws in wrapped_workspaces]
        indices_by_pws = [pair[0] for pair in indices_and_labels_by_pws]
        self._peaks_indices_in_detector_positions = np.concatenate(indices_by_pws or [np.array([], dtype=int)])
        positions_by_pws = [self.detector_positions[indices] for indices in indices_by_pws]
        labels_by_pws = [pair[1] for pair in indices_and_labels_by_pws]

        transformed_pos = [self._transform_vectors_with_matrix(p) for p in positions_by_pws]
        return transformed_pos, labels_by_pws, selected_peaks_workspaces

    def _get_index_of_closest_detector_with_peak(self, index_in_detector_positions: int) -> int:
        clicked_position = self.transformed_detector_positions[index_in_detector_positions]
        positions_detectors_with_peaks = self.transformed_detector_positions[self._peaks_indices_in_detector_positions]
        if len(positions_detectors_with_peaks) == 0:
            return index_in_detector_positions
        closest_peak_index = np.argmin(np.linalg.norm(positions_detectors_with_peaks - clicked_position, axis=1))
        return self._peaks_indices_in_detector_positions[closest_peak_index]

    @staticmethod
    def _located_peaks(peaks: list[Peak], unit: str) -> list[tuple[Peak, float]]:
        """Pair each peak with its location in unit, leaving out the peaks that have none."""
        located_peaks = []
        for peak in peaks:
            location = peak.location_in_unit(unit)
            if location is not None:
                located_peaks.append((peak, location))
        return located_peaks

    def get_peak_lineplot_overlay_arguments(
        self, selected_peaks_workspaces: list[str]
    ) -> tuple[list[list[float]], list[list[str]], list[str]]:
        if not self._lineplot_ws_in_base_units_not_summed:
            return [], [], []

        selected_peaks_workspaces = [ws for ws in selected_peaks_workspaces if AnalysisDataService.doesExist(ws)]
        wrapped_workspaces = [
            WorkspaceDetectorPeaks(ws_name, self.get_integration_units(), self._integration_limits) for ws_name in selected_peaks_workspaces
        ]
        # Key off the detectors actually plotted rather than the picked selection: they differ
        # when the plot is previewing an overlaid shape or a hovered detector, and the x-position
        # lookup below can only resolve detectors present in the extracted line plot workspace.
        peaks_by_pws = [wws.get_x_values_and_labels(self.line_plot_det_ids) for wws in wrapped_workspaces]
        # A peak can only be drawn where its position in the workspace unit is known, so drop
        # the ones where it isn't, taking their labels with them to keep the two lists aligned.
        located_peaks_by_pws = [self._located_peaks(peaks, self._workspace_x_unit) for peaks in peaks_by_pws]
        labels_by_pws = [[p.label for p, _ in peaks] for peaks in located_peaks_by_pws]
        # Convert peak units to currently plotted units
        # NOTE: Need to get x coords in workspace unit for better acuracy
        # Cannot trust units in peak workspaces
        converted_x = [
            [
                self._match_workspace_unit(
                    self._lineplot_ws_in_base_units_not_summed,
                    self._lineplot_ws_in_base_units_not_summed.getIndicesFromDetectorIDs([p.detector_id])[0],
                    location,
                    self._lineplot_ws_in_selected_units_not_summed,
                )
                for p, location in peaks
            ]
            for peaks in located_peaks_by_pws
        ]
        return converted_x, labels_by_pws, selected_peaks_workspaces

    def add_peak(self, x_in_lineplot_unit: float, selected_peaks_workspaces: list[str]) -> str:
        peaks_ws = self._get_peaks_workspace_for_adding_new_peak(selected_peaks_workspaces)
        detector_id = self.picked_detector_ids[0]
        x_in_workspace_unit = self._match_workspace_unit(
            # self.line_plot_workspace, 0, x_in_integration_unit, self._lineplot_ws_in_base_units_not_summed
            self._lineplot_ws_in_selected_units_not_summed,
            0,
            x_in_lineplot_unit,
            self._lineplot_ws_in_base_units_not_summed,
        )
        AddPeak(peaks_ws, self._workspace, x_in_workspace_unit, int(detector_id))
        return peaks_ws

    def _match_workspace_unit(self, ws_from, idx, x_from: float, ws_to):

        # Find closest dataX cell in integration workspace and get the value of that cell in self._workspace
        data_x_from = ws_from.x(int(idx))[:]
        data_x_to = ws_to.x(int(idx))[:]

        # ConvertUnits does not output one-to-one dataX for momentum transfer
        # Need to use inverse of dataX to get correct one-to-one match of dataX
        unit_from = ws_from.getAxis(0).getUnit().name().casefold()
        unit_to = ws_to.getAxis(0).getUnit().name().casefold()

        if unit_from == "q" or unit_from == "momentumtransfer":
            data_x_from = data_x_from[::-1]

        if unit_to == "q" or unit_to == "momentumtransfer":
            data_x_to = data_x_to[::-1]

        return data_x_to[np.argmin(np.abs(data_x_from - x_from))]

    def _get_peaks_workspace_for_adding_new_peak(self, selected_peaks_workspaces: list[str]) -> PeaksWorkspace:
        # If exactly one Peaks workspace in selected, add the peak to that workspace, otherwise
        # use a special workspace, which we create if it doesn't exist already.
        if len(selected_peaks_workspaces) == 1:
            return selected_peaks_workspaces[0]
        if AnalysisDataService.doesExist(self._instrument_view_peaks_ws_name):
            return self._instrument_view_peaks_ws_name
        CreatePeaksWorkspace(self._workspace, 0, OutputWorkspace=self._instrument_view_peaks_ws_name)
        return self._instrument_view_peaks_ws_name

    def delete_peak(self, x_in_integration_unit: float, selected_peaks_workspaces: list[str]) -> None:
        if len(selected_peaks_workspaces) == 0:
            return

        x_in_workspace_unit = self._match_workspace_unit(
            self.line_plot_workspace, 0, x_in_integration_unit, self._lineplot_ws_in_base_units_not_summed
        )
        closest_peak_by_ws = []
        for ws_name in selected_peaks_workspaces:
            peaks_by_detector = WorkspaceDetectorPeaks(ws_name, self.get_integration_units(), self.integration_limits).detector_peaks
            picked_detector_peaks = [p for p in peaks_by_detector if p.detector_id in self.picked_detector_ids]
            if len(picked_detector_peaks) == 0:
                continue
            peaks = sum([p.peaks for p in picked_detector_peaks], [])
            # Now we have all the peaks on the selected detector, so we need to find the closest
            # peak to where the mouse was clicked. A peak with no position in the workspace unit
            # has no distance to compare, so it cannot be the one that was clicked on.
            located_peaks = self._located_peaks(peaks, self.workspace_base_unit)
            if len(located_peaks) == 0:
                continue
            distance_to_click = np.abs([location - x_in_workspace_unit for _, location in located_peaks])
            index_of_closest = np.argmin(distance_to_click)
            closest_peak = located_peaks[index_of_closest][0]
            closest_peak_by_ws.append((ws_name, closest_peak.peak_index, distance_to_click[index_of_closest]))

        if len(closest_peak_by_ws) == 0:
            return

        closest_over_all_workspaces = min(closest_peak_by_ws, key=lambda x: x[2])
        AnalysisDataService.retrieve(closest_over_all_workspaces[0]).removePeak(closest_over_all_workspaces[1])

    def delete_peaks_on_all_selected_detectors(self, selected_peaks_workspaces) -> None:
        for ws_name in selected_peaks_workspaces:
            peaks_by_detector = WorkspaceDetectorPeaks(ws_name, self.get_integration_units(), self.integration_limits).detector_peaks
            picked_detector_peaks = [p for p in peaks_by_detector if p.detector_id in self.picked_detector_ids]
            if len(picked_detector_peaks) == 0:
                continue
            peaks_to_remove = sum([[p.peak_index for p in detector.peaks] for detector in picked_detector_peaks], [])
            AnalysisDataService.retrieve(ws_name).removePeaks(peaks_to_remove)

    def relative_detector_angle(self) -> float:
        picked_ids = self.picked_detector_ids
        if len(picked_ids) != 2:
            raise RuntimeError("Relative detector angle only valid when two detectors are selected")
        q_lab_1 = self._calculate_q_lab_direction(picked_ids[0])
        q_lab_2 = self._calculate_q_lab_direction(picked_ids[1])
        return np.degrees(np.arccos(np.clip(np.dot(q_lab_1, q_lab_2), -1.0, 1.0)))

    def _calculate_q_lab_direction(self, detector_id: int) -> np.ndarray:
        detector_info = self.workspace.detectorInfo()
        detector_index = detector_info.indexOf(int(detector_id))
        two_theta = detector_info.twoTheta(detector_index)
        phi = detector_info.azimuthal(detector_index)
        q_lab = np.array([-np.sin(two_theta) * np.cos(phi), -np.sin(two_theta) * np.sin(phi), 1 - np.cos(two_theta)])
        return q_lab / np.linalg.norm(q_lab)

    def add_new_detector_key(self, new_value: list[bool], kind: CurrentTab):
        if kind is CurrentTab.Masking:
            new_key = f"Mask {len(self._cached_masks_map) + 1} (unsaved)"
        else:
            new_key = f"Pick Selection {len(self._cached_rois_map) + 1} (unsaved)"
        return self.set_detector_key(new_key, new_value, kind)

    def set_detector_key(self, key: str, new_value: list[bool], kind: CurrentTab) -> str:
        """Store new_value (one entry per pickable detector) under key, replacing any existing entry."""
        if kind is CurrentTab.Masking:
            mask_to_save = self._is_masked_in_ws.copy()
            mask_to_save[self.is_pickable] = new_value
            self._cached_masks_map[key] = mask_to_save
        else:
            selection_to_save = np.zeros_like(self._workspace_indices, dtype=bool)
            selection_to_save[self.is_pickable] = new_value
            self._cached_rois_map[key] = selection_to_save
        return key

    def _get_boolean_masks_from_workspaces_in_ads(self, selected_keys: list[str], kind: CurrentTab):
        ws_in_ads = (
            self.get_workspaces_in_ads_of_type(MaskWorkspace)
            if kind is CurrentTab.Masking
            else self.get_workspaces_in_ads_of_type(GroupingWorkspace)
        )
        booleans_from_ws = []
        for key in selected_keys:
            for ws in ws_in_ads:
                if not key.startswith(ws.name()):
                    continue

                if kind is CurrentTab.Masking:
                    # TODO: Make Detector Table include masked column since this method is slow
                    # Faster to use CreateDetectorTable to get an array of the masked detectors
                    boolean_mask = np.isin(self._detector_ids, ws.getMaskedDetectors())
                else:
                    # TODO: Figure out if using numpy arrays is faster than getDetectorIDsOfGroup
                    # groups = ws.extractY().flatten()
                    # boolean_mask = np.isin(self._detector_ids, det_ids[groups == int(key.split("_")[-1])])
                    boolean_mask = np.isin(self._detector_ids, ws.getDetectorIDsOfGroup(int(key.split("_")[-1])))

                booleans_from_ws.append(boolean_mask)
        return booleans_from_ws

    def apply_detector_items(self, selected_keys: list[str], kind: CurrentTab):
        if kind is CurrentTab.Masking:
            booleans_from_ws = self._get_boolean_masks_from_workspaces_in_ads(selected_keys, CurrentTab.Masking)
            cached_masks = [self._cached_masks_map[key] for key in selected_keys if key in self._cached_masks_map.keys()]
            total_items = [*booleans_from_ws, *cached_masks]
            if not total_items:
                self._is_masked = self._is_masked_in_ws
                return
            self._is_masked = np.logical_or.reduce(total_items)
            return
        else:
            booleans_from_ws = self._get_boolean_masks_from_workspaces_in_ads(selected_keys, CurrentTab.Grouping)
            cached_selections = [self._cached_rois_map[key] for key in selected_keys if key in self._cached_rois_map.keys()]
            total_items = [self._point_picked_detectors, *booleans_from_ws, *cached_selections]
            # Filter out empty boolean masks
            total_items = [item for item in total_items if np.any(item)]
            if not total_items:
                self._detector_is_picked = self._point_picked_detectors
                self._current_detector_groupings[self._point_picked_detectors] = 1
                return
            self._detector_is_picked = np.logical_or.reduce(total_items)
            self._current_detector_groupings.fill(0)
            for i, group in enumerate(total_items):
                self._current_detector_groupings[group] = i + 1
            return

    def clear_stored_keys(self, kind: CurrentTab) -> None:
        if kind is CurrentTab.Masking:
            self._cached_masks_map.clear()
        else:
            self._cached_rois_map.clear()

    def cached_keys(self, kind: CurrentTab) -> list[str]:
        if kind is CurrentTab.Masking:
            return list(self._cached_masks_map.keys())
        else:
            return list(self._cached_rois_map.keys())

    @property
    def cached_pick_selections_keys(self) -> list[str]:
        return list(self._cached_rois_map.keys())

    def save_workspace_to_ads(self, kind: CurrentTab):
        if kind is CurrentTab.Masking:
            ws_to_save = self.mask_ws
        else:
            ws_to_save = self.roi_ws

        xmin, xmax = self._integration_limits
        ExtractMaskToTable(ws_to_save, Xmin=xmin, Xmax=xmax, OutputWorkspace="MaskTable")
        CloneWorkspace(ws_to_save, OutputWorkspace="MaskWorkspace")

    def save_mask_to_xml(self, filename):
        ws_to_save = self.mask_ws
        if not filename:
            return
        if Path(filename).suffix != ".xml":
            filename += ".xml"
        SaveMask(ws_to_save, OutputFile=filename)

    def save_mask_to_cal(self, filename):
        ws_to_save = self.mask_ws
        if not filename:
            return
        if Path(filename).suffix != ".cal":
            filename += ".cal"
        SaveCalFile(MaskWorkspace=ws_to_save, Filename=filename)

    def overwrite_mask_to_current_workspace(self) -> None:
        MaskDetectors(self._workspace.name(), MaskedWorkspace=self.mask_ws)

    def get_workspaces_in_ads_of_type(self, ws_type: MaskWorkspace | GroupingWorkspace | PeaksWorkspace):
        # TODO: Figure out how to avoid using a dictionary
        str_types = {MaskWorkspace: "MaskWorkspace", GroupingWorkspace: "GroupingWorkspace", PeaksWorkspace: "PeaksWorkspace"}
        ads = AnalysisDataService.Instance()
        workspaces_in_ads = ads.retrieveWorkspaces(ads.getObjectNames())
        return [
            pws
            for pws in workspaces_in_ads
            if str_types[ws_type] in str(type(pws)) and pws.getInstrument().getFullName() == self._workspace.getInstrument().getFullName()
        ]

    def get_grouping_keys_from_workspaces_in_ads(self):
        return [
            gws.name() + f"_{i}"
            for gws in self.get_workspaces_in_ads_of_type(GroupingWorkspace)
            for i in range(1, gws.getGroupIDs().max() + 1)
        ]

    def save_grouping_to_ads(self):
        grouping_name = "GroupingWorkspace"
        grouping_ws = self._create_current_grouping_workspace(grouping_name)
        AnalysisDataService.addOrReplace(grouping_name, grouping_ws)

    def save_grouping_to_xml(self, filename):
        if not filename:
            return
        if Path(filename).suffix != ".xml":
            filename += ".xml"
        grouping_name = "__temp_grouping_workspace_to_save"
        grouping_ws = self._create_current_grouping_workspace(grouping_name)
        SaveDetectorsGrouping(grouping_ws, filename)
        DeleteWorkspace(grouping_name)
        return

    def save_grouping_to_cal(self, filename):
        if not filename:
            return
        if Path(filename).suffix != ".cal":
            filename += ".cal"
        grouping_name = "__temp_grouping_workspace_to_save"
        grouping_ws = self._create_current_grouping_workspace(grouping_name)
        SaveCalFile(GroupingWorkspace=grouping_ws, Filename=filename)
        DeleteWorkspace(grouping_name)
        return

    def _create_current_grouping_workspace(self, grouping_name):
        # TODO: Ideally algorithm should use workspace as ADS hanging already fixed
        individual_groups_strings = []
        for i in range(1, self._current_detector_groupings.max() + 1):
            individual_groups_strings.append("+".join([str(id) for id in self._detector_ids[self._current_detector_groupings == i]]))

        CreateGroupingWorkspace(
            InstrumentFilename=self._workspace.instrumentFilename(),
            ComponentName=self._workspace.getInstrument().getFullName(),
            CustomGroupingString=",".join(individual_groups_strings),
            OutputWorkspace=grouping_name,
        )
        return AnalysisDataService.retrieve(grouping_name)

    @property
    def bank_groups_by_detector_id(self) -> list[tuple[list[int], str]] | None:
        """Return detector IDs grouped by bank for side-by-side projection.

        Returns None if not in side-by-side projection or if the projection
        doesn't support bank grouping.  Each element is
        ``(detector_ids, bank_type)``.
        """
        if self.active_projection is None or self.active_projection.type is not ProjectionType.SIDE_BY_SIDE:
            return []
        return self.active_projection.get_bank_groups_by_detector_id()

    def component_tree_indices_selected(self, component_indices: np.ndarray) -> None:
        if len(component_indices) == 0:
            self._is_selected_in_tree.fill(True)
            return
        # The first components in the tree are the detectors, but the order of the detectors
        # is different when you access CreateDetectorTable compared to their order in the
        # component tree
        detector_ids = self._workspace.detectorInfo().detectorIDs()
        detector_ids = detector_ids[component_indices[component_indices < len(detector_ids)]]
        detector_table_indices = np.nonzero(np.isin(self._detector_ids, detector_ids))[0]
        if len(detector_table_indices) == 0 or np.all(~self._is_valid[detector_table_indices]):
            self._is_selected_in_tree.fill(True)
            return
        self._is_selected_in_tree.fill(False)
        self._is_selected_in_tree[detector_table_indices] = True
        return
