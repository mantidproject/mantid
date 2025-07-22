# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Projections.projection import projection
import numpy as np
from instrumentview.Detectors import DetectorPosition
from mantid.api import PanelsSurfaceCalculator
from mantid.dataobjects import Workspace2D
from scipy.spatial.transform import Rotation


class FlatBankInfo:
    """Information about a flat bank detector."""

    rotation: Rotation
    start_detector_id: int = 0
    end_detector_id: int = 0
    reference_position: list[float]
    detector_id_position_map: dict[int, list[float]] = {}
    dimensions: list[float]
    steps: list[int]
    pixels: list[int]

    def translate(self, shift: list[float]):
        self.reference_position += shift
        for id in self.detector_id_position_map:
            self.detector_id_position_map[id] += shift


class SideBySide(projection):
    _detector_id_to_flat_bank_map: dict[int, FlatBankInfo] = {}
    _flat_banks: list[FlatBankInfo] = []
    _calculator: PanelsSurfaceCalculator

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
        self._construct_flat_panels(workspace)
        super().__init__(sample_position, root_position, detector_positions, axis)

    def _calculate_axes(self, root_position: np.ndarray) -> None:
        x = [0.0] * 3
        y = [0.0] * 3
        self._calculator.setupBasisAxes(x, y, list(self._projection_axis))
        self._x_axis = x
        self._y_axis = y

    def _construct_flat_panels(self, workspace: Workspace2D) -> None:
        self._flat_banks.clear()
        self._detector_id_to_flat_bank_map.clear()
        instrument = workspace.getInstrument()
        rectangular_banks = instrument.findGridDetectors()

        if len(rectangular_banks) == 0:
            return

        for bank in rectangular_banks:
            flat_bank = FlatBankInfo()
            flat_bank.detector_id_position_map.clear()
            self._flat_banks.append(flat_bank)
            flat_bank.reference_position = np.array(bank.getPos())
            rotation = bank.getRotation()
            flat_bank.rotation = Rotation.from_quat([rotation.real(), rotation.imagI(), rotation.imagJ(), rotation.imagK()])
            flat_bank.start_detector_id = bank.minDetectorID()
            flat_bank.end_detector_id = bank.maxDetectorID()
            flat_bank.dimensions = np.abs([bank.xsize(), bank.ysize(), bank.zsize()])
            flat_bank.steps = np.abs([bank.xstep(), bank.ystep(), bank.zstep()])
            flat_bank.pixels = [bank.xpixels(), bank.ypixels(), bank.zpixels()]

        self._arrange_panels()

        for bank in self._flat_banks:
            y_pixels = bank.pixels[1]
            x_pixels = bank.pixels[0]
            origin = bank.reference_position
            detector_ids = list(range(bank.start_detector_id, bank.end_detector_id + 1))
            for iy in range(y_pixels):
                y_offset = iy * bank.steps[1]
                for ix in range(x_pixels):
                    x_offset = ix * bank.steps[0]
                    id = detector_ids[iy * y_pixels + ix]
                    detector_position = np.add(origin, [x_offset, y_offset, 0])
                    bank.detector_id_position_map[id] = detector_position

            for id in detector_ids:
                self._detector_id_to_flat_bank_map[id] = bank

    def _arrange_panels(self) -> None:
        # Going to assume all banks are roughly the same size to make the
        # arrangement simpler
        max_dims = np.max([b.dimensions for b in self._flat_banks], axis=0)
        number_banks = len(self._flat_banks)
        banks_per_row = np.ceil(np.sqrt(number_banks))
        position = [0, 0, 0]
        banks_arranged = 0
        space_factor = 1.1
        for bank in self._flat_banks:
            bank.reference_position = position.copy()
            banks_arranged += 1
            if banks_arranged % banks_per_row == 0:
                position[0] = 0
                position[1] += max_dims[1] * space_factor
            else:
                position[0] += bank.dimensions[0] * space_factor

    def _calculate_2d_coordinates(self) -> tuple[np.ndarray, np.ndarray]:
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
