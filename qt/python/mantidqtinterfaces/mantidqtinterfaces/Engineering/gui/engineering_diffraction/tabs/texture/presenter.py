from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger
from mantid.simpleapi import Load
from mantidqt.interfacemanager import InterfaceManager
from qtpy.QtCore import QTimer

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
            self.fit_param_files.pop(self.fit_param_files.index(param))
        self.redraw_table()

    def redraw_table(self):
        self.update_ws_info()
        self.view.populate_workspace_table(self.ws_info)
        self.view.populate_workspace_list()

    def update_ws_info(self):
        ws_info = {}
        selected = self.view.get_selected_workspaces()
        for pos_ind, ws_name in enumerate(self.ws_names):
            param_status = self.fit_param_files[pos_ind] if pos_ind < len(self.fit_param_files) else "Not set"
            ws_info[ws_name] = self.model.get_ws_info(ws_name, param_status, ws_name in selected)  # maintain state of selected boxes
        self.ws_info = ws_info

    def on_apply_clicked(self):
        wss = self.view.get_selected_workspaces()
        out_wss = [f"Corrected_{ws}" for ws in wss]

        self.worker = AsyncTask(
            self._apply_all_corrections, (wss, out_wss), error_cb=self._on_worker_error, finished_cb=self._on_worker_success
        )
        self.worker.start()

    def _apply_all_corrections(self, wss, out_wss):
        for i, ws in enumerate(wss):
            abs_corr = 1.0
            div_corr = 1.0

            if self.view.include_absorption():
                self.model.define_gauge_volume(ws, self.view.get_shape_method(), self.view.get_custom_shape())
                self.model.calc_absorption(ws)
                abs_corr = "_abs_corr"

            if self.view.include_divergence():
                self.model.calc_divergence(ws, self.view.get_div_horz(), self.view.get_div_vert(), self.view.get_div_det_horz())
                div_corr = "_div_corr"

            self.model.apply_corrections(ws, out_wss[i], abs_corr, div_corr)

    def _copy_sample_to_all_selected(self):
        ref_ws = self.view.get_sample_reference_ws()
        wss = self.view.get_selected_workspaces()
        self.model.copy_sample_info(ref_ws, wss)
        self.redraw_table()

    def _on_worker_success(self):
        self.correction_notifier.notify_subscribers("Corrections Applied")

    def _on_worker_error(self, error_info):
        logger.error(str(error_info))

    def open_goniometer_dialog(self):
        self._open_alg_dialog("SetGoniometer")

    def open_load_sample_shape_dialog(self):
        self._open_alg_dialog("LoadSampleShape")

    def open_set_sample_shape_dialog(self):
        self._open_alg_dialog("SetSampleShape")

    def open_set_material_dialog(self):
        self._open_alg_dialog("SetSampleMaterial")

    def _open_alg_dialog(self, alg_str):
        manager = InterfaceManager()
        dialog = manager.createDialogFromName(alg_str, -1)
        if dialog is not None:
            dialog.finished.connect(self._redraw_on_alg_exec)
            dialog.open()

    def _redraw_on_alg_exec(self):
        QTimer.singleShot(200, self.redraw_table)
