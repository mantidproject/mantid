from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import Load
from mantidqt.interfacemanager import InterfaceManager
from qtpy.QtCore import QTimer
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import (
    INSTRUMENT_DICT,
    CalibrationObserver,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

import os
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_presenter import ShowSamplePresenter


class TextureCorrectionPresenter:
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None

        self.show_sample_presenter = ShowSamplePresenter(model, view, True)

        self.ws_names = []
        self.ws_info = {}

        self.calibration_observer = CalibrationObserver(self)
        self.current_calibration = CalibrationInfo()
        self.instrument = "ENGINX"
        self.rb_num = None

        self.correction_notifier = GenericObservable()

        self.view.set_on_load_clicked(self.load_files_into_table)
        self.view.set_on_load_orientation_clicked(self.load_all_orientations)
        self.view.set_on_select_all_clicked(self.select_all)
        self.view.set_on_deselect_all_clicked(self.deselect_all)
        self.view.set_on_apply_clicked(self.on_apply_clicked)
        self.view.set_on_delete_clicked(self.delete_selected_files)

        self.view.set_on_create_ref_ws_clicked(self.on_create_ref_sample_clicked)
        self.view.set_on_set_ref_ws_orientation_clicked(self.open_goniometer_dialog)
        self.view.set_on_save_ref_clicked(self._on_save_ref_clicked)
        self.view.set_on_view_reference_shape_clicked(self.show_sample_presenter.on_view_reference_shape_clicked)
        self.view.set_on_load_ref_clicked(self._on_load_ref_clicked)

        self.view.set_on_set_orientation_clicked(self.open_goniometer_dialog)
        self.view.set_on_load_sample_shape_clicked(self.open_load_sample_shape_dialog)
        self.view.set_on_set_sample_shape_clicked(self.open_set_sample_shape_dialog)
        self.view.set_on_set_material_clicked(self.open_set_material_dialog)
        self.view.set_on_check_inc_abs_corr_state_changed(self.view.update_absorption_section_visibility)
        self.view.set_on_check_inc_div_corr_state_changed(self.view.update_divergence_section_visibility)
        self.view.set_on_copy_sample_clicked(self._copy_sample_to_all_selected)
        self.view.set_on_copy_ref_sample_clicked(self._copy_ref_sample_to_all_selected)

        self.view.set_include_divergence(False)
        self.view.update_divergence_section_visibility()

        self.view.set_on_gauge_vol_state_changed(self.update_custom_shape_finder_vis)

        self.view.set_on_check_att_tab_state_changed(self.view.update_atten_tab_section_visibility)
        self.view.set_include_atten_tab(False)
        self.view.update_atten_tab_section_visibility()

        self.update_custom_shape_finder_vis()
        self.update_reference_info()

    def load_files_into_table(self):
        filenames = self.view.finder_corr.getFilenames()

        for path in filenames:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            if ADS.doesExist(ws_name):
                logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
            else:
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
        use_euler = self._get_setting("use_euler_angles", bool)
        euler_scheme = self._get_setting("euler_angles_scheme")
        euler_sense = self._get_setting("euler_angles_sense")
        self.model.load_all_orientations(wss, orientation_file, use_euler, euler_scheme, euler_sense)
        self.redraw_table()

    def select_all(self):
        self.view.set_all_workspaces_selected(True)

    def deselect_all(self):
        self.view.set_all_workspaces_selected(False)

    def _add_gauge_vol_view(self, fig):
        if self.view.include_absorption():
            self.model.plot_gauge_vol(self.view.get_shape_method(), self.view.get_custom_shape(), fig)

    def _on_save_ref_clicked(self):
        self.model.save_reference_file(self.rb_num, self.current_calibration, output_settings.get_output_path())

    def _on_load_ref_clicked(self):
        path = self.view.get_reference_file()
        if path:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            if ADS.doesExist(ws_name):
                logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
                self.model.set_reference_ws(ws_name)
            else:
                try:
                    Load(Filename=path, OutputWorkspace=ws_name)
                    self.model.set_reference_ws(ws_name)
                except Exception as e:
                    logger.warning(f"Failed to load {path}: {e}")
            self.update_reference_info()

    def on_apply_clicked(self):
        wss = self.view.get_selected_workspaces()
        out_wss = [f"Corrected_{ws}" for ws in wss]

        self.worker = AsyncTask(
            self._apply_all_corrections, (wss, out_wss), error_cb=self._on_worker_error, finished_cb=self._on_worker_success
        )
        self.worker.start()

    def _apply_all_corrections(self, wss, out_wss):
        include_atten_table = self.view.include_atten_tab()

        root_dir = output_settings.get_output_path()

        for i, ws in enumerate(wss):
            abs_corr = 1.0
            div_corr = 1.0

            if self.view.include_absorption():
                mc_param_str = self._get_setting("monte_carlo_params")

                self.model.define_gauge_volume(ws, self.view.get_shape_method(), self.view.get_custom_shape())
                self.model.calc_absorption(ws, mc_param_str)
                abs_corr = "_abs_corr"
                if include_atten_table:
                    atten_vals = self.model.read_attenuation_coefficient_at_value(
                        abs_corr, self.view.get_evaluation_value(), self.view.get_evaluation_units()
                    )
                    self.model.write_atten_val_table(
                        ws,
                        atten_vals,
                        self.view.get_evaluation_value(),
                        self.view.get_evaluation_units(),
                        self.rb_num,
                        self.current_calibration,
                        root_dir,
                    )

            if self.view.include_divergence():
                self.model.calc_divergence(ws, self.view.get_div_horz(), self.view.get_div_vert(), self.view.get_div_det_horz())
                div_corr = "_div_corr"

            remove_ws_after_processing = self._get_setting("clear_absorption_ws_after_processing", bool)
            self.model.apply_corrections(
                ws, out_wss[i], self.current_calibration.group, root_dir, abs_corr, div_corr, self.rb_num, remove_ws_after_processing
            )

    def _copy_sample_to_all_selected(self):
        ref_ws = self.view.get_sample_reference_ws()
        wss = self.view.get_selected_workspaces()
        self.model.copy_sample_info(ref_ws, wss)
        self.redraw_table()

    def _copy_ref_sample_to_all_selected(self):
        ref_ws = self.model.reference_ws
        if ref_ws:
            wss = self.view.get_selected_workspaces()
            self.model.copy_sample_info(ref_ws, wss, True)
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
        QTimer.singleShot(200, self.update_reference_info)

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def update_calibration(self, calibration):
        """
        Update the current calibration following a call from a CalibrationNotifier
        :param calibration: The new current calibration.
        """
        self.current_calibration = calibration

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def update_custom_shape_finder_vis(self):
        if self.view.get_shape_method() == "Custom Shape":
            self.view.finder_gauge_vol.setVisible(True)
        else:
            self.view.finder_gauge_vol.setVisible(False)

    def on_create_ref_sample_clicked(self):
        self.model.create_reference_ws(self.rb_num, self.instrument)
        self.update_reference_info()

    def update_reference_info(self):
        self.view.update_reference_info_section(*self.model.get_reference_info())

    def _get_setting(self, setting_name, return_type=str):
        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, setting_name, return_type)
