# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Detectors import DetectorInfo, Detector, DetectorPosition
import instrumentview.Projections.spherical_projection as iv_spherical
import instrumentview.Projections.cylindrical_projection as iv_cylindrical
import numpy as np
import math


class FullInstrumentViewModel:
    """Model for the Instrument View Window. Will calculate detector positions, indices, and integrated counts that give the colours"""

    _sample_position = np.array([0, 0, 0])
    _source_position = np.array([0, 0, 0])
    _invalid_index = -1
    _data_min = 0.0
    _data_max = 0.0

    def __init__(self, workspace):
        """For the given workspace, calculate detector positions, the map from detector indices to workspace indices, and integrated
        counts. Optionally will draw detector geometry, e.g. rectangular bank or tube instead of points."""
        self._workspace = workspace
        self._detector_info = workspace.detectorInfo()

        self._component_info = workspace.componentInfo()
        self._sample_position = np.array(self._component_info.samplePosition())

        has_source = workspace.getInstrument().getSource() is not None
        self._source_position = np.array(self._component_info.sourcePosition()) if has_source else np.array([0, 0, 0])

        self._detectors = [Detector(index, int(id), self._detector_info) for index, id in enumerate(self._detector_info.detectorIDs())]
        self._detector_ids = self.detector_ids()
        self._detector_is_picked = np.full(len(self._detector_ids), False)
        workspace_indices = [int(index) for index in self._workspace.getIndicesFromDetectorIDs(self._detector_ids)]
        self._detector_id_to_workspace_index = dict(zip(self._detector_ids, workspace_indices))

        self._bin_min = math.inf
        self._bin_max = -math.inf
        for workspace_index in range(workspace.getNumberHistograms()):
            x_data = self._workspace.dataX(workspace_index)
            self._union_with_current_bin_min_max(x_data[0])
            self._union_with_current_bin_min_max(x_data[-1])

        self.update_time_of_flight_range(self._bin_min, self._bin_max, True)

    def negate_picked_visibility(self, indices: list[int]) -> None:
        for i in indices:
            self._detector_is_picked[i] = not self._detector_is_picked[i]

    def _union_with_current_bin_min_max(self, bin_edge) -> None:
        """Expand current bin limits to include new bin edge"""
        if not math.isinf(bin_edge):
            if bin_edge < self._bin_min:
                self._bin_min = bin_edge
            elif bin_edge > self._bin_max:
                self._bin_max = bin_edge

    def update_time_of_flight_range(self, tof_min: float, tof_max: float, entire_range=False) -> None:
        workspace_indices = list(self._detector_id_to_workspace_index.values())
        new_detector_counts = np.array(
            self._workspace.getIntegratedCountsForWorkspaceIndices(
                workspace_indices, len(workspace_indices), float(tof_min), float(tof_max), entire_range
            ),
            dtype=int,
        )
        self._detector_counts = new_detector_counts
        self._data_max = max(self._detector_counts)
        self._data_min = min(self._detector_counts)

    def workspace(self):
        return self._workspace

    def workspace_index_from_detector_id(self, detector_id: int) -> int:
        return int(self._detector_id_to_workspace_index[detector_id])

    def workspace_index_from_detector_index(self, detector_index: int) -> int:
        return self.workspace_index_from_detector_id(self._detectors[detector_index].id)

    def sample_position(self) -> np.ndarray:
        return self._sample_position

    def detector_positions(self) -> list[DetectorPosition]:
        return [d.position for d in self._detectors if not d.is_monitor]

    def detector_projection_positions(self) -> list[DetectorPosition]:
        return self._detector_projection_positions

    def detector_visibility(self) -> np.ndarray:
        return np.array(self._detector_is_picked).astype(int)

    def picked_detector_ids(self) -> np.ndarray:
        return np.array(self._detector_ids)[self._detector_is_picked]

    def detector_counts(self) -> np.ndarray:
        return self._detector_counts

    def detector_ids(self) -> list[int]:
        return [d.id for d in self._detectors if not d.is_monitor]

    def data_limits(self) -> list:
        return [self._data_min, self._data_max]

    def bin_limits(self) -> list:
        return [self._bin_min, self._bin_max]

    def monitor_positions(self) -> list:
        return [d.position for d in self._detectors if d.is_monitor]

    def get_detector_info_text(self, detector_id: int) -> DetectorInfo:
        """For the specified detector, extract info that can be displayed in the View, and wrap it all up in a DetectorInfo class"""
        workspace_index = self.workspace_index_from_detector_id(detector_id)
        ws_detector = self._workspace.getDetector(workspace_index)
        name = ws_detector.getName()
        component_path = ws_detector.getFullName()
        detector = [d for d in self._detectors if d.id == detector_id][0]
        xyz_position = detector.position
        spherical_position = detector.spherical_position
        pixel_counts = self.detector_counts()[self.workspace_index_from_detector_id(detector_id)]

        return DetectorInfo(
            name, detector_id, workspace_index, np.array(xyz_position), np.array(spherical_position), component_path, int(pixel_counts)
        )

    def calculate_projection(self, is_spherical: bool, axis: np.ndarray) -> list[DetectorPosition]:
        """Calculate the 2D projection with the specified axis. Can be either cylindrical or spherical."""
        sample_position = np.array(self._component_info.samplePosition())
        root_position = np.array(self._component_info.position(0))
        projection = (
            iv_spherical.spherical_projection(sample_position, root_position, self.detector_positions(), axis)
            if is_spherical
            else iv_cylindrical.cylindrical_projection(sample_position, root_position, self.detector_positions(), axis)
        )
        self._detector_projection_positions = [DetectorPosition([x, y, 0]) for (x, y) in projection.positions()]
        return self._detector_projection_positions
