from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger
from mantid.simpleapi import Load
from mantidqt.interfacemanager import InterfaceManager
from qtpy.QtCore import QTimer

import os


class TextureCorrectionPresenter:
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None

        self.ws_names = []
        self.ws_info = {}

        self.correction_notifier = GenericObservable()

        self.view.set_on_load_clicked(self.load_files_into_table)
        self.view.set_on_load_orientation_clicked(self.load_all_orientations)
        self.view.set_on_select_all_clicked(self.view.select_all_workspaces)
        self.view.set_on_apply_clicked(self.on_apply_clicked)
        self.view.set_on_delete_clicked(self.delete_selected_files)
        self.view.set_on_set_orientation_clicked(self.open_goniometer_dialog)
        self.view.set_on_load_sample_shape_clicked(self.open_load_sample_shape_dialog)
        self.view.set_on_set_sample_shape_clicked(self.open_set_sample_shape_dialog)
        self.view.set_on_set_material_clicked(self.open_set_material_dialog)
        self.view.set_on_check_inc_abs_corr_state_changed(self.view.update_absorption_section_visibility)
        self.view.set_on_check_inc_div_corr_state_changed(self.view.update_divergence_section_visibility)
        self.view.set_on_copy_sample_clicked(self._copy_sample_to_all_selected)

        self.view.set_include_divergence(False)
        self.view.update_divergence_section_visibility()

    def load_files_into_table(self):
        filenames = self.view.finder_corr.getFilenames()

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

    def delete_selected_files(self):
        wss = self.view.get_selected_workspaces()
        for ws in wss:
            self.ws_names.pop(self.ws_names.index(ws))
        self.redraw_table()

    def redraw_table(self):
        self.update_ws_info()
        self.view.populate_workspace_table(self.ws_info)
        self.view.populate_workspace_list()

    def update_ws_info(self):
        ws_info = {}
        selected = self.view.get_selected_workspaces()
        for ws_name in self.ws_names:
            ws_info[ws_name] = self.model.get_ws_info(ws_name, ws_name in selected)  # maintain state of selected boxes
        self.ws_info = ws_info

    def load_all_orientations(self):
        wss = self.view.get_selected_workspaces()
        orientation_file = self.view.get_orientation_file()
        self.model.load_all_orientations(wss, orientation_file)
        self.redraw_table()

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
