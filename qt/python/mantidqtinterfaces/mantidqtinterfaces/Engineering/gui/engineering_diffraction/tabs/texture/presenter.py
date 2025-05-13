from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger
from mantid.simpleapi import Load
from qtpy.QtCore import QTimer
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings

import os


class TexturePresenter:
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None

        self.ws_names = []
        self.fit_param_files = []
        self.ws_info = {}

        self.correction_notifier = GenericObservable()

        self.view.set_on_load_ws_clicked(self.load_ws_files)
        self.view.set_on_load_param_clicked(self.load_param_files)

        self.view.set_on_select_all_clicked(self.view.select_all_workspaces)
        self.view.set_on_delete_clicked(self.delete_selected_files)

        self.view.set_on_check_inc_scatt_corr_state_changed(self.view.update_crystal_section_visibility)
        self.view.set_include_scatter_corr(False)
        self.view.set_crystal_section_visibility(False)
        self.view.set_on_set_crystal_clicked(self.on_set_crystal_clicked)
        self.view.set_on_set_all_crystal_clicked(self.on_set_all_crystal_clicked)

        self.view.set_on_calc_pf_clicked(self.on_calc_pf_clicked)

    def load_ws_files(self):
        filenames = self.view.finder_texture_ws.getFilenames()

        for path in filenames:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            try:
                Load(Filename=path, OutputWorkspace=ws_name)
            except Exception as e:
                logger.warning(f"Failed to load {path}: {e}")
                continue
            if ws_name not in self.ws_names:
                self.ws_names.append(ws_name)

        self.redraw_table()

    def load_param_files(self):
        filenames = self.view.finder_texture_tables.getFilenames()

        for path in filenames:
            param_file_name = os.path.splitext(os.path.basename(path))[0]
            try:
                Load(Filename=path, OutputWorkspace=param_file_name)
            except Exception as e:
                logger.warning(f"Failed to load {path}: {e}")
                continue
            if param_file_name not in self.fit_param_files:
                self.fit_param_files.append(param_file_name)

        self.redraw_table()

    def delete_selected_files(self):
        selected_wss, selected_params = self.view.get_selected_workspaces()
        for ws in selected_wss:
            self.ws_names.pop(self.ws_names.index(ws))
        for param in selected_params:
            if param != "Not set":
                self.fit_param_files.pop(self.fit_param_files.index(param))
        self.redraw_table()

    def redraw_table(self):
        self.update_ws_info()
        self.view.populate_workspace_table(self.ws_info)
        self.view.populate_workspace_list(self.ws_names)

    def update_ws_info(self):
        ws_info = {}
        selected_ws, _ = self.view.get_selected_workspaces()
        for pos_ind, ws_name in enumerate(self.ws_names):
            param_status = self.fit_param_files[pos_ind] if pos_ind < len(self.fit_param_files) else "Not set"
            ws_info[ws_name] = self.model.get_ws_info(ws_name, param_status, ws_name in selected_ws)  # maintain state of selected boxes
        self.ws_info = ws_info

    def on_set_crystal_clicked(self):
        self.model.set_ws_xtal(self.view.get_crystal_ws(), self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis())

    def on_set_all_crystal_clicked(self):
        wss, _ = self.view.get_selected_workspaces()
        self.model.set_all_ws_xtal(wss, self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis())

    def on_calc_pf_clicked(self):
        wss, params = self.view.get_selected_workspaces()
        # remove any 'not set' parameters workspaces from the list
        params = [p for p in params if p != "Not set"]
        hkl = self.model._parse_hkl(*self.view.get_hkl())
        inc_scatt = self.view.get_inc_scatt_power()
        out_ws = self.model.get_pf_table_name(wss, params, hkl)

        # default for now
        scat_vol_pos = (0.0, 0.0, 0.0)
        chi2_thresh = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "cost_func_thresh")
        peak_thresh = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "peak_pos_thresh")

        self.worker = AsyncTask(
            self._calc_pf,
            (wss, params, out_ws, hkl, inc_scatt, scat_vol_pos, chi2_thresh, peak_thresh),
            error_cb=self._on_worker_error,
            finished_cb=self._on_worker_success,
        )
        self.worker.start()

    def _calc_pf(self, wss, params, out_ws, hkl, inc_scatt, scat_vol_pos, chi2_thresh, peak_thresh):
        self.model.make_pole_figure_tables(wss, params, out_ws, hkl, inc_scatt, scat_vol_pos, chi2_thresh, peak_thresh)
        self.plot_pf(out_ws, self.view.get_projection_method())

    def _copy_sample_to_all_selected(self):
        ref_ws = self.view.get_sample_reference_ws()
        wss = self.view.get_selected_workspaces()
        self.model.copy_sample_info(ref_ws, wss)
        self.redraw_table()

    def _on_worker_success(self):
        self.correction_notifier.notify_subscribers("Corrections Applied")

    def _on_worker_error(self, error_info):
        logger.error(str(error_info))

    def _redraw_on_alg_exec(self):
        QTimer.singleShot(200, self.redraw_table)

    def plot_pf(self, pf_ws, proj):
        # Get figure and canvas from view
        fig, canvas = self.view.get_plot_axis()

        # Clear existing figure
        fig.clf()

        self.model.plot_pole_figure(pf_ws, proj, fig=fig)

        canvas.draw()
