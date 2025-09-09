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
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from matplotlib.figure import Figure
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


def plot_sample_directions(
    fig: Figure, ws_name: str, ax_transform: np.ndarray, ax_labels: Sequence[str], fix_axes_to_sample: bool = True
) -> None:
    ax = fig.axes[0]
    ws = ADS.retrieve(ws_name)
    rotation_matrix = ws.getRun().getGoniometer().getR() if fix_axes_to_sample else np.eye(3)
    sample_mesh = ws.sample().getShape().getMesh()
    rd, nd, td, arrow_lens = get_scaled_intrinsic_sample_directions_in_lab_frame(ax_transform, rotation_matrix, sample_mesh, scale=1.2)
    s_rd, s_nd, s_td = rd * arrow_lens[0], nd * arrow_lens[1], td * arrow_lens[2]
    ax.quiver(0, 0, 0, *s_rd, color="red", length=arrow_lens[0], normalize=True, arrow_length_ratio=0.05)
    ax.quiver(0, 0, 0, *s_nd, color="green", length=arrow_lens[1], normalize=True, arrow_length_ratio=0.05)
    ax.quiver(0, 0, 0, *s_td, color="blue", length=arrow_lens[2], normalize=True, arrow_length_ratio=0.05)
    ax.text(*s_rd, ax_labels[0])
    ax.text(*s_nd, ax_labels[1])
    ax.text(*s_td, ax_labels[2])


def plot_gauge_vol(gauge_vol_str: str, fig: Figure) -> None:
    if gauge_vol_str:
        mesh = get_xml_mesh(gauge_vol_str)
        if is_valid_mesh(mesh):
            axes = fig.gca()
            mesh_polygon = Poly3DCollection(mesh, facecolors="cyan", edgecolors="black", linewidths=0.1, alpha=0.25)
            axes.add_collection3d(mesh_polygon)
