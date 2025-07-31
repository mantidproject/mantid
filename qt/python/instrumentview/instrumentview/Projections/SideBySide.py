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


class FlatBankInfo:
    """Information about a flat bank detector."""

    rotation: Rotation
    detector_ids: list[int]
    reference_position: np.ndarray
    detector_id_position_map: dict[int, np.ndarray] = {}
    dimensions: np.ndarray
    steps: list[int]
    pixels: list[int]
    relative_projected_positions: np.ndarray = np.array([])

    def translate(self, shift: np.ndarray):
        self.reference_position += shift
        for id in self.detector_id_position_map:
            self.detector_id_position_map[id] += shift

    def _transform_positions_so_origin_bottom_left(self) -> None:
        if not self.relative_projected_positions.any():
            return

        min_coordinates = np.min(self.relative_projected_positions, axis=0)
        self.relative_projected_positions -= min_coordinates

    def calculate_projected_positions(self) -> None:
        if self.relative_projected_positions.any():
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
                id = self.detector_ids[iy * y_pixels + ix]
                detector_position = np.add(origin, [x_offset, y_offset, 0])
                self.detector_id_position_map[id] = detector_position


class SideBySide(Projection):
    _detector_id_to_flat_bank_map: dict[int, FlatBankInfo] = {}
    _flat_banks: list[FlatBankInfo] = []
    _calculator: PanelsSurfaceCalculator
    _workspace: Workspace2D
    _component_index_detector_id_map: dict[int, int]

    def __init__(
        self,
        workspace: Workspace2D,
        detector_ids: list[int] | np.ndarray,
        sample_position: np.ndarray,
        root_position: np.ndarray,
        detector_positions: list[DetectorPosition] | np.ndarray,
        axis: np.ndarray,
    ):
        self._calculator = PanelsSurfaceCalculator()
        self._detector_ids = detector_ids
        self._workspace = workspace
        component_info = workspace.componentInfo()
        detector_component_indices = np.array(component_info.detectorsInSubtree(component_info.root()))
        self._component_index_detector_id_map = dict(zip(detector_component_indices, detector_ids))
        super().__init__(sample_position, root_position, detector_positions, axis)

    def _calculate_axes(self, root_position: np.ndarray) -> None:
        x = [0.0] * 3
        y = [0.0] * 3
        self._calculator.setupBasisAxes(x, y, list(self._projection_axis))
        self._x_axis = x
        self._y_axis = y

    def _construct_flat_panels(self, workspace: Workspace2D) -> None:
        self._detector_id_to_flat_bank_map.clear()
        grids = self._construct_rectangles_and_grids(workspace)
        tubes = self._construct_tube_banks(workspace.componentInfo())
        self._flat_banks = grids + tubes

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
        flat_banks = []

        if len(rectangular_banks) == 0:
            return flat_banks

        for bank in rectangular_banks:
            flat_bank = FlatBankInfo()
            flat_bank.detector_id_position_map.clear()
            flat_bank.reference_position = np.array(bank.getPos())
            rotation = bank.getRotation()
            flat_bank.rotation = Rotation.from_quat([rotation.real(), rotation.imagI(), rotation.imagJ(), rotation.imagK()])
            flat_bank.detector_ids = list(range(bank.minDetectorID(), bank.maxDetectorID() + 1))
            flat_bank.dimensions = np.abs([bank.xsize(), bank.ysize(), bank.zsize()])
            flat_bank.steps = np.abs([bank.xstep(), bank.ystep(), bank.zstep()])
            flat_bank.pixels = [bank.xpixels(), bank.ypixels(), bank.zpixels()]
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
            detectors = [self._component_index_detector_id_map[i] for i in detector_component_indices]
            flat_bank = FlatBankInfo()
            flat_bank.detector_id_position_map.clear()
            flat_bank.reference_position = np.array(component_info.position(int(detector_component_indices[0])))
            normal = np.array(self._calculator.calculateBankNormal(component_info, group))
            rotation = self._calculator.calcBankRotation(
                list(flat_bank.reference_position),
                list(normal),
                list(self._projection_axis),
                list(self._y_axis),
                list(self._sample_position),
            )
            flat_bank.rotation = Rotation.from_quat([rotation[0], rotation[1], rotation[2], rotation[3]])
            flat_bank.detector_ids = detectors
            flat_bank.relative_projected_positions = self._calculate_projected_positions(
                np.array([component_info.position(int(d)) for d in detector_component_indices]), normal, flat_bank
            )
            flat_bank.dimensions = np.max(flat_bank.relative_projected_positions, axis=0) - np.min(
                flat_bank.relative_projected_positions, axis=0
            )
            flat_banks.append(flat_bank)

        return flat_banks

    def _calculate_projected_positions(self, detector_positions: np.ndarray, normal: np.ndarray, flat_bank: FlatBankInfo) -> np.ndarray:
        positions = detector_positions - flat_bank.reference_position
        x_axis = np.array([1, 0, 0])
        if np.allclose(x_axis, normal):  # if v and normal are parallel
            x_axis = np.array([0, 1, 0])
        u_plane = x_axis + (x_axis.dot(normal)) * normal
        u_plane = u_plane / np.linalg.norm(u_plane)
        v_plane = np.cross(u_plane, normal)
        return np.array([[np.dot(p, u_plane), np.dot(p, v_plane), 0] for p in positions])

    def _arrange_panels(self) -> None:
        max_dims = np.max([b.dimensions for b in self._flat_banks], axis=0)
        number_banks = len(self._flat_banks)
        banks_per_row = np.ceil(np.sqrt(number_banks))
        position = np.zeros(3)
        banks_arranged = 0
        space_factor = 1.1
        for bank in self._flat_banks:
            bank.translate(position - bank.reference_position)
            banks_arranged += 1
            if banks_arranged % banks_per_row == 0:
                position[0] = 0
                position[1] += max_dims[1] * space_factor
            else:
                position[0] += bank.dimensions[0] * space_factor

    def _calculate_2d_coordinates(self) -> tuple[np.ndarray, np.ndarray]:
        self._construct_flat_panels(self._workspace)

        u_positions = []
        v_positions = []
        for id in self._detector_ids:
            if id not in self._detector_id_to_flat_bank_map:
                u_positions.append(0.0)
                v_positions.append(0.0)
                continue
            bank = self._detector_id_to_flat_bank_map[id]
            position = bank.detector_id_position_map[id]
            u_positions.append(position[0])
            v_positions.append(position[1])
        return (np.array(u_positions), np.array(v_positions))
