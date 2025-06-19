from mantid.simpleapi import CreateSampleWorkspace, CreateSampleShape, logger
import numpy as np
from mantid.api import AnalysisDataService as ADS
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


def is_valid_mesh(mesh):
    return len(mesh) > 3


def has_no_valid_shape(ws_name):
    no_shape = False
    try:
        no_shape = not is_valid_mesh(ADS.retrieve(ws_name).sample().getShape().getMesh())
    except RuntimeError:
        no_shape = True
    return no_shape


def get_xml_mesh(xml):
    try:
        tmp_ws = CreateSampleWorkspace()
        CreateSampleShape(tmp_ws, xml)
        mesh = tmp_ws.sample().getShape().getMesh()
        ADS.remove("tmp_ws")
        return mesh
    except RuntimeError:
        logger.error(f"Shape mesh could not be constructed, check xml string is ok: '{xml}'")
        return []


def get_scaled_intrinsic_sample_directions_in_lab_frame(ax_transform, rotation_matrix, sample_mesh, scale=1.2):
    # apply the rotation matrix to the axes
    rotated_ax_transform = rotation_matrix @ ax_transform
    # transform the mesh vertices into new axes frame
    rot_vert = get_mesh_vertices_in_intrinsic_sample_axes(rotated_ax_transform, sample_mesh)
    # find the furthest vertex projected along each axis
    arrow_lens = np.dot(rotated_ax_transform, rot_vert).max(axis=1) * scale
    rd = rotated_ax_transform[:, 0]
    nd = rotated_ax_transform[:, 1]
    td = rotated_ax_transform[:, 2]
    return rd, nd, td, arrow_lens


def get_mesh_vertices(mesh):
    n_faces = mesh.shape[0]
    return np.unique(mesh.reshape((n_faces * 3, 3)), axis=0)


def get_mesh_vertices_in_intrinsic_sample_axes(rotated_ax_transform, sample_mesh):
    # change of coordinate basis: S.T@v@S where S is the transformation matrix and v is a vector
    verts = get_mesh_vertices(sample_mesh)
    return np.asarray([rotated_ax_transform.T @ v @ rotated_ax_transform for v in verts]).T


def plot_sample_directions(fig, ws_name, ax_transform, ax_labels, reference_ws=None):
    ax = fig.axes[0]
    if not ws_name:
        ws_name = reference_ws
        # if we are looking at the reference, we want to rotate the sample independently of our definition axes
        rotation_matrix = np.eye(3)
        ws = ADS.retrieve(ws_name)
    else:
        ws = ADS.retrieve(ws_name)
        # if not looking at the reference, we want our definition axes to rotate with the sample
        rotation_matrix = ws.getRun().getGoniometer().getR()
    sample_mesh = ws.sample().getShape().getMesh()
    rd, nd, td, arrow_lens = get_scaled_intrinsic_sample_directions_in_lab_frame(ax_transform, rotation_matrix, sample_mesh, scale=1.2)
    s_rd, s_nd, s_td = rd * arrow_lens[0], nd * arrow_lens[1], td * arrow_lens[2]
    ax.quiver(0, 0, 0, *s_rd, color="red", length=arrow_lens[0], normalize=True, arrow_length_ratio=0.05)
    ax.quiver(0, 0, 0, *s_nd, color="green", length=arrow_lens[1], normalize=True, arrow_length_ratio=0.05)
    ax.quiver(0, 0, 0, *s_td, color="blue", length=arrow_lens[2], normalize=True, arrow_length_ratio=0.05)
    ax.text(*s_rd, ax_labels[0])
    ax.text(*s_nd, ax_labels[1])
    ax.text(*s_td, ax_labels[2])


def plot_gauge_vol(gauge_vol_str, fig):
    if gauge_vol_str:
        mesh = get_xml_mesh(gauge_vol_str)
        if is_valid_mesh(mesh):
            axes = fig.gca()
            mesh_polygon = Poly3DCollection(mesh, facecolors="cyan", edgecolors="black", linewidths=0.1, alpha=0.25)
            axes.add_collection3d(mesh_polygon)
