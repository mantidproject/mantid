# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Detectors import DetectorInfo
import instrumentview.Projections.SphericalProjection as iv_spherical
import instrumentview.Projections.CylindricalProjection as iv_cylindrical

from mantid.dataobjects import Workspace2D
from mantid.simpleapi import CreateDetectorTable, ExtractSpectra, ConvertUnits, AnalysisDataService, SumSpectra, Rebin
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

    def __init__(self, workspace: Workspace2D):
        """For the given workspace, calculate detector positions, the map from detector indices to workspace indices, and integrated
        counts. Optionally will draw detector geometry, e.g. rectangular bank or tube instead of points."""
        self._workspace = workspace
        self._workspace_x_unit = workspace.getAxis(0).getUnit().unitID()

    def setup(self):
        component_info = self._workspace.componentInfo()
        self._sample_position = np.array(component_info.samplePosition()) if component_info.hasSample() else np.zeros(3)
        has_source = self._workspace.getInstrument().getSource() is not None
        self._source_position = np.array(component_info.sourcePosition()) if has_source else np.array([0, 0, 0])
        self._root_position = np.array(component_info.position(0))

        detector_info_table = CreateDetectorTable(self._workspace, IncludeDetectorPosition=True, StoreInADS=False)

        # Might have comma-separated multiple detectors, choose first one in the string in that case
        first_numbers = np.char.split(detector_info_table.column("Detector ID(s)"), sep=",")
        self._detector_ids = np.array([int(x[0]) for x in first_numbers])
        detector_positions = detector_info_table.column("Position")
        self._spherical_positions = np.array([pos.getSpherical() for pos in detector_positions])
        self._detector_positions = np.array(detector_positions)
        self._workspace_indices = np.array(detector_info_table.column("Index")).astype(int)
        self._is_monitor = np.array(detector_info_table.column("Monitor"))
        spectrum_number = np.array(detector_info_table.column("Spectrum No"))
        self._is_valid = (self._is_monitor == "no") & (spectrum_number != -1)
        self._monitor_positions = self._detector_positions[self._is_monitor == "yes"]

        # Initialise with zeros
        self._counts = np.zeros_like(self._detector_ids)
        self._counts_limits = (0, 0)
        self._detector_projection_positions = np.zeros_like(self._detector_positions)
        self._detector_is_picked = np.full(len(self._detector_ids[self._is_valid]), False)

        # Get min and max tof values
        if self._workspace.isRaggedWorkspace():
            first_last = np.array([self._workspace.readX(i)[[0, -1]] for i in range(self._workspace.getNumberHistograms())])
            self._tof_limits = (np.min(first_last[:, 0]), np.max(first_last[:, 1]))

        elif self._workspace.isCommonBins():
            self._tof_limits = tuple(self._workspace.dataX(0)[[0, -1]])

        else:
            data_x = self._workspace.extractX()
            self._tof_limits = (np.min(data_x[:, 0]), np.max(data_x[:, -1]))

        # Update counts with default total range
        self.update_time_of_flight_range(self._tof_limits, True)

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
    def default_projection(self) -> str:
        return self._workspace.getInstrument().getDefaultView()

    @property
    def sample_position(self) -> np.ndarray:
        return self._sample_position

    @property
    def detector_positions(self) -> np.ndarray:
        return self._detector_positions[self._is_valid]

    @property
    def detector_projection_positions(self) -> np.ndarray:
        return self._detector_projection_positions[self._is_valid]

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
    def tof_limits(self) -> tuple[float, float]:
        return self._tof_limits

    @tof_limits.setter
    def tof_limits(self, limits) -> None:
        try:
            min, max = limits
            assert float(max) > float(min)
        except (ValueError, AssertionError):
            return
        self._tof_limits = limits
        # Update the counts
        self.update_time_of_flight_range(self._tof_limits)

    def update_time_of_flight_range(self, tof_limits: tuple[float, float], entire_range: bool = False) -> None:
        tof_min, tof_max = tof_limits
        workspace_indices = self._workspace_indices[self._is_valid]
        new_detector_counts = np.array(
            self._workspace.getIntegratedCountsForWorkspaceIndices(
                workspace_indices, len(workspace_indices), float(tof_min), float(tof_max), entire_range
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

    def calculate_projection(self, is_spherical: bool, axis: list[int]):
        """Calculate the 2D projection with the specified axis. Can be either cylindrical or spherical."""
        projection = (
            iv_spherical.SphericalProjection(self._sample_position, self._root_position, self._detector_positions, np.array(axis))
            if is_spherical
            else iv_cylindrical.CylindricalProjection(self._sample_position, self._root_position, self._detector_positions, np.array(axis))
        )
        self._detector_projection_positions[:, :2] = projection.positions()  # Assign only x and y coordinate
        return self._detector_projection_positions

    def extract_spectra_for_line_plot(self, unit: str, sum_spectra: bool) -> None:
        workspace_indices = self.picked_workspace_indices
        if len(workspace_indices) == 0:
            self.line_plot_workspace = None
            return

        ws = ExtractSpectra(InputWorkspace=self._workspace, WorkspaceIndexList=workspace_indices, EnableLogging=False, StoreInADS=False)

        if sum_spectra and len(workspace_indices) > 1:
            # Sum in d-Spacing to avoid blurring peaks
            if self._workspace_x_unit != "dSpacing" and self.has_unit:
                ws = ConvertUnits(InputWorkspace=ws, target="dSpacing", EMode="Elastic", EnableLogging=False, StoreInADS=False)
            # Converting to d-Spacing will give spectra with different bin edges
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

        if self.has_unit:
            ws = ConvertUnits(InputWorkspace=ws, target=unit, EMode="Elastic", EnableLogging=False, StoreInADS=False)
        self.line_plot_workspace = ws

    def save_line_plot_workspace_to_ads(self) -> None:
        if self.line_plot_workspace is None or len(self.picked_workspace_indices) == 0:
            return
        name_exported_ws = f"instrument_view_selected_spectra_{self._workspace.name()}"
        AnalysisDataService.addOrReplace(name_exported_ws, self.line_plot_workspace)
