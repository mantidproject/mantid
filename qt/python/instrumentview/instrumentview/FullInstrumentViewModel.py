# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Detectors import DetectorInfo
from instrumentview.Projections.SphericalProjection import SphericalProjection
from instrumentview.Projections.CylindricalProjection import CylindricalProjection
from instrumentview.Projections.SideBySide import SideBySide
from instrumentview.Peaks.Peak import Peak
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks

from mantid.dataobjects import Workspace2D, PeaksWorkspace
from mantid.simpleapi import CreateDetectorTable, ExtractSpectra, ConvertUnits, AnalysisDataService, SumSpectra, Rebin
from itertools import groupby
import numpy as np
from typing import ClassVar


class FullInstrumentViewModel:
    """Model for the Instrument View Window. Will calculate detector positions, indices, and integrated counts that give the colours"""

    _FULL_3D: ClassVar[str] = "3D"
    _SPHERICAL_X: ClassVar[str] = "Spherical X"
    _SPHERICAL_Y: ClassVar[str] = "Spherical Y"
    _SPHERICAL_Z: ClassVar[str] = "Spherical Z"
    _CYLINDRICAL_X: ClassVar[str] = "Cylindrical X"
    _CYLINDRICAL_Y: ClassVar[str] = "Cylindrical Y"
    _CYLINDRICAL_Z: ClassVar[str] = "Cylindrical Z"
    _SIDE_BY_SIDE: ClassVar[str] = "Side by Side"
    _PROJECTION_OPTIONS: ClassVar[list[str]] = [
        _FULL_3D,
        _SPHERICAL_X,
        _SPHERICAL_Y,
        _SPHERICAL_Z,
        _CYLINDRICAL_X,
        _CYLINDRICAL_Y,
        _CYLINDRICAL_Z,
        _SIDE_BY_SIDE,
    ]

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
        x_unit = workspace.getAxis(0).getUnit()
        self._workspace_x_unit = x_unit.unitID()
        self._workspace_x_unit_display = f"{str(x_unit.caption())} ({str(x_unit.symbol())})"
        self._selected_peaks_workspaces = []

    def setup(self):
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
        self._detector_positions = detector_info_table.columnArray("Position")
        self._workspace_indices = detector_info_table.columnArray("Index")
        # Array of strings 'yes', 'no' and 'n/a'
        self._is_monitor = detector_info_table.columnArray("Monitor")
        self._is_valid = self._is_monitor == "no"
        self._monitor_positions = self._detector_positions[self._is_monitor == "yes"]
        self._current_projected_positions = self.detector_positions

        # Initialise with zeros
        self._counts = np.zeros_like(self._detector_ids)
        self._counts_limits = (0, 0)
        self._detector_is_picked = np.full(len(self._detector_ids[self._is_valid]), False)

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
    def default_projection(self) -> str:
        return self._workspace.getInstrument().getDefaultView()

    @property
    def sample_position(self) -> np.ndarray:
        return self._sample_position

    @property
    def detector_positions(self) -> np.ndarray:
        return self._detector_positions[self._is_valid]

    @property
    def detector_ids(self) -> np.ndarray:
        return self._detector_ids[self._is_valid]

    @property
    def monitor_positions(self) -> np.ndarray:
        return self._monitor_positions

    @property
    def picked_visibility(self) -> np.ndarray:
        return self._detector_is_picked.astype(int)

    @property
    def picked_detector_ids(self) -> np.ndarray:
        return self._detector_ids[self._is_valid][self._detector_is_picked]

    @property
    def picked_workspace_indices(self) -> np.ndarray:
        return self._workspace_indices[self._is_valid][self._detector_is_picked]

    @property
    def detector_counts(self) -> np.ndarray:
        return self._counts[self._is_valid]

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
    def current_projected_positions(self) -> np.ndarray:
        return self._current_projected_positions

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
        workspace_indices = self._workspace_indices[self._is_valid]
        new_detector_counts = np.array(
            self._workspace.getIntegratedCountsForWorkspaceIndices(
                workspace_indices, len(workspace_indices), float(integration_min), float(integration_max), entire_range
            ),
            dtype=int,
        )
        self._counts_limits = (np.min(new_detector_counts), np.max(new_detector_counts))
        self._counts[self._is_valid] = new_detector_counts

    def negate_picked_visibility(self, indices: list[int] | np.ndarray) -> None:
        self._detector_is_picked[indices] = ~self._detector_is_picked[indices]

    def clear_all_picked_detectors(self) -> None:
        self._detector_is_picked.fill(False)

    def picked_detectors_info_text(self) -> list[DetectorInfo]:
        """For the specified detector, extract info that can be displayed in the View, and wrap it all up in a DetectorInfo class"""

        picked_ws_indices = self._workspace_indices[self._is_valid][self._detector_is_picked]
        picked_ids = self._detector_ids[self._is_valid][self._detector_is_picked]
        picked_xyz_positions = self._detector_positions[self._is_valid][self._detector_is_picked]
        picked_spherical_positions = self._spherical_positions[self._is_valid][self._detector_is_picked]
        picked_counts = self._counts[self._is_valid][self._detector_is_picked]

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

    def reset_cached_projection_positions(self) -> None:
        self._current_projected_positions = self.detector_positions

    def calculate_projection(self, projection_option: str, axis: list[int], positions: np.ndarray):
        """Calculate the 2D projection with the specified axis. Can be cylindrical, spherical, or side-by-side."""
        if projection_option == self._SIDE_BY_SIDE:
            projection = SideBySide(
                self._workspace, self.detector_ids, self.sample_position, self._root_position, positions, np.array(axis)
            )
        elif projection_option.startswith("Spherical"):
            projection = SphericalProjection(self.sample_position, self._root_position, positions, np.array(axis))
        elif projection_option.startswith("Cylindrical"):
            projection = CylindricalProjection(self.sample_position, self._root_position, positions, np.array(axis))
        else:
            raise ValueError(f"Unknown projection type: {projection_option}")

        projected_positions = np.zeros_like(positions)
        projected_positions[:, :2] = projection.positions()  # Assign only x and y coordinate
        self._current_projected_positions = projected_positions
        return self._current_projected_positions

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

    def peaks_workspaces_in_ads(self) -> list[PeaksWorkspace]:
        ads = AnalysisDataService.Instance()
        workspaces_in_ads = ads.retrieveWorkspaces(ads.getObjectNames())
        return [
            pws
            for pws in workspaces_in_ads
            if "PeaksWorkspace" in str(type(pws)) and pws.getInstrument().getFullName() == self._workspace.getInstrument().getFullName()
        ]

    def set_peaks_workspaces(self, peaks_workspace_names: list[str]) -> None:
        self._selected_peaks_workspaces = AnalysisDataService.Instance().retrieveWorkspaces(peaks_workspace_names)

    def peak_overlay_points(self) -> list[list[DetectorPeaks]]:
        detector_info = self._workspace.detectorInfo()
        peaks_grouped_by_ws = []
        for pws in self._selected_peaks_workspaces:
            peaks = []
            peaks_dict = pws.toDict()
            detector_ids = peaks_dict["DetID"]
            hkls = zip(peaks_dict["h"], peaks_dict["k"], peaks_dict["l"])
            positions = [np.array(detector_info.position(detector_info.indexOf(id))) for id in detector_ids]
            tofs = peaks_dict["TOF"]
            dspacings = peaks_dict["DSpacing"]
            wavelengths = peaks_dict["Wavelength"]
            peaks += [
                Peak(det_id, v, hkl, tof, dspacing, wavelength, 2 * np.pi / dspacing)
                for (det_id, v, hkl, tof, dspacing, wavelength) in zip(detector_ids, positions, hkls, tofs, dspacings, wavelengths)
            ]
            # Combine peaks on the same detector
            detector_peaks = []
            # groupby groups consecutive matches, so must be sorted
            peaks.sort(key=lambda x: x.detector_id)
            for det_id, peaks_for_id in groupby(peaks, lambda x: x.detector_id):
                if det_id in self.detector_ids:
                    detector_peaks.append(DetectorPeaks(list(peaks_for_id)))
            peaks_grouped_by_ws.append(detector_peaks)
        return peaks_grouped_by_ws
