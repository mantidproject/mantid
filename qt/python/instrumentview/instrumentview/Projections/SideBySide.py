# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.Projection import Projection
import numpy as np
from instrumentview.Detectors import DetectorPosition
from mantid.api import PanelsSurfaceCalculator
from mantid.dataobjects import Workspace2D
from mantid.geometry import ComponentInfo
from scipy.spatial.transform import Rotation
from dataclasses import dataclass, field
from typing import List, Dict, Optional


@dataclass
class FlatBankInfo:
    """Information about a flat bank detector."""

    rotation: Optional[Rotation] = None
    detector_ids: List[int] = field(default_factory=list)
    reference_position: np.ndarray = field(default_factory=lambda: np.zeros(3))
    detector_id_position_map: Dict[int, np.ndarray] = field(default_factory=dict)
    dimensions: np.ndarray = field(default_factory=lambda: np.zeros(3))
    steps: List[float] = field(default_factory=list)
    pixels: List[int] = field(default_factory=list)
    relative_projected_positions: np.ndarray = field(default_factory=lambda: np.empty((0, 3)))
    has_position_in_idf: bool = False
    bank_type: str = "other"

    def translate(self, shift: np.ndarray):
        self.reference_position += shift
        for id in self.detector_id_position_map:
            self.detector_id_position_map[id] += shift

    def _transform_positions_so_origin_bottom_left(self) -> None:
        if len(self.relative_projected_positions) == 0:
            return

        min_coordinates = np.min(self.relative_projected_positions, axis=0)
        self.relative_projected_positions -= min_coordinates

    def calculate_projected_positions(self) -> None:
        if len(self.relative_projected_positions) > 0:
            self._transform_positions_so_origin_bottom_left()
            for index in range(len(self.detector_ids)):
                self.detector_id_position_map[self.detector_ids[index]] = self.relative_projected_positions[index] + self.reference_position
            return

        y_pixels = self.pixels[1]
        x_pixels = self.pixels[0]
        origin = self.reference_position
        for iy in range(y_pixels):
            y_offset = iy * self.steps[1]
            for ix in range(x_pixels):
                x_offset = ix * self.steps[0]
                index = iy * x_pixels + ix
                # Masked workspaces can have fewer detectors
                # than the number of pixels specified in the
                # bank
                if index >= len(self.detector_ids):
                    return
                id = self.detector_ids[iy * x_pixels + ix]
                detector_position = np.add(origin, [x_offset, y_offset, 0])
                self.detector_id_position_map[id] = detector_position


class SideBySide(Projection):
    def __init__(
        self,
        workspace: Workspace2D,
        detector_ids: np.ndarray,
        sample_position: np.ndarray,
        root_position: np.ndarray,
        detector_positions: list[DetectorPosition] | np.ndarray,
        axis: np.ndarray,
    ):
        self._calculator = PanelsSurfaceCalculator()
        self._detector_id_to_flat_bank_map: dict[int, FlatBankInfo] = {}
        self._flat_banks: list[FlatBankInfo] = []
        self._detector_ids = detector_ids
        self._workspace = workspace
        all_detector_ids = np.array(workspace.detectorInfo().detectorIDs())
        # We can have a subset of all detector IDs, so we need to know the index of the given detectors
        # in the list of all the detectors, since this will give us the component index, which we
        # will need later. If there are N detectors, then 0,..., N-1 are their component indices
        detector_component_indices = np.where(np.isin(all_detector_ids, detector_ids))[0]
        self._component_index_detector_id_map = dict(zip(detector_component_indices, detector_ids))
        self._detector_id_component_index_map = {id: c for c, id in self._component_index_detector_id_map.items()}
        super().__init__(sample_position, root_position, detector_positions, axis)

    def _find_and_correct_x_gap(self) -> None:
        # We don't want any gaps corrected
        return

    def _calculate_axes(self, root_position: np.ndarray) -> None:
        x = [0.0] * 3
        y = [0.0] * 3
        self._calculator.setupBasisAxes(x, y, self._projection_axis)
        self._x_axis = x
        self._y_axis = y

    def _construct_flat_panels(self, workspace: Workspace2D) -> None:
        grids = self._construct_rectangles_and_grids(workspace)
        tubes = self._construct_tube_banks(workspace.componentInfo())
        self._flat_banks = grids + tubes
        detectors_in_banks = (
            np.empty(0, dtype=int)
            if len(self._flat_banks) == 0
            else np.concatenate([np.array(d.detector_ids, dtype=int) for d in self._flat_banks])
        )
        remaining_detectors_bank = self._create_flat_bank_with_missing_detectors(detectors_in_banks)
        if remaining_detectors_bank is not None:
            self._flat_banks.append(remaining_detectors_bank)

        if len(self._flat_banks) == 0:
            return

        self._arrange_panels()

        for bank in self._flat_banks:
            bank.calculate_projected_positions()
            for id in bank.detector_ids:
                self._detector_id_to_flat_bank_map[id] = bank

    def _construct_rectangles_and_grids(self, workspace: Workspace2D) -> list[FlatBankInfo]:
        instrument = workspace.getInstrument()
        rectangular_banks = instrument.findGridDetectors()
        component_info = self._workspace.componentInfo()
        flat_banks = []

        if len(rectangular_banks) == 0:
            return flat_banks

        for bank in rectangular_banks:
            bank_detector_ids = np.array(range(bank.minDetectorID(), bank.maxDetectorID() + 1))
            valid_detector_ids = self._detector_ids[np.isin(self._detector_ids, bank_detector_ids)]
            if len(valid_detector_ids) == 0:
                continue
            flat_bank = FlatBankInfo()
            flat_bank.bank_type = "grid"
            flat_bank.detector_id_position_map.clear()
            flat_bank.reference_position = np.array(bank.getPos())
            rotation = bank.getRotation()
            flat_bank.rotation = Rotation.from_quat([rotation.imagI(), rotation.imagJ(), rotation.imagK(), rotation.real()])
            flat_bank.detector_ids = list(valid_detector_ids)
            flat_bank.dimensions = np.abs([bank.xsize(), bank.ysize(), bank.zsize()])
            flat_bank.steps = np.abs([bank.xstep(), bank.ystep(), bank.zstep()])
            flat_bank.pixels = [bank.xpixels(), bank.ypixels(), bank.zpixels()]
            parent_component_index = component_info.parent(int(self._detector_id_component_index_map[flat_bank.detector_ids[0]]))
            override_pos = self._calculator.getSideBySideViewPos(component_info, instrument, parent_component_index)
            flat_bank.has_position_in_idf = override_pos[0]
            if flat_bank.has_position_in_idf:
                flat_bank.reference_position = np.array(override_pos[1] + [0])
            flat_banks.append(flat_bank)

        return flat_banks

    def _construct_tube_banks(self, component_info: ComponentInfo) -> list[FlatBankInfo]:
        tubeGroupParents = self._calculator.getAllTubeDetectorFlatGroupParents(component_info)
        flat_banks = []
        for group in tubeGroupParents:
            number_of_tube_groups = len(group)
            if number_of_tube_groups == 0:
                continue
            detector_component_indices = np.concatenate([component_info.children(p) for p in group])
            detector_component_indices.sort()
            detectors = [
                self._component_index_detector_id_map[i] for i in detector_component_indices if i in self._component_index_detector_id_map
            ]
            flat_bank = FlatBankInfo()
            flat_bank.bank_type = "tube"
            flat_bank.detector_id_position_map.clear()
            flat_bank.reference_position = np.array(component_info.position(int(detector_component_indices[0])))
            normal = np.array(self._calculator.calculateBankNormal(component_info, group))
            rotation = self._calculator.calcBankRotation(
                flat_bank.reference_position,
                normal,
                self._projection_axis,
                self._y_axis,
                self._sample_position,
            )
            # SciPy from_quat expects (x, y, z, w)
            flat_bank.rotation = Rotation.from_quat([rotation[1], rotation[2], rotation[3], rotation[0]])
            flat_bank.detector_ids = detectors
            flat_bank.relative_projected_positions = self._calculate_projected_positions(
                np.array([component_info.position(int(d)) for d in detector_component_indices]), normal, flat_bank
            )
            flat_bank.dimensions = np.max(flat_bank.relative_projected_positions, axis=0) - np.min(
                flat_bank.relative_projected_positions, axis=0
            )
            override_pos = self._calculator.getSideBySideViewPos(component_info, self._workspace.getInstrument(), group[0])
            flat_bank.has_position_in_idf = override_pos[0]
            if flat_bank.has_position_in_idf:
                flat_bank.reference_position = np.array(override_pos[1] + [0])
            flat_banks.append(flat_bank)

        return flat_banks

    def _calculate_projected_positions(self, detector_positions: np.ndarray, normal: np.ndarray, flat_bank: FlatBankInfo) -> np.ndarray:
        positions = detector_positions - flat_bank.reference_position
        x_axis = np.array([1, 0, 0])
        if np.allclose(x_axis, normal):  # if v and normal are parallel
            x_axis = np.array([0, 1, 0])
        u_plane = x_axis - (x_axis.dot(normal)) * normal
        u_plane = u_plane / np.linalg.norm(u_plane)
        v_plane = np.cross(u_plane, normal)
        return np.array([[np.dot(p, u_plane), np.dot(p, v_plane), 0] for p in positions])

    def _create_flat_bank_with_missing_detectors(self, detectors_already_in_banks: np.ndarray) -> Optional[FlatBankInfo]:
        detectors = np.array(self._detector_ids)
        missing_detectors = np.setdiff1d(detectors, detectors_already_in_banks)
        number_of_detectors = len(missing_detectors)
        if number_of_detectors == 0:
            return None
        flat_bank = FlatBankInfo()
        flat_bank.detector_id_position_map.clear()
        flat_bank.reference_position = np.zeros(3)
        flat_bank.rotation = Rotation.identity()
        flat_bank.detector_ids = list(missing_detectors)
        detectors_on_x_axis = np.ceil(np.sqrt(number_of_detectors)).astype(int)
        # Base separation roughly on any existing detector densities
        separation = 0.01
        if len(self._flat_banks) > 0 and detectors_on_x_axis > 1:
            existing_max_dims = np.max([b.dimensions for b in self._flat_banks], axis=0)
            area_per_detector = (existing_max_dims[0] * existing_max_dims[1]) / sum([len(f.detector_ids) for f in self._flat_banks])
            if area_per_detector > 0:
                separation = max([np.sqrt(area_per_detector) / number_of_detectors / (detectors_on_x_axis - 1), 0.001])

        # Arrange detectors into a grid with sqrt(len) points on the x axis, the last
        # row may have less than that number
        points_in_square_grid = ((np.floor(number_of_detectors / detectors_on_x_axis)) * detectors_on_x_axis).astype(int)
        square_grid = missing_detectors[0:points_in_square_grid].reshape((detectors_on_x_axis, -1))
        # Get position for each detector in square grid
        row_indices, column_indices = np.indices(square_grid.shape)
        square_grid_positions = np.stack((row_indices, column_indices, np.zeros_like(row_indices)), axis=2) * separation
        flat_bank.relative_projected_positions = square_grid_positions.reshape((-1, 3))

        # Do the same for the remainder. Unless we're lucky, we'll have a partial row of
        # detectors left over that we need to arrange
        if points_in_square_grid < number_of_detectors:
            remainder = missing_detectors[points_in_square_grid:]
            row_y = square_grid.shape[1] * separation
            remainder_positions = [[x * separation, row_y, 0] for x in range(len(remainder))]
            flat_bank.relative_projected_positions = np.append(flat_bank.relative_projected_positions, remainder_positions, axis=0)

        flat_bank.dimensions = np.max(flat_bank.relative_projected_positions, axis=0) - np.min(
            flat_bank.relative_projected_positions, axis=0
        )
        return flat_bank

    def _arrange_panels(self) -> None:
        max_dims = np.max([b.dimensions for b in self._flat_banks], axis=0)
        number_banks = len(self._flat_banks)
        banks_per_row = int(np.ceil(np.sqrt(number_banks)))
        position = np.zeros(3)
        banks_arranged = 0
        space_factor = 1.2
        for bank in self._flat_banks:
            if not bank.has_position_in_idf:
                bank.translate(position - bank.reference_position)
            banks_arranged += 1
            if banks_arranged % banks_per_row == 0:
                position[0] = 0
                position[1] += max_dims[1] * space_factor
            else:
                position[0] += bank.dimensions[0] * space_factor

    def get_bank_groups_by_detector_id(self) -> list[tuple[list[int], str]]:
        """Return detector IDs grouped by bank, with bank type.

        Returns
        -------
        list[tuple[list[int], str]]
            Each element is ``(detector_ids, bank_type)`` where *bank_type*
            is ``"grid"``, ``"tube"``, or ``"other"``.
        """
        return [(list(bank.detector_ids), bank.bank_type) for bank in self._flat_banks]

    def _calculate_2d_coordinates(self) -> tuple[np.ndarray, np.ndarray]:
        self._construct_flat_panels(self._workspace)

        u_positions = []
        v_positions = []
        for id in self._detector_ids:
            if id not in self._detector_id_to_flat_bank_map:
                raise RuntimeError(f"Detector with ID {id} not found in projection")
            bank = self._detector_id_to_flat_bank_map[id]
            position = bank.detector_id_position_map[id]
            u_positions.append(position[0])
            v_positions.append(position[1])
        return (np.array(u_positions), np.array(v_positions))
