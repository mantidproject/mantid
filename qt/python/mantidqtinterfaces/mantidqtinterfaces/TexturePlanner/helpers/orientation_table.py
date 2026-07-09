# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections.abc import KeysView, ValuesView, ItemsView

import numpy as np
from dataclasses import dataclass, field
from typing import List, Tuple, Sequence
from scipy.spatial.transform import Rotation
from mantid.kernel import logger
from Engineering.texture.texture_helper import vec_string_to_norm_array
import re


MAX_GONIOMETERS = 6
_DEFAULT_GONIO_STRING = "0,1.0,0.0,0.0,-1"  # angle=0, vec=(1,0,0), sense=-1


@dataclass
class Orientation:
    """One row in the orientation table: goniometer settings, the resulting
    rotation(s), inclusion/selection flags, and any cached projection / MC results."""

    gonio_strings: List[str] = field(default_factory=lambda: [_DEFAULT_GONIO_STRING] * MAX_GONIOMETERS)
    gRs: List[Rotation] = field(default_factory=lambda: [Rotation.identity()])
    R: Rotation = field(default_factory=Rotation.identity)
    include: bool = True
    select: bool = True
    pf_points: np.ndarray | None = None
    transmission: np.ndarray | None = None

    def copy(self) -> "Orientation":
        return Orientation(
            gonio_strings=list(self.gonio_strings),
            gRs=list(self.gRs),
            R=self.R,
            include=self.include,
            select=self.select,
            pf_points=None if self.pf_points is None else self.pf_points.copy(),
            transmission=None if self.transmission is None else self.transmission.copy(),
        )


class OrientationTable:
    """The list of saved orientations and operations on them (add / select /
    include / delete / load-from-file / goniometer-string IO).

    Holds the table-level state (saved_orientations, orientation_index,
    n_gonio) and the Euler goniometer convention (orientation_kwargs) used when
    loading orientation files.
    """

    # Returned by load_orientation_file when the file is not in Euler-angles format
    # (matrix format, or an error path). Falls back to a 3-axis (YXY) goniometer view.
    _DEFAULT_NUM_AXES = 3

    def __init__(self):
        # Euler goniometer convention used when loading orientation files (Axes scheme + Senses).
        self.orientation_kwargs = {"Axes": "YXY", "Senses": "-1,-1,-1"}
        self.sense_vals = {"Clockwise": -1, "Counterclockwise": 1}  # mantid convention
        self.sense_names = {"-1": "Clockwise", "1": "Counterclockwise"}
        self.axis_dict = {"x": (1, 0, 0), "y": (0, 1, 0), "z": (0, 0, 1)}
        self.n_gonio = 2
        self.orientation_index = 0
        self.saved_orientations = {0: self._make_default_orientation()}

    # dict-like access ---------------------------------------------------
    def __getitem__(self, index: int) -> Orientation:
        return self.saved_orientations[index]

    def keys(self) -> KeysView:
        return self.saved_orientations.keys()

    def values(self) -> ValuesView:
        return self.saved_orientations.values()

    def items(self) -> ItemsView:
        return self.saved_orientations.items()

    # index / count ------------------------------------------------------
    def get_orientation_index(self) -> int:
        return self.orientation_index

    def set_orientation_index(self, index: int) -> None:
        self.orientation_index = index

    def get_num_orientations(self) -> int:
        return len(self.saved_orientations.keys())

    def get_table_info(self) -> List[List[str | bool]]:
        return [[list(v.gonio_strings), v.include, v.select] for v in self.saved_orientations.values()]

    def set_n_gonio(self, val: int) -> None:
        self.n_gonio = val

    # setting values from elsewhere ---------------------------------------

    def set_transmission_at_index(self, transmission: np.ndarray, index: int) -> None:
        self.saved_orientations[index].transmission = transmission

    # rotation maths -----------------------------------------------------
    def calc_gRs(self, vecs: List[Tuple[float, float, float]], senses: List[float], angles: List[float]) -> Tuple[List[Rotation], Rotation]:
        self._validate_gR_data(vecs, senses, angles)  # throws an error if these lists have become different lengths
        gRs = [Rotation.identity()]
        R = Rotation.identity()
        for i, vec in enumerate(vecs):
            sense = senses[i]
            r_step = Rotation.from_davenport(vec, "extrinsic", sense * angles[i], degrees=True)
            R = R * r_step
            gRs.append(R)
        return gRs, R

    def update_gRs(
        self,
        vecs: List[Tuple[float, float, float]],
        senses: List[float],
        angles: List[float],
        current_index: int,
    ) -> None:
        gRs, R = self.calc_gRs(vecs, senses, angles)
        orientation = self.saved_orientations[current_index]
        orientation.gRs = gRs
        orientation.R = R
        orientation.pf_points = None
        orientation.transmission = None

    # selection / inclusion / delete ------------------------------------
    def update_selected(self, selected_inds: Sequence[int]) -> None:
        for k, v in self.saved_orientations.items():
            v.select = k in selected_inds

    def update_included(self, included_inds: Sequence[int]) -> None:
        for k, v in self.saved_orientations.items():
            v.include = k in included_inds

    def select_all(self) -> None:
        for v in self.saved_orientations.values():
            v.select = True

    def deselect_all(self) -> None:
        for v in self.saved_orientations.values():
            v.select = False

    def delete_selected(self) -> None:
        to_keep = [k for k, v in self.saved_orientations.items() if not v.select]
        if len(to_keep) == 0:
            self.saved_orientations = {0: self._make_default_orientation()}
        else:
            self.saved_orientations = {i: self.saved_orientations[k] for i, k in enumerate(to_keep)}
        new_orientation_index = to_keep.index(self.orientation_index) if self.orientation_index in to_keep else 0
        self.set_orientation_index(new_orientation_index)

    def add_orientation(self) -> None:
        # create a new orientation, initially just as a copy of the current orientation
        self.saved_orientations[self.get_num_orientations()] = self.saved_orientations[self.orientation_index].copy()

    # defaults / goniometer string IO -----------------------------------
    def _make_default_orientation(self) -> Orientation:
        vecs = [(1, 0, 0)] * MAX_GONIOMETERS
        senses = [-1] * MAX_GONIOMETERS
        angles = [0.0] * MAX_GONIOMETERS
        gRs, R = self.calc_gRs(vecs[: self.n_gonio], senses[: self.n_gonio], angles[: self.n_gonio])
        gonio_strings = [self.get_goniometer_string(vecs[i], senses[i], angles[i]) for i in range(MAX_GONIOMETERS)]
        return Orientation(gonio_strings=gonio_strings, gRs=gRs, R=R)

    @staticmethod
    def get_goniometer_string(vec: Tuple[float, float, float], sense: float, angle: float) -> str:
        sense = int(float(sense))
        if sense not in (-1, 1):
            raise ValueError(f"Invalid goniometer sense: {sense}")
        return f"{angle},{np.round(vec[0], 3)},{np.round(vec[1], 3)},{np.round(vec[2], 3)},{sense}"

    def update_gonio_string(self, vecs: List[Tuple[float, float, float]], senses: List[float], angles: List[float], index: int) -> None:
        self._validate_gR_data(vecs, senses, angles)  # throws an error if these lists have become different lengths
        orientation = self.saved_orientations[index]
        for i, vec in enumerate(vecs):
            orientation.gonio_strings[i] = self.get_goniometer_string(vec, senses[i], angles[i])
        for i in range(len(vecs), MAX_GONIOMETERS):
            orientation.gonio_strings[i] = self.get_goniometer_string((1, 0, 0), 1, 0)

    def read_goniometer_string(self, goniometer_string: str) -> Tuple[str, float, float]:
        angle, v1, v2, v3, sense = goniometer_string.split(",")
        sense_key = str(int(float(sense)))
        return f"{v1},{v2},{v3}", self.sense_names[sense_key], float(angle)

    def get_goniometer_values(self, index: int) -> Tuple[List[str], List[float], List[float]]:
        info = self.saved_orientations[index]
        vecs, senses, angles = [], [], []
        for gonio_string in info.gonio_strings:
            vec, sense, angle = self.read_goniometer_string(gonio_string)
            vecs.append(vec)
            senses.append(sense)
            angles.append(angle)
        return vecs, senses, angles

    # file loaders -------------------------------------------------------
    def load_orientation_file(self, txt_file: str) -> int:
        """Load orientations from a whitespace/comma-separated text file."""
        logger.notice("Loading Orientations from file")
        with open(txt_file, "r") as f:
            goniometer_strings = [line.strip().replace("\t", ",") for line in f]
            goniometer_lists = [[float(x) for x in re.split(r"[\s,]+", gs) if x] for gs in goniometer_strings]
        if len(goniometer_lists) == 0:
            logger.warning("No orientations found in file provided")
            return self._DEFAULT_NUM_AXES
        num_entries = len(goniometer_lists[0])
        is_matrix_format = num_entries > 6
        if is_matrix_format:
            self._load_matrix_orientations(goniometer_lists)
            return self._DEFAULT_NUM_AXES
        return self._load_euler_orientations(goniometer_lists, num_entries)

    def _load_matrix_orientations(self, goniometer_lists: List[List[float]]) -> None:
        vecs = [(0, 1, 0), (1, 0, 0), (0, 1, 0)]
        senses = [1, 1, 1]
        for goniometer_list in goniometer_lists:
            R_mat = np.asarray(goniometer_list[:9]).reshape((3, 3))
            R = Rotation.from_matrix(R_mat)
            # the YXY decomposition is only used to populate the displayed goniometer fields
            euler_angles = R.as_euler("YXY", degrees=True)
            display_angles = np.round(euler_angles, 2)
            self.add_orientation()
            new_index = self.get_num_orientations() - 1
            self.update_gonio_string(vecs, senses, display_angles, new_index)
            self.update_gRs(vecs, senses, euler_angles, new_index)
            # preserve the exact input matrix so projection / export stay faithful to the file
            self.saved_orientations[new_index].R = R

    def _load_euler_orientations(self, goniometer_lists: List[List[float]], num_entries: int) -> int:
        axes = self.orientation_kwargs["Axes"]
        senses = self.orientation_kwargs["Senses"].split(",")
        num_ax, num_senses = len(axes), len(senses)
        errors = []
        if num_entries != num_ax:
            errors.append(f"Number of Angles ({num_entries}) does not match number of goniometer axes ({num_ax})")
        if num_entries != num_senses:
            errors.append(f"Number of Angles ({num_entries}) does not match number of goniometer senses ({num_senses})")
        if errors:
            logger.error("\n".join(errors) + "\n")
            return self._DEFAULT_NUM_AXES
        vecs = [self.axis_dict[ax.lower()] for ax in axes]
        sense_ints = [int(sense) for sense in senses]
        for angles in goniometer_lists:
            self.add_orientation()
            new_index = self.get_num_orientations() - 1
            # the file angles are the source of truth; use them unrounded for both the displayed
            # string and the rotation so the two cannot disagree
            self.update_gonio_string(vecs, sense_ints, angles, new_index)
            self.update_gRs(vecs, sense_ints, angles, new_index)
        return num_ax

    # helpers for converting view fields into typed values --------------
    def get_vecs(self, all_vec_strings: List[str], num_gonios: int) -> List[np.ndarray]:
        vec_strings = all_vec_strings[:num_gonios]
        return [vec_string_to_norm_array(vec_string) for vec_string in vec_strings]

    def get_senses(self, senses: List[str], num_gonios: int) -> List[float]:
        return [self.sense_vals[x] for x in senses[:num_gonios]]

    @staticmethod
    def get_angles(angles: List[str], num_gonios: int) -> List[float]:
        return [float(x) for x in angles[:num_gonios]]

    # validation helper

    @staticmethod
    def _validate_gR_data(vecs, senses, angles) -> None:
        if len({len(vecs), len(senses), len(angles)}) != 1:
            raise ValueError(f"Goniometer data lists have unequal lengths: vecs={len(vecs)}, senses={len(senses)}, angles={len(angles)}")
