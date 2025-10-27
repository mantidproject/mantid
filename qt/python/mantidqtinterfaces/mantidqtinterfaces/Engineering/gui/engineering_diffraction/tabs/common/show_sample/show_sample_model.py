# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import numpy as np
from mantid.api import AnalysisDataService as ADS
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from typing import Sequence
from mantid.kernel import logger
from mantidqt.plotting import sample_shape
from Engineering.common.texture_sample_viewer import get_scaled_intrinsic_sample_directions_in_lab_frame, get_xml_mesh, is_valid_mesh


class ShowSampleModel(object):
    def __init__(self, inc_gauge_vol=False, fix_axes_to_sample=True):
        self.ws_name = None
        self.include_gauge_vol = inc_gauge_vol
        self.fix_axes_to_sample = fix_axes_to_sample
        self.gauge_vol_str = ""
        self.fig = None

    def set_ws_name(self, ws_name):
        self.ws_name = ws_name

    def set_fix_axes_to_sample(self, fix_axes):
        self.fix_axes_to_sample = fix_axes

    def set_include_gauge_vol(self, inc_gauge_vol):
        self.include_gauge_vol = inc_gauge_vol

    def set_gauge_vol_str(self, gauge_vol_str):
        self.gauge_vol_str = gauge_vol_str

    def show_shape_plot(self, ax_transform, ax_labels):
        try:
            self.fig = sample_shape.plot_sample_container_and_components(self.ws_name, alpha=0.5, custom_color="grey")
            self.plot_sample_directions(ax_transform, ax_labels, self.fix_axes_to_sample)
            if self.include_gauge_vol:
                self.plot_gauge_vol()
        except Exception as exception:
            logger.warning("Could not show sample shape for workspace '{}':\n{}\n".format(self.ws_name, exception))

    def plot_sample_directions(self, ax_transform: np.ndarray, ax_labels: Sequence[str], fix_axes_to_sample: bool = True) -> None:
        ax = self.fig.axes[0]
        ws = ADS.retrieve(self.ws_name)
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

    def plot_gauge_vol(self) -> None:
        if self.gauge_vol_str:
            mesh = get_xml_mesh(self.gauge_vol_str)
            if is_valid_mesh(mesh):
                axes = self.fig.gca()
                mesh_polygon = Poly3DCollection(mesh, facecolors="cyan", edgecolors="black", linewidths=0.1, alpha=0.25)
                axes.add_collection3d(mesh_polygon)
