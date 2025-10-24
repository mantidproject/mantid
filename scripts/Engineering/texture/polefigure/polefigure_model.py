# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    CreatePoleFigureTableWorkspace,
    CloneWorkspace,
    CombineTableWorkspaces,
    logger,
    SaveNexus,
    CreateEmptyTableWorkspace,
    SaveAscii,
)
import numpy as np
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence, Tuple
import matplotlib.pyplot as plt
from os import path, makedirs
from scipy.interpolate import griddata
from scipy.ndimage import gaussian_filter
from Engineering.common.texture_sample_viewer import has_valid_shape
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from Engineering.texture.xtal_helper import get_xtal_structure


class TextureProjection:
    # ~~~~~ Pole Figure Data functions ~~~~~~~~

    def make_pole_figure_tables(
        self,
        wss: Sequence[str],
        peak_wss: Optional[Sequence[str]],
        out_ws: str,
        hkl: Optional[Sequence[int]],
        inc_scatt_corr: bool,
        scat_vol_pos: Sequence[float],
        chi2_thresh: Optional[float],
        peak_thresh: Optional[float],
        save_dirs: Optional[Sequence[str]] = None,
        ax_transform: Sequence[float] = np.eye(3),
        readout_col: str = "",
    ) -> None:
        flat_ax_transform = np.reshape(ax_transform, (9,))
        table_workspaces = []
        if peak_wss and (len(peak_wss) == len(wss)):
            for iws, ws in enumerate(wss):
                ws_str = f"_{iws}_abi_table"
                CreatePoleFigureTableWorkspace(
                    InputWorkspace=ws,
                    PeakParameterWorkspace=peak_wss[iws],
                    OutputWorkspace=ws_str,
                    Reflection=hkl,
                    Chi2Threshold=chi2_thresh,
                    PeakPositionThreshold=peak_thresh,
                    ApplyScatteringPowerCorrection=inc_scatt_corr,
                    ScatteringVolumePosition=scat_vol_pos,
                    AxesTransform=flat_ax_transform,
                    ReadoutColumn=readout_col,
                )
                table_workspaces.append(ws_str)
        else:
            for iws, ws in enumerate(wss):
                default_param_vals = "_default_param_table"
                self.create_default_parameter_table_with_value(ws, iws + 1, default_param_vals)
                ws_str = f"_{iws}_abi_table"
                CreatePoleFigureTableWorkspace(
                    InputWorkspace=ws,
                    PeakParameterWorkspace=default_param_vals,
                    OutputWorkspace=ws_str,
                    Reflection=hkl,
                    Chi2Threshold=chi2_thresh,
                    PeakPositionThreshold=peak_thresh,
                    ApplyScatteringPowerCorrection=inc_scatt_corr,
                    ScatteringVolumePosition=scat_vol_pos,
                    AxesTransform=flat_ax_transform,
                )
                table_workspaces.append(ws_str)
        CloneWorkspace(InputWorkspace=table_workspaces[0], OutputWorkspace=out_ws)
        for tw in table_workspaces[1:]:
            CombineTableWorkspaces(LHSWorkspace=out_ws, RHSWorkspace=tw, OutputWorkspace=out_ws)
        self._save_files(out_ws, save_dirs)

    @staticmethod
    def get_pole_figure_data(ws_name: str, projection: str, readout_col: str = "I"):
        if projection.lower() == "stereographic":
            proj = ster_proj
        else:
            proj = azim_proj
        ws = ADS.retrieve(ws_name)
        alphas = np.asarray(ws.column("Alpha"))
        betas = np.asarray(ws.column("Beta"))
        i = np.asarray(ws.column(readout_col))
        return proj(alphas, betas, i)

    @staticmethod
    def get_pf_table_name(
        wss: Sequence[str], fit_params: Sequence[str], hkl: Optional[Sequence[int]], readout_column: str
    ) -> Tuple[str, str]:
        fws, lws = ADS.retrieve(wss[0]), ADS.retrieve(wss[-1])
        try:
            run_range = f"{fws.getRun().getLogData('run_number').value}-{lws.getRun().getLogData('run_number').value}"
            instr = fws.getInstrument().getName()
        except RuntimeError:
            instr = "UNKNOWN"
            run_range = "XXXXXX-XXXXXX"
        try:
            grouping = fws.getRun().getLogData("Grouping").value
        except RuntimeError:
            grouping = "GROUP"
        try:
            # try and get a peak reference either from hkl or from the X0 column of param
            peak = "".join([str(ind) for ind in hkl]) if hkl else str(np.round(np.mean(ADS.retrieve(fit_params[0]).column("X0")), 2))
            table_name = f"{peak}_{instr}_{run_range}_{grouping}_pf_table_{readout_column}"
        except Exception:
            # if no param table given, no peak reference
            table_name = f"{instr}_{run_range}_{grouping}_pf_table_{readout_column}"
        return table_name, grouping

    # ~~~~~ Pole Figure Plotting functions ~~~~~~~~

    def plot_pole_figure(
        self,
        ws_name: str,
        projection: str,
        fig: Optional[Figure] = None,
        readout_col: str = "I",
        save_dirs: Optional[Sequence[str] | str] = None,
        plot_exp: bool = True,
        ax_labels: Sequence[str] = ("Dir1", "Dir2"),
        contour_kernel: Optional[float] = 2.0,
        **kwargs,
    ) -> Figure:
        pfi = self.get_pole_figure_data(ws_name, projection, readout_col)

        if plot_exp:
            suffix = "scatter"
            fig, ax = self.plot_exp_pf(pfi, ax_labels, readout_col, fig, **kwargs)
        else:
            suffix = f"contour_{contour_kernel}"
            fig, ax = self.plot_contour_pf(pfi, ax_labels, readout_col, fig, contour_kernel, **kwargs)
        if save_dirs:
            for save_dir in save_dirs:
                fig.savefig(str(path.join(save_dir, ws_name + f"_{suffix}.png")))

        return fig, ax

    @staticmethod
    def plot_exp_pf(pfi: np.ndarray, ax_labels: Sequence[str], column_label: str, fig: Optional[Figure] = None, **kwargs) -> [Figure, Axes]:
        u = np.linspace(0, 2 * np.pi, 100)
        x = np.cos(u)
        y = np.sin(u)
        z = np.zeros_like(x)
        eq = np.concatenate((x[None, :], y[None, :], z[None, :]), axis=0)

        fig = plt.figure(layout="constrained") if not fig else fig
        gs = fig.add_gridspec(
            1,
            3,
            width_ratios=[1, 30, 1],  # tweak 30 to control plot vs cbar width
            left=0.05,
            right=0.98,
            top=0.98,
            bottom=0.06,
            wspace=0.05,
        )
        # left spacer: gs[0, 0] (we don't use it)
        ax = fig.add_subplot(gs[0, 1])
        cax = fig.add_subplot(gs[0, 2])
        scat_plot = ax.scatter(pfi[:, 1], pfi[:, 0], c=pfi[:, 2], s=20, cmap="jet", label="poles", **kwargs)
        ax.plot(eq[0], eq[1], c="grey", label="plot bounding circle")
        ax.set_aspect("equal")
        ax.set_axis_off()
        ax.quiver(-1, -1, 0.2, 0, color="blue", scale=1)
        ax.quiver(-1, -1, 0, 0.2, color="red", scale=1)
        ax.text(-0.8, -0.95, ax_labels[-1], fontsize=10)
        ax.text(-0.95, -0.8, ax_labels[0], fontsize=10)
        cbar = fig.colorbar(scat_plot, cax=cax)
        ax.set_label("Pole Figure Plot")
        scat_plot.set_label("pole figure data")
        cbar.set_label(column_label, rotation=0, labelpad=15)
        return fig, ax

    @staticmethod
    def plot_contour_pf(
        pfi: np.ndarray, ax_labels: Sequence[str], column_label: str, fig: Optional[Figure] = None, contour_kernel: float = 2.0, **kwargs
    ) -> [Figure, Axes]:
        x, y, z = pfi[:, 1], pfi[:, 0], np.nan_to_num(pfi[:, 2])
        # Grid definition
        R = 1
        grid_x, grid_y = np.mgrid[-R:R:200j, -R:R:200j]

        # Mask to keep only points inside the circle of radius R
        mask = grid_x**2 + grid_y**2 <= R**2

        # Interpolate z-values on the grid
        grid_z = np.asarray(griddata((x, y), z, (grid_x, grid_y), method="nearest"))
        grid_z = np.asarray(gaussian_filter(grid_z, sigma=contour_kernel))

        # Apply the mask
        grid_z[~mask] = np.nan

        # Plotting
        fig = plt.figure() if not fig else fig
        gs = fig.add_gridspec(
            1,
            3,
            width_ratios=[1, 30, 1],  # tweak 30 to control plot vs cbar width
            left=0.05,
            right=0.98,
            top=0.98,
            bottom=0.06,
            wspace=0.05,
        )
        # left spacer: gs[0, 0] (we don't use it)
        ax = fig.add_subplot(gs[0, 1])
        cax = fig.add_subplot(gs[0, 2])
        contour_plot = ax.contourf(grid_x, grid_y, grid_z, levels=10, cmap="jet", **kwargs)
        circle = plt.Circle((0, 0), R, color="grey", fill=False, linestyle="-", label="plot bounding circle")
        ax.add_patch(circle)
        ax.set_aspect("equal")
        ax.set_axis_off()
        ax.quiver(-1, -1, 0.2, 0, color="blue", scale=1)
        ax.quiver(-1, -1, 0, 0.2, color="red", scale=1)
        ax.text(-0.8, -0.95, ax_labels[-1], fontsize=10)
        ax.text(-0.95, -0.8, ax_labels[0], fontsize=10)
        ax.set_label("Pole Figure Plot")
        contour_plot.set_label("pole figure data")
        cbar = fig.colorbar(contour_plot, cax=cax)
        cbar.set_label(column_label, rotation=0, labelpad=15)
        return fig, ax

    # ~~~~~ General Utility functions ~~~~~~~~

    @staticmethod
    def get_save_dirs(root_dir: str, dir_name: str, rb_num: Optional[str], grouping: Optional[str] = "GROUP") -> Sequence[str]:
        save_dirs = [path.join(root_dir, dir_name)]
        if rb_num:
            save_dirs.append(path.join(root_dir, "User", rb_num, dir_name, grouping))
        for save_dir in save_dirs:
            if not path.exists(save_dir):
                makedirs(save_dir)
        return save_dirs

    @staticmethod
    def _save_files(ws: str, save_dirs: Sequence[str]) -> None:
        for save_dir in save_dirs:
            SaveNexus(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".nxs"))
            SaveAscii(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".txt"), Separator="Tab")

    def get_ws_info(self, ws_name: str, parameter_file: str, select: bool = True) -> dict:
        return {
            "shape": "Not set" if self._has_no_valid_shape(ws_name) else "set",
            "fit_parameters": parameter_file,
            "crystal": "Not set" if not self._has_xtal(ws_name) else self._get_xtal_info(ws_name),
            "select": select,
        }

    @staticmethod
    def _has_no_valid_shape(ws_name: str):
        return not has_valid_shape(ws_name)

    # ~~~~~ Parameter Table helper functions ~~~~~~~~

    @staticmethod
    def check_param_ws_for_columns(wss: Sequence[str]) -> Tuple[bool, bool]:
        has_chi2, has_x0 = True, True
        for ws in wss:
            param_ws = ADS.retrieve(ws)
            column_names = param_ws.getColumnNames()
            # check if any param ws are missing chi2 or x0
            if "chi2" not in column_names:
                has_chi2 = False
            if "X0" not in column_names:
                has_x0 = False
            return has_chi2, has_x0

    @staticmethod
    def create_default_parameter_table_with_value(ws_name: str, val: float, out_ws: str):
        tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)
        tab.addColumn("float", "I")
        ws = ADS.retrieve(ws_name)
        for _ in range(ws.getNumberHistograms()):
            tab.addRow(
                [
                    float(val),
                ]
            )

    @staticmethod
    def read_param_cols(ws_name: str, target_default: str = "I") -> Tuple[str, int]:
        ws = ADS.retrieve(ws_name)
        col_names = ws.getColumnNames()
        col_types = ws.columnTypes()
        col_names = [c for i, c in enumerate(col_names) if col_types[i] in ("double", "int", "float", "bool")]
        index = col_names.index(target_default) if target_default in col_names else 0
        return col_names, index

    # ~~~~~ Crystal Structure helper functions ~~~~~~~~

    @staticmethod
    def parse_hkl(H, K, L):
        try:
            return [int(H), int(K), int(L)]
        except Exception:
            return None

    @staticmethod
    def _has_xtal(ws: str) -> bool:
        return ADS.retrieve(ws).sample().hasCrystalStructure()

    @staticmethod
    def _get_xtal_info(ws: str) -> str:
        xtal = ADS.retrieve(ws).sample().getCrystalStructure()
        sg = xtal.getSpaceGroup().getHMSymbol()  # get the HM symbol
        scatts = ", ".join([scat.split(" ")[0] for scat in xtal.getScatterers()])
        return f"{scatts}: {sg}"

    @staticmethod
    def set_ws_xtal(ws: str, lattice: str, space_group: str, basis: str, cif: str) -> None:
        ws = ADS.retrieve(ws)
        has_cif = cif != ""
        has_latt_prop = lattice != "" and space_group != "" and basis != ""

        input = "cif" if has_cif else "string" if has_latt_prop else None
        args = (cif,) if has_cif else (lattice, space_group, basis) if has_latt_prop else None

        xtal = get_xtal_structure(input, *args)
        ws.sample().setCrystalStructure(xtal)
        logger.notice("Crystal Structure Set")

    def set_all_ws_xtal(self, wss: Sequence[str], lattice: str, space_group: str, basis: str, cif: str) -> None:
        for ws in wss:
            self.set_ws_xtal(ws, lattice, space_group, basis, cif)


def ster_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    betas = np.pi - betas  # this formula projects onto the north-pole, and beta is taken from the south
    r = np.sin(betas) / (1 - np.cos(betas))
    out = np.zeros((len(alphas), 3))
    out[:, 0] = r * np.cos(alphas)
    out[:, 1] = r * np.sin(alphas)
    out[:, 2] = i
    return out


def azim_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    betas = betas / (np.pi / 2)
    xs = (betas * np.cos(alphas))[:, None]
    zs = (betas * np.sin(alphas))[:, None]
    out = np.concatenate([xs, zs, i[:, None]], axis=1)
    return out
