# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from matplotlib.axes import Axes

from mantidqt.plotting.sample_shape import plot_sample_only
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel
from Engineering.texture.texture_helper import (
    azim_proj_xy,
    get_alpha_beta_from_cart,
    ring,
    ster_proj_xy,
)
from typing import Protocol, ValuesView, List, ItemsView, Tuple
from abc import abstractmethod
from mantid.api import MatrixWorkspace
from scipy.spatial.transform import Rotation
from dataclasses import dataclass


class _WorkspaceManagerType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    ws: MatrixWorkspace
    ungrouped_ws: MatrixWorkspace
    updated_mesh_ws: MatrixWorkspace
    wsname: str
    gauge_volume_str: str
    scattering_centre: np.ndarray

    @abstractmethod
    def copy_sample_preserving_initial_rotation(self, source_ws: MatrixWorkspace, dest_ws: MatrixWorkspace) -> None:
        pass


class _InstrumentType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    @abstractmethod
    def get_grouping_path(self) -> str:
        pass


class _OrientationType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    include: bool
    select: bool
    R: Rotation
    transmission: np.ndarray | None
    gRs: List[Rotation]
    pf_points: np.ndarray | None


class _OrientationTableType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    @abstractmethod
    def values(self) -> ValuesView[_OrientationType]:
        pass

    @abstractmethod
    def items(self) -> ItemsView[int, _OrientationType]:
        pass

    @abstractmethod
    def __getitem__(self, index: int) -> _OrientationType:
        pass


class _GeometryType(Protocol):
    detQs_lab: np.ndarray
    det_k: np.ndarray


class _BaseModelType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    workspaces: _WorkspaceManagerType
    instrument: _InstrumentType
    orientations: _OrientationTableType
    geometry: _GeometryType

    gonio_index: int
    ax_transform: np.ndarray
    dir_names: List[str]
    projection: str
    plot_transmission: bool


@dataclass
class PlotProperties:
    sample_vec_scaler: float = 1.2
    num_points_in_ring: int = 360
    sample_opacity: float = 0.5
    sample_colour: str = "grey"
    goniometer_axis_scale: float = 2
    diffraction_vector_scale: float = 1.33
    arrow_head_size: float = 0.05
    cbar_inset_location: Tuple[float, float, float, float] = (0.9, 0.15, 0.05, 0.7)
    pf_ax_lims: Tuple[float, float] = (-1.5, 1.5)
    d1_name_loc: Tuple[float, float] = (-0.95, -0.8)
    d2_name_loc: Tuple[float, float] = (-0.8, -0.95)

    def goniometer_plot_scales(self, n_gon: int, current_index: int, sample_size_scaler: float) -> float:
        # get the scaler for the current_index goniometer in a progression of decreasing scales for n_goniometers
        return (1 + ((n_gon - current_index) / 2)) * sample_size_scaler


class TexturePlotter:
    """Renders the lab-frame 3D scene and the 2D pole-figure projection.

    Reads model state at draw time (workspace, detector Qs, sample mesh,
    orientation table, visualisation settings) via back-reference.
    """

    def __init__(self, model: _BaseModelType):
        self._model = model
        # visual / rendering config owned by the plotter (the colour/label constants plus the
        # visualisation toggles the settings dialog writes to)
        self.gon_colors = ("hotpink", "orange", "purple", "goldenrod", "plum", "saddlebrown")
        self.dir_cols = ("red", "green", "blue")
        self.vis_settings = {"directions": True, "goniometers": True, "incident": True, "ks": True, "scattered": False}
        # when True the transmission plot colour scale spans the data range; otherwise it is fixed to [0, 1]
        self.transmission_use_data_range = False
        self._transmission_cax = None
        self._plot_properties = PlotProperties()

    def update_plot(
        self, vecs: List[np.ndarray], senses: List[float], angles: List[float], fig: Figure, lab_ax: Axes, proj_ax: Axes, current_index: int
    ) -> None:
        # Easier to rebuild whole plot rather than updating individual artists
        lab_ax.clear()
        proj_ax.clear()

        # pull the orientation information
        orientation = self._model.orientations[current_index]
        gRs = orientation.gRs
        R = orientation.R
        n_gon = len(gRs)
        # and the location of the scattering centre
        scat_centre = self._model.workspaces.scattering_centre

        # set the goniometer on the workspace
        self._model.workspaces.ws.run().getGoniometer().setR(R.as_matrix())

        # get a copy of the unrotated and rotated sample shape mesh
        shape_mesh = self._model.workspaces.updated_mesh_ws.sample().getShape().getMesh().copy()
        rot_mesh = R.apply(shape_mesh.reshape((-1, 3))).reshape(shape_mesh.shape)

        # calculate a scaling value that is based on how far the furthest vertex of the sample mesh is from the origin
        extent = (np.linalg.norm(shape_mesh, axis=(1, 2)).max() / 2) * self._plot_properties.sample_vec_scaler

        # draw the lab frame
        g_vecs = self._draw_goniometers(lab_ax, vecs, senses, angles, gRs, n_gon, extent)
        self._draw_sample_and_axes(fig, lab_ax, rot_mesh, extent, n_gon, scat_centre)
        self._draw_beam_and_detectors(lab_ax, scat_centre, extent, n_gon)
        lab_ax.set_axis_off()

        # draw the pole figure plot
        g_pole_xy = self._project_goniometer_poles(R, g_vecs)
        self._draw_pole_figure(proj_ax, g_pole_xy, current_index)
        self._decorate_pole_figure(proj_ax)

        # ensure the plots are updated
        fig.canvas.draw_idle()
        proj_ax.figure.canvas.draw_idle()

    def _draw_goniometers(
        self,
        lab_ax: Axes,
        vecs: List[np.ndarray],
        senses: List[float],
        angles: List[float],
        gRs: List[Rotation],
        n_gon: int,
        extent: float,
    ) -> List[np.ndarray | int]:

        # iterate through the goniometers and draw them to the lab frame
        g_vecs = []
        for i, vec in enumerate(vecs):
            # get the rotation matrix corresponding to all the goniometer transformations beneath this one
            gR = gRs[i]
            # apply the rotation to the axis of this goniometer
            g_vec = gR.apply(vec)
            g_vecs.append(g_vec)

            # calculate appropriate scaling values so each goniometer sits inside those it depends on
            gon_scale = self._plot_properties.goniometer_plot_scales(n_gon, i, extent)
            # read the number of points to use for the goniometer ring rendering
            _ring_res = self._plot_properties.num_points_in_ring

            # apply the transformation of the outer goniometers on the ring render
            gon_ring = gR.apply(ring(vec, gon_scale, res=_ring_res).T).T

            # calculate which way round the ring points should be ordered
            angle = angles[i] * senses[i]
            if angle <= 0:
                gon_ring = np.flip(gon_ring, axis=1)  # reverse the ring if the angle is negative
            # find the nearest index of point in the ring so the coloured section can be drawn to the correct arc
            pos_ind = int(np.abs(angle) / 360 * _ring_res)

            # plot the goniometer coloured section (corresponding to the angle rotated through),
            # the uncoloured section (corresponding to the remaining rotation of the goniometer), and
            # the goniometer axis
            if self.vis_settings["goniometers"]:
                lab_ax.plot(*gon_ring[:, : pos_ind + 1], color=self.gon_colors[i])
                lab_ax.plot(*gon_ring[:, pos_ind:], color="grey")
                lab_ax.quiver(
                    *np.zeros(3),
                    *g_vec * extent * self._plot_properties.goniometer_axis_scale,
                    color=self.gon_colors[i],
                    ls=("-", "--")[int(i != self._model.gonio_index)],
                    label=f"Axis {i}",
                )
        if self.vis_settings["goniometers"]:
            lab_ax.legend()
        return g_vecs

    def _draw_sample_and_axes(
        self, fig: Figure, lab_ax: Axes, rot_mesh: np.ndarray, extent: float, n_gon: int, scat_centre: np.ndarray
    ) -> None:
        # set current axis of fig to lab frame
        fig.sca(lab_ax)
        # plot the sample mesh
        plot_sample_only(fig, rot_mesh, self._plot_properties.sample_opacity, self._plot_properties.sample_colour)

        # get a show sample model helper class for plotting the sample direction vectors + gauge volume
        sample_model = ShowSampleModel(fig=fig, ws_name=self._model.workspaces.wsname)
        # set the current gauge volume
        sample_model.set_gauge_vol_str(self._model.workspaces.gauge_volume_str)

        # if there is a gauge volume, plot it
        if self._model.workspaces.gauge_volume_str:
            sample_model.plot_gauge_vol()

        # if the setting want the sample directions plotted, plot them
        if self.vis_settings["directions"]:
            sample_model.plot_sample_directions(self._model.ax_transform, self._model.dir_names, scat_centre=scat_centre)

        # scale the lab frame
        lim = [-(abs_lim := extent * n_gon / 1.5), abs_lim]
        lab_ax.set(xlim=lim, ylim=lim, zlim=lim, aspect="equal")

    def _draw_beam_and_detectors(self, lab_ax: Axes, scat_centre: np.ndarray, extent: float, n_gon: int) -> None:
        # get the screen space positions for the beam direction arrow
        comp_info = self._model.workspaces.ws.componentInfo()
        ki = scat_centre - np.array(comp_info.sourcePosition())
        ki = (ki / np.linalg.norm(ki)) * extent * n_gon * self._plot_properties.diffraction_vector_scale

        # if the incident beam is desired, plot it
        if self.vis_settings["incident"]:
            lab_ax.quiver(*(-ki), *ki, arrow_length_ratio=self._plot_properties.arrow_head_size, color="black", alpha=0.25)

        # if the diffraction vectors are desired, plot them
        if self.vis_settings["ks"]:
            self._draw_quiver_bundle(lab_ax, self._model.geometry.detQs_lab, scat_centre, extent, "dodgerblue", linestyle="--")
        # if the detector positions are desired, plot them
        if self.vis_settings["scattered"]:
            self._draw_quiver_bundle(lab_ax, np.asarray(self._model.geometry.det_k), scat_centre, extent, "grey")

    def _draw_quiver_bundle(
        self, lab_ax: Axes, dirs: np.ndarray, scat_centre: np.ndarray, extent: float, tip_color: str, linestyle: str = "-"
    ) -> None:
        # get screen space positions of the provided vectors
        scaled = dirs * (1.25 * extent)
        tips = scaled + scat_centre[None, :]
        n = len(scaled)

        # plot the arrows
        lab_ax.quiver(
            np.ones(n) * scat_centre[0],
            np.ones(n) * scat_centre[1],
            np.ones(n) * scat_centre[2],
            scaled[:, 0],
            scaled[:, 1],
            scaled[:, 2],
            arrow_length_ratio=self._plot_properties.arrow_head_size,
            color="grey",
            alpha=0.25,
            linestyle=linestyle,
        )
        # plot the points at the end of the arrows
        lab_ax.scatter(tips[:, 0], tips[:, 1], tips[:, 2], color=tip_color, s=2)

    def _project_goniometer_poles(self, R: Rotation, g_vecs: List[np.ndarray | int]) -> np.ndarray:
        g_pole = R.inv().apply(np.array(g_vecs)) @ self._model.ax_transform
        cart_g_pole = get_alpha_beta_from_cart(g_pole.T)
        return ster_proj_xy(*cart_g_pole.T) if self._model.projection == "ster" else azim_proj_xy(*cart_g_pole.T)

    def _draw_pole_figure(self, proj_ax: Axes, g_pole_xy: np.ndarray, current_index: int) -> None:
        # update the transmission colour bar
        if self._transmission_cax is not None:
            self._transmission_cax.remove()
            self._transmission_cax = None

        # plot the goniometers onto the figure
        for i, gP in enumerate(g_pole_xy):
            pc = self.gon_colors[i]
            fc = "None" if i != self._model.gonio_index else pc
            # if the point is approximately on the edge of the pole figure circle, plot a line instead
            if np.isclose(np.linalg.norm(gP), 1):
                proj_ax.plot((gP[1], -gP[1]), (gP[0], -gP[0]), color=pc, ls=("-", "--")[int(i != self._model.gonio_index)])
            else:
                proj_ax.scatter(gP[1], gP[0], s=30, edgecolor=pc, facecolor=fc)

        # if there aren't transmission values
        if not self._model.plot_transmission:
            # go through all the orientations
            for i, orientation in self._model.orientations.items():
                if orientation.pf_points is None:
                    continue
                if orientation.include:
                    pf_xy = orientation.pf_points
                    # plot current selected orientation with full circles
                    if i == current_index:
                        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, c="dodgerblue")
                    # plot all other orientations as just edges
                    else:
                        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, facecolor="None", edgecolor="dodgerblue")
                # if the current selected orientation is not set to be included plot it as a grey edge
                elif i == current_index:
                    pf_xy = orientation.pf_points
                    proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, facecolor="None", edgecolor="grey", alpha=0.5)
        # if there are transmission values to be plotted
        else:
            # get the included runs
            included = [
                o for o in self._model.orientations.values() if o.include and o.pf_points is not None and o.transmission is not None
            ]
            if not included:
                return
            # get the locations of the included points
            all_pf_xy = np.concatenate([o.pf_points for o in included], axis=0)
            # get the values of the included points
            all_transmissions = np.concatenate([o.transmission for o in included], axis=0)
            # constrain the cmap range if desired
            clim_kwargs = {} if self.transmission_use_data_range else {"vmin": 0, "vmax": 1}
            # plot the points
            scatt = proj_ax.scatter(all_pf_xy[:, 1], all_pf_xy[:, 0], s=20, c=all_transmissions, cmap="jet", **clim_kwargs)
            self._transmission_cax = proj_ax.inset_axes(self._plot_properties.cbar_inset_location)
            proj_ax.figure.colorbar(scatt, cax=self._transmission_cax)

    def _decorate_pole_figure(self, proj_ax: Axes) -> None:
        proj_ax.set(xlim=self._plot_properties.pf_ax_lims, ylim=self._plot_properties.pf_ax_lims, aspect="equal")
        for i, bv in enumerate(np.eye(2)):
            proj_ax.quiver(*np.array((-1, -1)), *bv, color=self.dir_cols[-1 + i], scale=5)
        proj_ax.add_patch(plt.Circle((0, 0), 1, color="grey", fill=False, linestyle="-"))
        proj_ax.annotate(self._model.dir_names[0], self._plot_properties.d1_name_loc)
        proj_ax.annotate(self._model.dir_names[2], self._plot_properties.d2_name_loc)
        proj_ax.set_axis_off()
