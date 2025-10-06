# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import LoadEmptyInstrument, GroupDetectors, SetSampleShape
from Engineering.EnggUtils import GROUP, CALIB_DIR
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel
from Engineering.common.xml_shapes import get_cube_xml
from mantidqt.plotting.sample_shape import plot_sample_only
import os
import numpy as np
from scipy.spatial.transform import Rotation
import matplotlib.pyplot as plt
from mantid.kernel import logger


class TexturePlannerModel(object):
    def __init__(self, instrument="ENGINX", projection="azimuthal"):
        # probably static
        self.wsname = "__texture_planning_ws"
        self.sense_vals = {"Clockwise": -1, "Counterclockwise": 1}  # mantid convention
        self.supported_groups = ("Texture20", "Texture30", "banks")

        # properties which will be updated
        self.ws = None
        self.instr = instrument
        self.calib_info = None
        self.group = None
        self.ax_transform = np.eye(3)
        self.dir_names = ["D1", "D2", "D3"]
        self.dir_cols = ["red", "green", "blue"]
        self.gRs = []
        self.R = Rotation.identity()
        self.detQs_lab = None
        self.projection = projection
        self.gonio_index = 0

        # data structure
        self.saved_orientations = []

        # init func calls
        self.update_ws()

    def update_ws(self):
        if not self.ws:
            self.ws = LoadEmptyInstrument(InstrumentName=self.instr, OutputWorkspace=self.wsname)
            SetSampleShape(self.ws, get_cube_xml("default_cube", 0.01))
        # add handling here for copying over sample shape if the instrument is changed

    def set_group(self, group_str):
        self.group = GROUP(group_str)
        self.update_calib_info()

    def set_ax_transform(self, vec1, vec2, vec3):
        vec1, vec2, vec3 = vec_string_to_norm_array(vec1), vec_string_to_norm_array(vec2), vec_string_to_norm_array(vec3)
        self.ax_transform = np.concatenate((vec1[:, None], vec2[:, None], vec3[:, None]), axis=1)

    def set_dir_names(self, name1, name2, name3):
        self.dir_names = [name1, name2, name3]

    def set_gonio_index(self, index):
        self.gonio_index = index

    def update_calib_info(self):
        self.calib_info = CalibrationInfo(group=self.group)

    @staticmethod
    def get_default_texture_directions():
        return ("RD", "ND", "TD"), ((1, 0, 0), (0, 1, 0), (0, 0, 1))

    def get_num_orientations(self):
        return len(self.saved_orientations) + 1

    def get_vecs(self, all_vec_strings, num_gonios):
        vec_strings = all_vec_strings[:num_gonios]
        return [vec_string_to_norm_array(vec_string) for vec_string in vec_strings]

    def get_senses(self, senses, num_gonios):
        return [self.sense_vals[x] for x in senses[:num_gonios]]

    def get_angles(self, angles, num_gonios):
        return [float(x) for x in angles[:num_gonios]]

    def update_gonio_index(self, num_gonios):
        max_ind = num_gonios - 1
        return self.gonio_index if self.gonio_index <= max_ind else max_ind

    def get_gRs(self, vecs, senses, angles):
        gRs = [Rotation.identity()]
        R = Rotation.identity()
        for i, vec in enumerate(vecs):
            sense = -senses[i]  # mantid convention is that ccw = 1, we need cw = 1
            r_step = Rotation.from_davenport(vec, "extrinsic", sense * angles[i], degrees=True)
            R = R * r_step
            gRs.append(R)
        self.gRs = gRs
        self.R = R

    def get_detQ_lab(self):
        group_ws = GroupDetectors(
            InputWorkspace=self.ws,
            MapFile=os.path.join(CALIB_DIR, self.calib_info.get_group_file()),
            OutputWorkspace="group_ws",
            StoreInADS=False,
        )
        spec_info = group_ws.spectrumInfo()
        det_pos = np.asarray(
            [spec_info.position(i) / np.linalg.norm(spec_info.position(i)) for i in range(1, group_ws.getNumberHistograms())]
        )
        detQs_lab = det_pos - np.array((0, 0, 1))
        self.detQs_lab = detQs_lab / np.linalg.norm(detQs_lab, axis=1)[:, None]

    def update_plot(self, vecs, senses, angles, fig, lab_ax, proj_ax):
        lab_ax.clear()
        proj_ax.clear()

        gRs = self.gRs
        R = self.R
        nGon = len(gRs)

        gon_colors = ["hotpink", "orange", "purple", "goldenrod", "plum", "saddlebrown"]
        ge = ((nGon / 2) + 1) * 0.866

        gVecs = []

        detQs_lab = self.detQs_lab
        ax_transform = self.ax_transform

        self.ws.run().getGoniometer().setR(R.as_matrix())

        shape_mesh = self.ws.sample().getShape().getMesh()
        extent = (np.linalg.norm(shape_mesh, axis=(1, 2)).max() / 2) * 1.2

        rot_mesh = R.apply(shape_mesh.reshape((-1, 3))).reshape(shape_mesh.shape)
        rot_pos = R.inv().apply(detQs_lab) @ ax_transform

        cart_pos = get_alpha_beta_from_cart(rot_pos.T)
        pf_xy = ster_proj(*cart_pos.T) if self.projection == "ster" else azim_proj(*cart_pos.T)

        for i, vec in enumerate(vecs):
            gR = gRs[i]
            gon_ring = gR.apply(ring(vec, 1 + ((nGon - i) / 2), res=360).T).T
            gon_ring = gon_ring * extent  # scale to sample
            # lab_ax.plot(*gon_ring, color="grey")

            sense = -senses[i]  # mantid convention is that ccw = 1, we need cw = 1
            angle = angles[i] * sense

            if angle <= 0:
                gon_ring = np.flip(gon_ring, axis=1)  # reverse the ring if the angle is negative
            pos_ind = int(np.abs(angle))
            lab_ax.plot(*gon_ring[:, : pos_ind + 1], color=gon_colors[i])
            lab_ax.plot(*gon_ring[:, pos_ind:], color="grey")

            gVec = gRs[i].apply(vec)
            gVecs.append(gVec)
            lab_ax.quiver(*np.zeros(3), *gVec * extent * 2, color=gon_colors[i], ls=("-", "--")[int(i != self.gonio_index)])

        gPole = R.inv().apply(np.array(gVecs)) @ ax_transform
        cart_gPole = get_alpha_beta_from_cart(gPole.T)
        gPole_xy = ster_proj(*cart_gPole.T) if self.projection == "ster" else azim_proj(*cart_gPole.T)

        lab_ax.set_xlim([-ge, ge])
        lab_ax.set_ylim([-ge, ge])
        lab_ax.set_zlim([-ge, ge])
        lab_ax.set_axis_off()

        # 3D plot
        sample_model = ShowSampleModel()
        sample_model.fig = fig
        sample_model.ws_name = self.wsname
        fig.sca(lab_ax)
        plot_sample_only(fig, rot_mesh * 0.5, 0.5, "grey")
        sample_model.plot_sample_directions(ax_transform, self.dir_names)
        lab_ax.set_xlim([-extent * nGon / 2, extent * nGon / 2])
        lab_ax.set_ylim([-extent * nGon / 2, extent * nGon / 2])
        lab_ax.set_zlim([-extent * nGon / 2, extent * nGon / 2])
        [lab_ax.quiver(*np.zeros(3), *dQ * 1.25 * extent, arrow_length_ratio=0.05, color="grey", alpha=0.25) for dQ in detQs_lab]
        [
            lab_ax.scatter(
                *dQ * 1.25 * extent,
                color="dodgerblue",
                s=2,
            )
            for i, dQ in enumerate(detQs_lab)
        ]
        lab_ax.set_axis_off()

        # 2D plot
        for i, gP in enumerate(gPole_xy):
            pc = gon_colors[i]
            fc = "None" if i != self.gonio_index else pc
            if np.isclose(np.linalg.norm(gP), 1):
                proj_ax.plot((gP[1], -gP[1]), (gP[0], -gP[0]), color=pc, ls=("-", "--")[int(i != self.gonio_index)])
            else:
                proj_ax.scatter(gP[1], gP[0], s=30, edgecolor=pc, facecolor=fc)
        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, c="dodgerblue")

        # for i, tpf in enumerate(temp_arr_points):
        #    fc = "None" if i != temp_ind[0] else "dodgerblue"
        #    proj_ax.scatter(tpf[:, 1], tpf[:, 0], s=20, facecolor=fc, edgecolor='dodgerblue', alpha=0.2)

        proj_ax.set_aspect("equal")
        proj_ax.set_xlim(-1.1, 1.1)
        proj_ax.set_ylim(-1.1, 1.1)
        [proj_ax.quiver(*np.array((-1, -1)), *bv, color=self.dir_cols[-1 + i], scale=5) for i, bv in enumerate(np.eye(2))]
        circle = plt.Circle((0, 0), 1, color="grey", fill=False, linestyle="-")
        proj_ax.add_patch(circle)
        proj_ax.annotate(self.dir_names[0], (-0.95, -0.8))
        proj_ax.annotate(self.dir_names[2], (-0.8, -0.95))
        proj_ax.set_axis_off()

        fig.canvas.draw_idle()
        proj_ax.figure.canvas.draw_idle()
        return pf_xy


def ring(axis, r=1, res=100, offset=(0, 0, 0)):
    u = np.linspace(0, 2 * np.pi, res)
    a = r * np.cos(u) + offset[0]
    b = r * np.sin(u) + offset[1]
    c = np.zeros_like(a) + offset[2]
    points = np.concatenate((a[None, :], b[None, :], c[None, :]), axis=0)
    R, _ = Rotation.align_vectors(axis, np.array((0, 0, 1)))
    return R.apply(points.T).T


def get_alpha_beta_from_cart(q_sample_cart: np.ndarray) -> np.ndarray:
    """
    get spherical angles from cartesian coordinates
    alpha is angle from positive x towards positive z
    beta is angle from positive y
    """
    q_sample_cart = np.clip(q_sample_cart.copy(), -1.0, 1.0)  # numerical inaccuracies outside this range will give nan in the trig funcs
    q_sample_cart = np.where(q_sample_cart[1] < 0, -q_sample_cart, q_sample_cart)  # invert the southern points
    alphas = np.arctan2(q_sample_cart[2], q_sample_cart[0])
    betas = np.arccos(q_sample_cart[1])
    return np.concatenate([alphas[:, None], betas[:, None]], axis=1)


def spherical_to_cartesian(phi, theta):
    """
    Convert spherical angles to 3D Cartesian unit vector.
    phi: azimuthal angle [0, 2pi]
    theta: polar angle [0, pi/2]
    """
    x = np.sin(theta) * np.cos(phi)
    y = np.cos(theta)
    z = np.sin(theta) * np.sin(phi)
    return np.stack((x, y, z), axis=-1)


def ster_proj(alphas: np.ndarray, betas: np.ndarray) -> np.ndarray:
    betas = np.pi - betas  # this formula projects onto the north pole, and beta is taken from the south
    r = np.sin(betas) / (1 - np.cos(betas))
    out = np.zeros((len(alphas), 2))
    out[:, 0] = r * np.cos(alphas)
    out[:, 1] = r * np.sin(alphas)
    return out


def azim_proj(alphas: np.ndarray, betas: np.ndarray) -> np.ndarray:
    betas = betas / (np.pi / 2)
    xs = (betas * np.cos(alphas))[:, None]
    zs = (betas * np.sin(alphas))[:, None]
    out = np.concatenate([xs, zs], axis=1)
    return out


def vec_string_to_norm_array(vec_string):
    if len(vec_string.split(",")) != 3:
        logger.error(f"Vector string provided is not the correct length: {vec_string}, (1,0,0) has been used instead")
        return np.array((1, 0, 0))
    try:
        vec = np.asarray([float(x) for x in vec_string.split(",")])
    except Exception:
        logger.error(f"Invalid vector string provided: {vec_string}, (1,0,0) has been used instead")
        # this is anticipated when entering goniometer axis, we just return it to default on error to prevent crashes
        # from the auto plot updates
        return np.array((1, 0, 0))
    return vec / np.linalg.norm(vec)
