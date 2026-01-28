# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Detectors import DetectorInfo
from instrumentview.Globals import CurrentTab
from instrumentview.Peaks.Peak import Peak
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Projections.SphericalProjection import SphericalProjection
from instrumentview.Projections.CylindricalProjection import CylindricalProjection
from instrumentview.Projections.SideBySide import SideBySide
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.ConvertUnitsCalculator import ConvertUnitsCalculator

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
    MaskDetectors,
    CloneWorkspace,
    CreatePeaksWorkspace,
    AddPeak,
    CreateGroupingWorkspace,
    SaveDetectorsGrouping,
    DeleteWorkspace,
)
from mantid.api import MatrixWorkspace
from itertools import groupby
from pathlib import Path
import numpy as np


class FullInstrumentViewModel:
    """Model for the Instrument View Window. Will calculate detector positions, indices, and integrated counts that give the colours"""

    _sample_position = np.array([0, 0, 0])
    _source_position = np.array([0, 0, 0])
    _invalid_index = -1
    _data_min = 0.0
    _data_max = 0.0
    line_plot_workspace = None
    _workspace_x_unit: str
    _workspace_x_unit_display: str
    _selected_peaks_workspaces: list[PeaksWorkspace]

    def __init__(self, workspace: Workspace2D):
        """For the given workspace, calculate detector positions, the map from detector indices to workspace indices, and integrated
        counts. Optionally will draw detector geometry, e.g. rectangular bank or tube instead of points."""
        self._workspace = workspace

    def setup(self):
        x_unit = self._workspace.getAxis(0).getUnit()
        self._workspace_x_unit = x_unit.unitID()
        self._workspace_x_unit_display = f"{str(x_unit.caption())} ({str(x_unit.symbol())})"
        self._selected_peaks_workspaces = []
        self._instrument_view_peaks_ws_name = f"instrument_view_peaks_{self._workspace.name()}"
        self._unit_converter = ConvertUnitsCalculator(self._workspace)

        component_info = self._workspace.componentInfo()
        self._sample_position = np.array(component_info.samplePosition()) if component_info.hasSample() else np.zeros(3)
        has_source = self._workspace.getInstrument().getSource() is not None
        self._source_position = np.array(component_info.sourcePosition()) if has_source else np.array([0, 0, 0])
        self._root_position = np.array(component_info.position(0))

        detector_info_table = CreateDetectorTable(
            self._workspace, IncludeDetectorPosition=True, PickOneDetectorID=True, StoreInADS=False, EnableLogging=False
        )

        # Might have comma-separated multiple detectors, choose first one in the string in that case
        self._detector_ids = detector_info_table.columnArray("Detector ID(s)")
        r = detector_info_table.columnArray("R")
        theta = detector_info_table.columnArray("Theta")
        phi = detector_info_table.columnArray("Phi")
        self._spherical_positions = np.transpose(np.vstack([r, theta, phi]))
        self._detector_positions_3d = detector_info_table.columnArray("Position")
        self._workspace_indices = detector_info_table.columnArray("Index")
        self._spectrum_nos = detector_info_table.columnArray("Spectrum No")
        # Array of strings 'yes', 'no' and 'n/a'
        self._is_monitor = detector_info_table.columnArray("Monitor")
        self._is_valid = self._is_monitor == "no"
        self._mask_ws, _ = ExtractMask(self._workspace, StoreInADS=False)
        self._roi_ws = self._mask_ws.clone(StoreInADS=False)
        self._is_masked_in_ws = self._mask_ws.extractY().flatten().astype(bool)
        # For computing current mask, detached from the permanent mask in ws
        self._is_masked = self._is_masked_in_ws
        self._monitor_positions = self._detector_positions_3d[self._is_monitor == "yes"]

        # Initialise with zeros
        self._counts = np.zeros_like(self._detector_ids)
        self._counts_limits = (0, 0)
        self._detector_is_picked = np.full(len(self._detector_ids), False)
        self._current_detector_groupings = np.zeros_like(self._detector_ids)
        self._point_picked_detectors = np.full(len(self._detector_ids), False)

        self._projection_type = ProjectionType.THREE_D
        self._cached_projections_map = {}

        self._cached_masks_map = {}
        self._cached_rois_map = {}

        # Get min and max integration values
        if self._workspace.isRaggedWorkspace():
            first_last = np.array([self._workspace.readX(i)[[0, -1]] for i in self._workspace_indices[self._is_valid]])
            self._integration_limits = (np.min(first_last[:, 0]), np.max(first_last[:, 1]))

        elif self._workspace.isCommonBins():
            self._integration_limits = tuple(self._workspace.dataX(0)[[0, -1]])

        else:
            data_x = self._workspace.extractX()[self._is_valid]
            self._integration_limits = (np.min(data_x[:, 0]), np.max(data_x[:, -1]))

        # Update counts with default total range
        self.update_integration_range(self._integration_limits, True)

    @property
    def workspace(self) -> Workspace2D:
        return self._workspace

    @property
    def has_unit(self) -> bool:
        return self._workspace_x_unit != "Empty"

    @property
    def workspace_x_unit(self) -> str:
        return self._workspace_x_unit

    @property
    def workspace_x_unit_display(self) -> str:
        return self._workspace_x_unit_display

    @property
    def sample_position(self) -> np.ndarray:
        return self._sample_position

    @property
    def detector_ids(self) -> np.ndarray:
        return self._detector_ids[self.is_pickable]

    @property
    def spectrum_nos(self) -> np.ndarray:
        return self._spectrum_nos[self.is_pickable]

    @property
    def monitor_positions(self) -> np.ndarray:
        return self._monitor_positions

    @property
    def is_pickable(self) -> np.ndarray:
        return ~self._is_masked & self._is_valid

    @property
    def picked_visibility(self) -> np.ndarray:
        return self._detector_is_picked.astype(int)[self.is_pickable]

    @property
    def picked_detector_ids(self) -> np.ndarray:
        return self._detector_ids[self.is_pickable & self._detector_is_picked]

    @property
    def picked_spectrum_nos(self) -> np.ndarray:
        return self._spectrum_nos[self.is_pickable & self._detector_is_picked]

    @property
    def picked_workspace_indices(self) -> np.ndarray:
        return self._workspace_indices[self.is_pickable & self._detector_is_picked]

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
        # Don't need to check detector IDs because ExtractMask outputs same order of det ids
        for i, v in enumerate(self._is_masked):
            self._mask_ws.dataY(i)[:] = v
        return self._mask_ws

    @property
    def roi_ws(self) -> MatrixWorkspace:
        for i, v in enumerate(~self._detector_is_picked):
            self._roi_ws.dataY(i)[:] = v
        return self._roi_ws

    @property
    def integration_limits(self) -> tuple[float, float]:
        return self._integration_limits

    @integration_limits.setter
    def integration_limits(self, limits) -> None:
        try:
            min, max = limits
            assert float(max) > float(min)
        except (ValueError, AssertionError):
            return
        self._integration_limits = limits
        # Update the counts
        self.update_integration_range(self._integration_limits)

    def update_integration_range(self, integration_limits: tuple[float, float], entire_range: bool = False) -> None:
        integration_min, integration_max = integration_limits
        workspace_indices = self._workspace_indices[self.is_pickable]
        new_detector_counts = np.array(
            self._workspace.getIntegratedCountsForWorkspaceIndices(
                workspace_indices, len(workspace_indices), float(integration_min), float(integration_max), entire_range
            ),
            dtype=int,
        )
        self._counts_limits = (np.min(new_detector_counts), np.max(new_detector_counts))
        self._counts[self.is_pickable] = new_detector_counts

    def update_point_picked_detectors(self, index: int) -> None:
        # TODO: Check which selection is quicker, mask or indices
        # NOTE: This is slightly awkard because cannot do chained mask selections
        global_index = np.argwhere(self.is_pickable)[index]
        self._detector_is_picked[global_index] = ~self._detector_is_picked[global_index]
        self._point_picked_detectors[global_index] = self._detector_is_picked[global_index]

    def clear_point_picked_detectors(self) -> None:
        self._detector_is_picked[self._point_picked_detectors] = False
        self._point_picked_detectors.fill(False)

    def picked_detectors_info_text(self) -> list[DetectorInfo]:
        """For the specified detector, extract info that can be displayed in the View, and wrap it all up in a DetectorInfo class"""

        picked_ws_indices = self._workspace_indices[self._detector_is_picked]
        picked_ids = self._detector_ids[self._detector_is_picked]
        picked_xyz_positions = self._detector_positions_3d[self._detector_is_picked]
        picked_spherical_positions = self._spherical_positions[self._detector_is_picked]
        picked_counts = self._counts[self._detector_is_picked]

        picked_info = []
        for i, ws_index in enumerate(picked_ws_indices):
            ws_detector = self._workspace.getDetector(int(ws_index))
            name = ws_detector.getName()
            component_path = ws_detector.getFullName()
            det_info = DetectorInfo(
                name, picked_ids[i], ws_index, picked_xyz_positions[i], picked_spherical_positions[i], component_path, int(picked_counts[i])
            )
            picked_info.append(det_info)
        return picked_info

    def get_default_projection_index_and_options(self) -> tuple[int, list[str]]:
        possible_returns_map = {
            "3D": ProjectionType.THREE_D,
            "SPHERICAL_X": ProjectionType.SPHERICAL_X,
            "SPHERICAL_Y": ProjectionType.SPHERICAL_Y,
            "SPHERICAL_Z": ProjectionType.SPHERICAL_Z,
            "CYLINDRICAL_X": ProjectionType.CYLINDRICAL_X,
            "CYLINDRICAL_Y": ProjectionType.CYLINDRICAL_Y,
            "CYLINDRICAL_Z": ProjectionType.CYLINDRICAL_Z,
        }
        default_projection_type = possible_returns_map[self._workspace.getInstrument().getDefaultView()]
        projection_options = [p.value for p in ProjectionType]
        return projection_options.index(default_projection_type.value), projection_options

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
    def masked_positions(self) -> np.ndarray:
        if self._projection_type == ProjectionType.THREE_D:
            return self._detector_positions_3d[self._is_masked & self._is_valid]
        return self._calculate_projection()[self._is_masked & self._is_valid]

    def _calculate_projection(self) -> np.ndarray:
        """Calculate the 2D projection with the specified axis. Can be either cylindrical or spherical."""

        if self._projection_type.name in self._cached_projections_map.keys():
            return self._cached_projections_map[self._projection_type.name]

        axis = [1, 0, 0]
        if self._projection_type in (ProjectionType.SPHERICAL_Y, ProjectionType.CYLINDRICAL_Y):
            axis = [0, 1, 0]
        elif self._projection_type in (ProjectionType.SPHERICAL_Z, ProjectionType.CYLINDRICAL_Z):
            axis = [0, 0, 1]

        if self._projection_type in (ProjectionType.SPHERICAL_X, ProjectionType.SPHERICAL_Y, ProjectionType.SPHERICAL_Z):
            projection = SphericalProjection(self._sample_position, self._root_position, self._detector_positions_3d, np.array(axis))
        elif self._projection_type in (ProjectionType.CYLINDRICAL_X, ProjectionType.CYLINDRICAL_Y, ProjectionType.CYLINDRICAL_Z):
            projection = CylindricalProjection(self._sample_position, self._root_position, self._detector_positions_3d, np.array(axis))
        else:
            projection = SideBySide(
                self._workspace, self._detector_ids, self._sample_position, self._root_position, self._detector_positions_3d, np.array(axis)
            )

        projected_positions = np.zeros_like(self._detector_positions_3d)
        projected_positions[:, :2] = projection.positions()  # Assign only x and y coordinate

        self._cached_projections_map[self._projection_type.name] = projected_positions
        return projected_positions

    def extract_spectra_for_line_plot(self, unit: str, sum_spectra: bool) -> None:
        workspace_indices = self.picked_workspace_indices
        if len(workspace_indices) == 0:
            self.line_plot_workspace = None
            return

        ws = ExtractSpectra(InputWorkspace=self._workspace, WorkspaceIndexList=workspace_indices, EnableLogging=False, StoreInADS=False)
        if self.has_unit and unit != self.workspace_x_unit:
            ws = ConvertUnits(InputWorkspace=ws, target=unit, EMode="Elastic", EnableLogging=False, StoreInADS=False)

        if sum_spectra and len(workspace_indices) > 1:
            # Find the spectrum with the widest range, and the one with the smallest bin width and use that
            # combo to rebin the selected spectra. We have to loop over the spectra because otherwise ragged
            # workspaces will have their bin edge vector truncated
            if not ws.isCommonBins():
                min_bin_edge = np.inf
                max_bin_edge = 0
                min_bin_width = np.inf
                for ws_index_i in range(len(workspace_indices)):
                    bin_edges = ws.readX(ws_index_i)
                    min_bin_edge = min(min_bin_edge, bin_edges[0])
                    max_bin_edge = max(max_bin_edge, bin_edges[-1])
                    min_bin_width = min(min_bin_width, np.min(np.diff(bin_edges)))

                ws = Rebin(InputWorkspace=ws, Params=[min_bin_edge, min_bin_width, max_bin_edge], EnableLogging=False, StoreInADS=False)

            ws = SumSpectra(InputWorkspace=ws, EnableLogging=False, StoreInADS=False)

        self.line_plot_workspace = ws

    def save_line_plot_workspace_to_ads(self) -> None:
        if self.line_plot_workspace is None or len(self.picked_workspace_indices) == 0:
            return
        name_exported_ws = f"instrument_view_selected_spectra_{self._workspace.name()}"
        AnalysisDataService.addOrReplace(name_exported_ws, self.line_plot_workspace)

    def set_peaks_workspaces(self, peaks_workspace_names: list[str]) -> None:
        self._selected_peaks_workspaces = AnalysisDataService.Instance().retrieveWorkspaces(peaks_workspace_names)

    def peak_overlay_points(self) -> dict[str : list[DetectorPeaks]]:
        detector_info = self._workspace.detectorInfo()
        peaks_grouped_by_ws = {}
        for pws in self._selected_peaks_workspaces:
            peaks = []
            peaks_dict = pws.toDict()
            detector_ids = peaks_dict["DetID"]
            workspace_indices = self.workspace.getIndicesFromDetectorIDs(detector_ids)
            spectrum_nos = self._spectrum_nos[workspace_indices]
            hkls = zip(peaks_dict["h"], peaks_dict["k"], peaks_dict["l"], strict=True)
            positions = [np.array(detector_info.position(detector_info.indexOf(id))) for id in detector_ids]
            tofs = peaks_dict["TOF"]
            dspacings = peaks_dict["DSpacing"]
            wavelengths = peaks_dict["Wavelength"]
            peaks += [
                Peak(det_id, spec_no, v, peak_idx, hkl, tof, dspacing, wavelength, 2 * np.pi / dspacing)
                for (det_id, spec_no, v, peak_idx, hkl, tof, dspacing, wavelength) in zip(
                    detector_ids, spectrum_nos, positions, range(len(tofs)), hkls, tofs, dspacings, wavelengths, strict=True
                )
            ]
            # Combine peaks on the same detector
            detector_peaks = []
            # groupby groups consecutive matches, so must be sorted
            peaks.sort(key=lambda x: x.spectrum_no)
            for spec_no, peaks_for_spec in groupby(peaks, lambda x: x.spectrum_no):
                if spec_no in self.spectrum_nos:
                    detector_peaks.append(DetectorPeaks(list(peaks_for_spec)))

            peaks_grouped_by_ws[pws.name()] = detector_peaks
        return peaks_grouped_by_ws

    def _peaks_workspace_for_adding_new_peak(self, selected_peaks_workspaces: list[str]) -> PeaksWorkspace:
        # If exactly one Peaks workspace in selected, add the peak to that workspace, otherwise
        # use a special workspace, which we create if it doesn't exist already.
        ads = AnalysisDataService.Instance()
        if len(selected_peaks_workspaces) == 1:
            return ads.retrieveWorkspaces(selected_peaks_workspaces)[0]
        if ads.doesExist(self._instrument_view_peaks_ws_name):
            return ads.retrieveWorkspaces([self._instrument_view_peaks_ws_name])[0]
        peaks_ws = CreatePeaksWorkspace(self._workspace, 0, OutputWorkspace=self._instrument_view_peaks_ws_name, StoreInADS=False)
        ads.addOrReplace(self._instrument_view_peaks_ws_name, peaks_ws)
        return peaks_ws

    def add_peak(self, x_in_workspace_unit: float, selected_peaks_workspaces: list[str]) -> str:
        peaks_ws = self._peaks_workspace_for_adding_new_peak(selected_peaks_workspaces)
        detector_id = self.picked_detector_ids[0]
        AddPeak(peaks_ws, self._workspace, x_in_workspace_unit, int(detector_id))
        return peaks_ws.name()

    def delete_peak(self, x_in_workspace_unit: float) -> None:
        detector_ids = self.picked_detector_ids
        peaks_grouped_by_ws = self.peak_overlay_points()
        closest_peak_by_ws = []
        for peaks_ws in self._selected_peaks_workspaces:
            if peaks_ws.name() not in peaks_grouped_by_ws:
                continue
            peaks_by_detector = peaks_grouped_by_ws[peaks_ws.name()]
            picked_detector_peaks = [p for p in peaks_by_detector if p.detector_id in detector_ids]
            if len(picked_detector_peaks) == 0:
                continue
            peaks = sum([p.peaks for p in picked_detector_peaks], [])
            # Now we have all the peaks on the selected detector, so we need to find the
            # closest peak to where the mouse was clicked
            distance_to_click = np.abs([p.location_in_unit(self.workspace_x_unit) - x_in_workspace_unit for p in peaks])
            index_of_closest = np.argmin(distance_to_click)
            closest_peak = peaks[index_of_closest]
            closest_peak_by_ws.append((peaks_ws, closest_peak.peak_index, distance_to_click[index_of_closest]))

        if len(closest_peak_by_ws) == 0:
            return

        closest_over_all_workspaces = min(closest_peak_by_ws, key=lambda x: x[2])
        closest_over_all_workspaces[0].removePeak(closest_over_all_workspaces[1])

    def delete_peaks_on_all_selected_detectors(self) -> None:
        peaks_grouped_by_ws = self.peak_overlay_points()
        for peaks_ws in self._selected_peaks_workspaces:
            if peaks_ws.name() not in peaks_grouped_by_ws:
                continue
            peaks_by_detector = peaks_grouped_by_ws[peaks_ws.name()]
            picked_detector_peaks = [p for p in peaks_by_detector if p.detector_id in self.picked_detector_ids]
            if len(picked_detector_peaks) == 0:
                continue
            peaks_to_remove = sum([[p.peak_index for p in detector.peaks] for detector in picked_detector_peaks], [])
            peaks_ws.removePeaks(peaks_to_remove)

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
            mask_to_save = self._is_masked_in_ws.copy()
            mask_to_save[self.is_pickable] = new_value
            self._cached_masks_map[new_key] = mask_to_save
            return new_key
        else:
            new_key = f"Pick Selection {len(self._cached_rois_map) + 1} (unsaved)"
            selection_to_save = np.zeros_like(self._workspace_indices, dtype=bool)
            selection_to_save[self.is_pickable] = new_value
            self._cached_rois_map[new_key] = selection_to_save
            return new_key

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
                    # NOTE: This is a roundabout way of getting masked detector ids because ws.getMaskedDetectors() is much slower
                    det_table = CreateDetectorTable(ws, PickOneDetectorID=True, StoreInADS=False, EnableLogging=False)
                    det_ids = det_table.columnArray("Detector ID(s)")
                    is_mask = ws.extractY().flatten().astype(bool)
                    boolean_mask = np.isin(self._detector_ids, det_ids[is_mask])
                else:
                    # TODO: Figure out if using numpy arrays is faster than getDetectorIDsOfGroup
                    # groups = ws.extractY().flatten()
                    # boolean_mask = np.isin(self._detector_ids, det_ids[groups == int(key[-1])])
                    boolean_mask = np.isin(self._detector_ids, ws.getDetectorIDsOfGroup(int(key[-1])))

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

    def overwrite_mask_to_current_workspace(self) -> None:
        ws_to_save = self.mask_ws
        # TODO: Check if copies are expensive with big workspaces
        temp_ws = CloneWorkspace(self._workspace.name(), StoreInADS=False)
        temp_ws_name = f"__instrument_view_temp_{self._workspace.name()}"
        AnalysisDataService.addOrReplace(temp_ws_name, temp_ws)
        MaskDetectors(temp_ws_name, MaskedWorkspace=ws_to_save)
        AnalysisDataService.addOrReplace(self._workspace.name(), AnalysisDataService.retrieve(temp_ws_name))

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

    def _create_current_grouping_workspace(self, grouping_name):
        # TODO: Ideally algorithm should use workspace when ADS hanging is fixed
        individual_groups_strings = []
        for i in range(1, self._current_detector_groupings.max() + 1):
            individual_groups_strings.append("+".join([str(id) for id in self._detector_ids[self._current_detector_groupings == i]]))

        CreateGroupingWorkspace(
            InstrumentFilename=self._workspace.getInstrument().getFilename(),
            ComponentName=self._workspace.getInstrument().getFullName(),
            CustomGroupingString=",".join(individual_groups_strings),
            OutputWorkspace=grouping_name,
        )
        return AnalysisDataService.retrieve(grouping_name)

    def convert_units(self, source_unit: str, target_unit: str, picked_detector_index: int, value: float) -> float:
        return self._unit_converter.convert(
            source_unit,
            target_unit,
            self.picked_spectrum_nos[picked_detector_index],
            self.picked_detector_ids[picked_detector_index],
            value,
        )
