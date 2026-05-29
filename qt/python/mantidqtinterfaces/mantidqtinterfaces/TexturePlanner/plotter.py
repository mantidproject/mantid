# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
import matplotlib.pyplot as plt

from mantidqt.plotting.sample_shape import plot_sample_only
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel
from Engineering.texture.texture_helper import (
    azim_proj_xy,
    get_alpha_beta_from_cart,
    ring,
    ster_proj_xy,
)


class TexturePlotter:
    """Renders the lab-frame 3D scene and the 2D pole-figure projection.

    Reads model state at draw time (workspace, detector Qs, sample mesh,
    orientation table, visualisation settings) via a back-reference. Owns no
    persistent state of its own — `update_plot` is a pure function of the model.
    """

    def __init__(self, model):
        self._model = model

    def update_plot(self, vecs, senses, angles, fig, lab_ax, proj_ax, current_index):
        m = self._model
        lab_ax.clear()
        proj_ax.clear()

        orientation = m.orientations[current_index]
        gRs = orientation.gRs
        R = orientation.R
        n_gon = len(gRs)

        m.workspaces.ws.run().getGoniometer().setR(R.as_matrix())

        shape_mesh = m.workspaces.updated_mesh_ws.sample().getShape().getMesh().copy()
        extent = (np.linalg.norm(shape_mesh, axis=(1, 2)).max() / 2) * 1.2
        rot_mesh = R.apply(shape_mesh.reshape((-1, 3))).reshape(shape_mesh.shape)

        m.workspaces.update_scattering_centre()
        scat_centre = m.workspaces.scattering_centre

        g_vecs = self._draw_goniometers(lab_ax, vecs, senses, angles, gRs, n_gon, extent)
        self._draw_sample_and_axes(fig, lab_ax, rot_mesh, extent, n_gon, scat_centre)
        self._draw_beam_and_detectors(lab_ax, scat_centre, extent, n_gon)
        lab_ax.set_axis_off()

        g_pole_xy = self._project_goniometer_poles(R, g_vecs)
        self._draw_pole_figure(proj_ax, g_pole_xy, current_index)
        self._decorate_pole_figure(proj_ax)

        fig.canvas.draw_idle()
        proj_ax.figure.canvas.draw_idle()

    def _draw_goniometers(self, lab_ax, vecs, senses, angles, gRs, n_gon, extent):
        m = self._model
        g_vecs = []
        for i, vec in enumerate(vecs):
            gR = gRs[i]
            g_vec = gR.apply(vec)
            g_vecs.append(g_vec)

            gon_scale = (1 + ((n_gon - i) / 2)) * extent
            _ring_res = 360
            gon_ring = gR.apply(ring(vec, gon_scale, res=_ring_res).T).T

            angle = angles[i] * senses[i]
            if angle <= 0:
                gon_ring = np.flip(gon_ring, axis=1)  # reverse the ring if the angle is negative
            pos_ind = int(np.abs(angle) / 360 * _ring_res)

            if m.vis_settings["goniometers"]:
                lab_ax.plot(*gon_ring[:, : pos_ind + 1], color=m.gon_colors[i])
                lab_ax.plot(*gon_ring[:, pos_ind:], color="grey")
                lab_ax.quiver(
                    *np.zeros(3),
                    *g_vec * extent * 2,
                    color=m.gon_colors[i],
                    ls=("-", "--")[int(i != m.gonio_index)],
                    label=f"Axis {i}",
                )
        if m.vis_settings["goniometers"]:
            lab_ax.legend()
        return g_vecs

    def _draw_sample_and_axes(self, fig, lab_ax, rot_mesh, extent, n_gon, scat_centre):
        m = self._model
        sample_model = ShowSampleModel()
        sample_model.fig = fig
        sample_model.ws_name = m.workspaces.wsname
        sample_model.gauge_vol_str = m.workspaces.gauge_volume_str
        fig.sca(lab_ax)
        plot_sample_only(fig, rot_mesh, 0.5, "grey")
        if m.vis_settings["directions"]:
            sample_model.plot_sample_directions(m.ax_transform, m.dir_names, scat_centre=scat_centre)
        lim = extent * n_gon / 1.5
        lab_ax.set_xlim([-lim, lim])
        lab_ax.set_ylim([-lim, lim])
        lab_ax.set_zlim([-lim, lim])
        lab_ax.set_aspect("equal")
        if m.workspaces.gauge_volume_str:
            sample_model.plot_gauge_vol()

    def _draw_beam_and_detectors(self, lab_ax, scat_centre, extent, n_gon):
        m = self._model
        comp_info = m.workspaces.ws.componentInfo()
        ki = scat_centre - np.array(comp_info.sourcePosition())
        ki = (ki / np.linalg.norm(ki)) * extent * n_gon / 0.75
        if m.vis_settings["incident"]:
            lab_ax.quiver(*(-ki), *ki, arrow_length_ratio=0.05, color="black", alpha=0.25)

        if m.vis_settings["ks"]:
            self._draw_quiver_bundle(lab_ax, m.geometry.detQs_lab, scat_centre, extent, "dodgerblue", linestyle="--")
        if m.vis_settings["scattered"]:
            self._draw_quiver_bundle(lab_ax, np.asarray(m.geometry.det_k), scat_centre, extent, "grey")

    @staticmethod
    def _draw_quiver_bundle(lab_ax, dirs, scat_centre, extent, tip_color, linestyle="-"):
        scaled = dirs * (1.25 * extent)
        tips = scaled + scat_centre[None, :]
        n = len(scaled)
        lab_ax.quiver(
            np.ones(n) * scat_centre[0],
            np.ones(n) * scat_centre[1],
            np.ones(n) * scat_centre[2],
            scaled[:, 0],
            scaled[:, 1],
            scaled[:, 2],
            arrow_length_ratio=0.05,
            color="grey",
            alpha=0.25,
            linestyle=linestyle,
        )
        lab_ax.scatter(tips[:, 0], tips[:, 1], tips[:, 2], color=tip_color, s=2)

    def _project_goniometer_poles(self, R, g_vecs):
        m = self._model
        g_pole = R.inv().apply(np.array(g_vecs)) @ m.ax_transform
        cart_g_pole = get_alpha_beta_from_cart(g_pole.T)
        return ster_proj_xy(*cart_g_pole.T) if m.projection == "ster" else azim_proj_xy(*cart_g_pole.T)

    def _draw_pole_figure(self, proj_ax, g_pole_xy, current_index):
        m = self._model
        for i, gP in enumerate(g_pole_xy):
            pc = m.gon_colors[i]
            fc = "None" if i != m.gonio_index else pc
            if np.isclose(np.linalg.norm(gP), 1):
                proj_ax.plot((gP[1], -gP[1]), (gP[0], -gP[0]), color=pc, ls=("-", "--")[int(i != m.gonio_index)])
            else:
                proj_ax.scatter(gP[1], gP[0], s=30, edgecolor=pc, facecolor=fc)

        if not m.plot_attenuation:
            for i, orientation in m.orientations.items():
                if orientation.include:
                    pf_xy = orientation.pf_points
                    if i == current_index:
                        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, c="dodgerblue")
                    else:
                        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, facecolor="None", edgecolor="dodgerblue")
                elif i == current_index:
                    pf_xy = orientation.pf_points
                    proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, facecolor="None", edgecolor="grey", alpha=0.5)
        else:
            included = [o for o in m.orientations.values() if o.include]
            all_pf_xy = np.concatenate([o.pf_points for o in included], axis=0)
            all_mus = np.concatenate([o.mu for o in included], axis=0)
            scatt = proj_ax.scatter(all_pf_xy[:, 1], all_pf_xy[:, 0], s=20, c=all_mus, vmin=0, vmax=1, cmap="jet")
            cax = proj_ax.inset_axes([0.9, 0.15, 0.05, 0.7])
            proj_ax.figure.colorbar(scatt, cax=cax)

    def _decorate_pole_figure(self, proj_ax):
        m = self._model
        proj_ax.set_aspect("equal")
        proj_ax.set_xlim(-1.5, 1.5)
        proj_ax.set_ylim(-1.5, 1.5)
        for i, bv in enumerate(np.eye(2)):
            proj_ax.quiver(*np.array((-1, -1)), *bv, color=m.dir_cols[-1 + i], scale=5)
        proj_ax.add_patch(plt.Circle((0, 0), 1, color="grey", fill=False, linestyle="-"))
        proj_ax.annotate(m.dir_names[0], (-0.95, -0.8))
        proj_ax.annotate(m.dir_names[2], (-0.8, -0.95))
        proj_ax.set_axis_off()
