# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from typing import Sequence, List, Dict, Tuple, TypeAlias
from matplotlib.figure import Figure
from Engineering.texture.polefigure.polefigure_model import TextureProjection
from Engineering.texture.texture_helper import ster_proj, azim_proj
from mantid.kernel import logger
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import Load
import os
from numpy import ndarray

HKL_Type: TypeAlias = Tuple[int, int, int] | Tuple[str, str, str]


class ProjectionModel(TextureProjection):
    def __init__(self):
        self.out_ws = None
        self.hkl = None
        self.projection_method = None
        self.inc_scatt = None
        self.scat_vol_pos = None
        self.chi2_thresh = None
        self.peak_thresh = None
        self.rb_num = None
        self.ax_transform = None
        self.ax_labels = None
        self.readout_col = "I"
        self.grouping = None
        self.plot_exp = None
        self.contour_kernel = None
        self.combined_ws = None

    # loading logic

    def load_files(self, filenames: Sequence[str]) -> List[str]:
        ws_names = []
        for path in filenames:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            ws_names.append(ws_name)
            if ADS.doesExist(ws_name):
                logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
                continue
            try:
                Load(Filename=path, OutputWorkspace=ws_name)
            except Exception as e:
                logger.warning(f"Failed to load {path}: {e}")
        return ws_names

    # pf logic

    def exec_make_pf_tables(self, wss: List[str], params: List[str], save_dirs: List[str]) -> None:
        self.make_pole_figure_tables(
            wss,
            params,
            self.get_out_ws(),
            self.get_combined_ws(),
            self.get_hkl(),
            self.get_inc_scatt(),
            self.get_scat_vol_pos(),
            self.get_chi2_thresh(),
            self.get_peak_thresh(),
            save_dirs,
            self.get_ax_trans(),
            self.get_readout_col(),
        )

    def exec_plot_pf(self, fig: Figure, save_dirs: List[str]):
        self.plot_pole_figure(
            self.get_out_ws(),
            self.get_projection_method(),
            fig=fig,
            readout_col=self.get_readout_col(),
            save_dirs=save_dirs,
            plot_exp=self.get_plot_exp(),
            ax_labels=self.get_ax_labels(),
            contour_kernel=self.get_contour_kernel(),
        )

    # xtal logic

    def has_latt(self, latt: str, spacegroup: str, basis: str) -> bool:
        return latt != "" and spacegroup != "" and basis != ""

    def has_any_latt(self, latt: str, spacegroup: str, basis: str) -> bool:
        return latt != "" or spacegroup != "" or basis != ""

    def has_cif(self, cif: str) -> bool:
        return cif != ""

    def has_xtal_and_ws(self, latt: str, spacegroup: str, basis: str, cif: str, ws: str) -> bool:
        return (self.has_latt(latt, spacegroup, basis) or self.has_cif(cif)) and ws != ""

    def can_set_all_crystal(self, has_xtal_and_ws: bool, a_selected_ws: bool) -> bool:
        return has_xtal_and_ws and a_selected_ws

    # table/ selected ws logic

    @staticmethod
    def has_selected_wss(selected_wss: List[str]) -> bool:
        return len(selected_wss) > 0

    @staticmethod
    def at_least_one_param_assigned(assigned_params: List[str]) -> bool:
        return len(assigned_params) > 0

    @staticmethod
    def all_wss_have_params(selected_wss: List[str], selected_params: List[str]) -> bool:
        valid_params = [p for p in selected_params if p != "Not set"]
        return len(selected_wss) == len(valid_params) and len(selected_wss) > 0

    @staticmethod
    def assign_unpaired_wss_and_params(
        unassigned_wss: List[str], unassigned_params: List[str], ws_assignments: Dict[str, str], param_assignments: Dict[str, str]
    ) -> Tuple[List[str], List[str], Dict[str, str], Dict[str, str]]:
        while (len(unassigned_params) > 0) and (len(unassigned_wss) > 0):
            ws = unassigned_wss.pop(0)
            param = unassigned_params.pop(0)
            ws_assignments[ws] = param
            param_assignments[param] = ws
        # don't want to keep an unseen stack of unassigned parameter files
        unassigned_params = []
        return unassigned_wss, unassigned_params, ws_assignments, param_assignments

    @staticmethod
    def get_param_from_ws(ws_name: str, ws_assignments: Dict[str, str]) -> str:
        return ws_assignments[ws_name] if ws_name in ws_assignments.keys() else "Not set"

    @staticmethod
    def has_at_least_one_col(cols: Sequence[str]) -> bool:
        return len(cols) > 0

    @staticmethod
    def get_last_directory(filepaths: Sequence[str]) -> str | None:
        directories = set()
        directory = None
        for filepath in filepaths:
            directory, discard = os.path.split(filepath)
            directories.add(directory)
        if len(directories) == 1:
            return directory

    # setters

    def set_out_ws_and_grouping(self, wss: List[str], params: List[str]) -> None:
        self.out_ws, self.combined_ws, self.grouping = self.get_pf_output_names(wss, params, self.hkl, self.readout_col)

    def set_hkl(self, hkl: HKL_Type | None) -> None:
        self.hkl = self.parse_hkl(*hkl) if self.inc_scatt else None

    def set_projection_method(self, proj_method: str) -> None:
        self.projection_method = proj_method

    def set_inc_scatt(self, inc_scatt: bool) -> None:
        self.inc_scatt = inc_scatt

    def set_scat_vol_pos(self, scat_vol_pos: Tuple[float, float, float] | ndarray) -> None:
        self.scat_vol_pos = scat_vol_pos

    def set_chi2_thresh(self, chi2_thresh: float | int) -> None:
        self.chi2_thresh = chi2_thresh

    def set_peak_thresh(self, peak_thresh: float | int) -> None:
        self.peak_thresh = peak_thresh

    def set_rb_num(self, rb_num: str) -> None:
        self.rb_num = rb_num

    def set_ax_trans(self, ax_trans: ndarray) -> None:
        self.ax_transform = ax_trans

    def set_ax_labels(self, ax_labels: Sequence[str]) -> None:
        self.ax_labels = ax_labels

    def set_readout_col(self, readout_col: str) -> None:
        self.readout_col = readout_col if readout_col != "" else "I"

    def set_plot_exp(self, plot_exp: bool) -> None:
        self.plot_exp = plot_exp

    def set_contour_kernel(self, contour_kernel: float | int) -> None:
        self.contour_kernel = contour_kernel

    # getters

    def get_out_ws(self) -> str:
        return self.out_ws

    def get_combined_ws(self) -> str:
        return self.combined_ws

    def get_grouping(self) -> str:
        return self.grouping

    def get_hkl(self) -> Tuple[int, int, int] | None:
        return self.hkl

    def get_projection_method(self) -> str:
        return self.projection_method

    def get_inc_scatt(self) -> bool:
        return self.inc_scatt

    def get_scat_vol_pos(self) -> ndarray:
        return self.scat_vol_pos

    def get_chi2_thresh(self) -> float | int:
        return self.chi2_thresh

    def get_peak_thresh(self) -> float | int:
        return self.peak_thresh

    def get_rb_num(self) -> str:
        return self.rb_num

    def get_ax_trans(self) -> ndarray:
        return self.ax_transform

    def get_ax_labels(self) -> Sequence[str]:
        return self.ax_labels

    def get_readout_col(self) -> str:
        return self.readout_col

    def get_plot_exp(self) -> bool:
        return self.plot_exp

    def get_contour_kernel(self) -> float | int:
        return self.contour_kernel


ster_proj = ster_proj
azim_proj = azim_proj
