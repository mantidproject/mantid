# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import numpy as np
from mantid.simpleapi import CreateSampleWorkspace, CreateSampleShape, logger
from mantid.api import AnalysisDataService as ADS
from typing import Sequence, Tuple


def is_valid_mesh(mesh: np.ndarray) -> bool:
    return len(mesh) > 3


def has_valid_shape(ws_name: str) -> bool:
    try:
        return is_valid_mesh(ADS.retrieve(ws_name).sample().getShape().getMesh())
    except RuntimeError:
        return False


def get_xml_mesh(xml: str) -> np.ndarray:
    try:
        tmp_ws = CreateSampleWorkspace()
        CreateSampleShape(tmp_ws, xml)
        mesh = tmp_ws.sample().getShape().getMesh()
        ADS.remove("tmp_ws")
        return mesh
    except RuntimeError:
        logger.error(f"Shape mesh could not be constructed, check xml string is ok: '{xml}'")
        return np.empty(0)


def get_scaled_intrinsic_sample_directions_in_lab_frame(
    ax_transform: np.ndarray, rotation_matrix: np.ndarray, sample_mesh: np.ndarray, scale: float = 1.2
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, Sequence[float]]:
    # apply the rotation matrix to the axes
    rotated_ax_transform = rotation_matrix @ ax_transform
    # transform the mesh vertices into new axes frame
    rot_vert = get_mesh_vertices_in_intrinsic_sample_axes(rotated_ax_transform, sample_mesh)
    # find the furthest vertex projected along each axis
    arrow_lens = rot_vert.max(axis=0) * scale
    rd = rotated_ax_transform[:, 0]
    nd = rotated_ax_transform[:, 1]
    td = rotated_ax_transform[:, 2]
    return rd, nd, td, arrow_lens


def get_mesh_vertices(mesh: np.ndarray):
    n_faces = mesh.shape[0]
    return np.unique(mesh.reshape((n_faces * 3, 3)), axis=0)


def get_mesh_vertices_in_intrinsic_sample_axes(rotated_ax_transform: np.ndarray, sample_mesh: np.ndarray) -> np.ndarray:
    # change of coordinate basis: S@v where S is the transformation matrix and v is a vector
    verts = get_mesh_vertices(sample_mesh)
    return np.asarray([rotated_ax_transform.T @ v for v in verts])
