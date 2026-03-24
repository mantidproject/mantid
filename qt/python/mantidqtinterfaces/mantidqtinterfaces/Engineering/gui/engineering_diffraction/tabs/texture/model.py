# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from Engineering.texture.polefigure.polefigure_model import TextureProjection
from Engineering.texture.texture_helper import ster_proj, azim_proj
from mantid.kernel import logger
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import Load
import os


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

    def load_files(self, filenames):
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

    def exec_make_pf_tables(self, wss, params, save_dirs):
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

    def exec_plot_pf(self, fig, save_dirs):
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

    def has_latt(self, latt, spacegroup, basis):
        return latt != "" and spacegroup != "" and basis != ""

    def has_any_latt(self, latt, spacegroup, basis):
        return latt != "" or spacegroup != "" or basis != ""

    def has_cif(self, cif):
        return cif != ""

    def has_xtal_and_ws(self, latt, spacegroup, basis, cif, ws):
        return (self.has_latt(latt, spacegroup, basis) or self.has_cif(cif)) and ws != ""

    def can_set_all_crystal(self, has_xtal_and_ws, a_selected_ws):
        return has_xtal_and_ws and a_selected_ws

    # table/ selected ws logic

    @staticmethod
    def has_selected_wss(selected_wss):
        return len(selected_wss) > 0

    @staticmethod
    def at_least_one_param_assigned(assigned_params):
        return len(assigned_params) > 0

    @staticmethod
    def all_wss_have_params(selected_wss, selected_params):
        valid_params = [p for p in selected_params if p != "Not set"]
        return len(selected_wss) == len(valid_params) and len(selected_wss) > 0

    @staticmethod
    def assign_unpaired_wss_and_params(unassigned_wss, unassigned_params, ws_assignments, param_assignments):
        while (len(unassigned_params) > 0) and (len(unassigned_wss) > 0):
            ws = unassigned_wss.pop(0)
            param = unassigned_params.pop(0)
            ws_assignments[ws] = param
            param_assignments[param] = ws
        # don't want to keep an unseen stack of unassigned parameter files
        unassigned_params = []
        return unassigned_wss, unassigned_params, ws_assignments, param_assignments

    @staticmethod
    def get_param_from_ws(ws_name, ws_assignments):
        return ws_assignments[ws_name] if ws_name in ws_assignments.keys() else "Not set"

    @staticmethod
    def has_at_least_one_col(cols):
        return len(cols) > 0

    @staticmethod
    def get_last_directory(filepaths):
        directories = set()
        directory = None
        for filepath in filepaths:
            directory, discard = os.path.split(filepath)
            directories.add(directory)
        if len(directories) == 1:
            return directory

    # setters

    def set_out_ws_and_grouping(self, wss, params):
        self.out_ws, self.combined_ws, self.grouping = self.get_pf_output_names(wss, params, self.hkl, self.readout_col)

    def set_hkl(self, hkl):
        self.hkl = self.parse_hkl(*hkl) if self.inc_scatt else None

    def set_projection_method(self, proj_method):
        self.projection_method = proj_method

    def set_inc_scatt(self, inc_scatt):
        self.inc_scatt = inc_scatt

    def set_scat_vol_pos(self, scat_vol_pos):
        self.scat_vol_pos = scat_vol_pos

    def set_chi2_thresh(self, chi2_thresh):
        self.chi2_thresh = chi2_thresh

    def set_peak_thresh(self, peak_thresh):
        self.peak_thresh = peak_thresh

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def set_ax_trans(self, ax_trans):
        self.ax_transform = ax_trans

    def set_ax_labels(self, ax_labels):
        self.ax_labels = ax_labels

    def set_readout_col(self, readout_col):
        self.readout_col = readout_col if readout_col != "" else "I"

    def set_plot_exp(self, plot_exp):
        self.plot_exp = plot_exp

    def set_contour_kernel(self, contour_kernel):
        self.contour_kernel = contour_kernel

    # getters

    def get_out_ws(self):
        return self.out_ws

    def get_combined_ws(self):
        return self.combined_ws

    def get_grouping(self):
        return self.grouping

    def get_hkl(self):
        return self.hkl

    def get_projection_method(self):
        return self.projection_method

    def get_inc_scatt(self):
        return self.inc_scatt

    def get_scat_vol_pos(self):
        return self.scat_vol_pos

    def get_chi2_thresh(self):
        return self.chi2_thresh

    def get_peak_thresh(self):
        return self.peak_thresh

    def get_rb_num(self):
        return self.rb_num

    def get_ax_trans(self):
        return self.ax_transform

    def get_ax_labels(self):
        return self.ax_labels

    def get_readout_col(self):
        return self.readout_col

    def get_plot_exp(self):
        return self.plot_exp

    def get_contour_kernel(self):
        return self.contour_kernel


ster_proj = ster_proj
azim_proj = azim_proj
