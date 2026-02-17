# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    logger,
    SaveNexus,
    SaveAscii,
)
import numpy as np
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence, Tuple
from os import path, makedirs
from Engineering.common.texture_sample_viewer import has_valid_shape
from Engineering.texture.xtal_helper import get_xtal_structure
from Engineering.texture.texture_helper import plot_pole_figure, create_pole_figure_tables
from matplotlib.figure import Figure
from matplotlib.axes import Axes


class TextureProjection:
    # ~~~~~ Pole Figure Data functions ~~~~~~~~

    def make_pole_figure_tables(
        self,
        wss: Sequence[str],
        peak_wss: Optional[Sequence[str]],
        out_ws_name: str,
        combined_ws_name: Optional[str],
        hkl: Optional[Sequence[int]],
        inc_scatt_corr: bool,
        scat_vol_pos: Sequence[float],
        chi2_thresh: Optional[float],
        peak_thresh: Optional[float],
        save_dirs: Optional[Sequence[str]] = None,
        ax_transform: Sequence[float] = np.eye(3),
        readout_col: str = "",
        include_spec_info: bool = True,
        save_ascii: bool = False,
    ) -> None:
        create_pole_figure_tables(
            wss=wss,
            peak_wss=peak_wss,
            out_ws=out_ws_name,
            combined_ws=combined_ws_name,
            hkl=hkl,
            inc_scatt_corr=inc_scatt_corr,
            scat_vol_pos=scat_vol_pos,
            chi2_thresh=chi2_thresh,
            peak_thresh=peak_thresh,
            ax_transform=ax_transform,
            readout_col=readout_col,
            include_spec_info=include_spec_info,
        )
        self._save_files(out_ws_name, save_dirs, ascii=save_ascii)
        if combined_ws_name:
            self._save_files(combined_ws_name, save_dirs, ascii=save_ascii)

    @staticmethod
    def get_pf_output_names(
        wss: Sequence[str], fit_params: Sequence[str], hkl: Optional[Sequence[int]], readout_column: str
    ) -> Tuple[str, str, str]:
        fws, lws = ADS.retrieve(wss[0]), ADS.retrieve(wss[-1])
        readout_column = readout_column.replace("/", "_over_")
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
            combined_wsname = f"{peak}_{instr}_{run_range}_{grouping}_spectra"
        except Exception:
            # if no param table given, no peak reference
            table_name = f"{instr}_{run_range}_{grouping}_pf_table_{readout_column}"
            combined_wsname = f"{instr}_{run_range}_{grouping}_spectra"
        return table_name, combined_wsname, grouping

    # ~~~~~ Pole Figure Plotting functions ~~~~~~~~

    def plot_pole_figure(self, *args, **kwargs) -> [Figure, Axes]:
        return plot_pole_figure(*args, **kwargs)

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
    def _save_files(ws: str, save_dirs: Sequence[str], ascii=True) -> None:
        for save_dir in save_dirs:
            SaveNexus(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".nxs"))
            if ascii:
                try:
                    SaveAscii(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".txt"), Separator="Tab")
                except RuntimeError as e:
                    logger.warning(f"Failed to save {ws} as a txt file: " + str(e) + "Try Rebinning and calling SaveAscii manually")

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
    def read_param_cols(ws_name: str, target_default: str = "I") -> Tuple[Sequence[str], int]:
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
