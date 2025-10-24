# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from mantidqt.utils.asynchronous import AsyncTask
from mantid.api import AlgorithmObserver
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger
from mantidqt.interfacemanager import InterfaceManager
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import (
    INSTRUMENT_DICT,
    CalibrationObserver,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_presenter import ShowSamplePresenter

from functools import wraps


def redraws_table(func):
    @wraps(func)
    def wrapper(self):
        func(self)
        self.redraw_table()
        return

    return wrapper


class TextureCorrectionPresenter(AlgorithmObserver):
    def __init__(self, model, view):
        super(TextureCorrectionPresenter, self).__init__()
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
        self.view.set_on_set_ref_ws_orientation_clicked(self.open_ref_goniometer_dialog)
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

        self.view.alg_ui_finished.connect(self._on_alg_finished)

    @redraws_table
    def load_files_into_table(self):
        filenames = self.view.finder_corr.getFilenames()
        wss = self.model.load_files(filenames)
        for ws_name in wss:
            if ws_name not in self.ws_names:
                self.ws_names.append(ws_name)

    @redraws_table
    def delete_selected_files(self):
        wss = self.view.get_selected_workspaces()
        for ws in wss:
            self.ws_names.pop(self.ws_names.index(ws))

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

    @redraws_table
    def load_all_orientations(self):
        wss = self.view.get_selected_workspaces()
        orientation_file = self.view.get_orientation_file()
        use_euler = self._get_setting("use_euler_angles", bool)
        euler_scheme = self._get_setting("euler_angles_scheme")
        euler_sense = self._get_setting("euler_angles_sense")
        self.model.load_all_orientations(wss, orientation_file, use_euler, euler_scheme, euler_sense)

    def select_all(self):
        self.view.set_all_workspaces_selected(True)

    def deselect_all(self):
        self.view.set_all_workspaces_selected(False)

    def _on_save_ref_clicked(self):
        self.model.save_reference_file(self.rb_num, self.current_calibration, output_settings.get_output_path())

    def _on_load_ref_clicked(self):
        self.model.load_ref(self.view.get_reference_file())
        self.update_reference_info()

    def on_apply_clicked(self):
        wss = self.view.get_selected_workspaces()
        out_wss = self.model.get_out_ws_names(wss)

        self.worker = AsyncTask(
            self._apply_all_corrections, (wss, out_wss), error_cb=self._on_worker_error, finished_cb=self._on_worker_success
        )
        self.worker.start()

    def _apply_all_corrections(self, wss, out_wss):
        self.model.set_include_abs(self.view.include_absorption())
        self.model.set_include_atten(self.view.include_atten_tab())
        self.model.set_include_div(self.view.include_divergence())
        self.model.set_rb_num(self.rb_num)
        self.model.set_calibration(self.current_calibration)
        self.model.set_remove_after_processing(self._get_setting("clear_absorption_ws_after_processing", bool))

        abs_args = self.get_abs_args()
        atten_args, atten_err = self.get_atten_args()
        div_args, div_err = self.get_div_args()

        if not div_err and not atten_err:
            self.model.calc_all_corrections(wss, out_wss, output_settings.get_output_path(), abs_args, atten_args, div_args)

    def get_abs_args(self):
        if self.view.include_absorption():
            return {
                "gauge_vol_preset": self.view.get_shape_method(),
                "gauge_vol_file": self.view.get_custom_shape(),
                "mc_param_str": self._get_setting("monte_carlo_params"),
            }

    def get_atten_args(self):
        return self.model.get_atten_args(self.view.include_atten_tab(), self.view.get_evaluation_value(), self.view.get_evaluation_units())

    def get_div_args(self):
        return self.model.get_div_args(
            self.view.include_divergence(), self.view.get_div_horz(), self.view.get_div_vert(), self.view.get_div_det_horz()
        )

    @redraws_table
    def _copy_sample_to_all_selected(self):
        ref_ws = self.view.get_sample_reference_ws()
        wss = self.view.get_selected_workspaces()
        self.model.copy_sample_info(ref_ws, wss)

    @redraws_table
    def _copy_ref_sample_to_all_selected(self):
        ref_ws = self.model.reference_ws
        if ref_ws:
            wss = self.view.get_selected_workspaces()
            self.model.copy_sample_info(ref_ws, wss, True)

    def _on_worker_success(self):
        self.correction_notifier.notify_subscribers(self.model.get_corrected_files())

    def _on_worker_error(self, error_info):
        logger.error(str(error_info))

    def open_goniometer_dialog(self):
        self._open_alg_dialog("SetGoniometer")

    def open_ref_goniometer_dialog(self):
        self._open_alg_dialog("SetGoniometer", disabled=("Workspace",))

    def open_load_sample_shape_dialog(self):
        self._open_alg_dialog("LoadSampleShape")

    def open_set_sample_shape_dialog(self):
        self._open_alg_dialog("SetSampleShape")

    def open_set_material_dialog(self):
        self._open_alg_dialog("SetSampleMaterial")

    def _open_alg_dialog(self, alg_str, enabled=("InputWorkspace",), disabled=("",)):
        manager = InterfaceManager()
        dialog = manager.createDialogFromName(alg_str, -1, self.view, False, self.model.get_alg_preset_values(), "", enabled, disabled)
        dialog.addAlgorithmObserver(self)
        dialog.setModal(True)
        dialog.show()

    def finishHandle(self):
        # this finishHandle is called whenever a dialog created in _open_alg_dialog finishes
        # when such an alg finishes, we get our UI view to emit a signal
        self.view.signal_alg_finished()
        # this signal is then connected to on_alg_finished back here on the presenter
        # this ensures the _on_alg_finished call happens on the Qt GUI thread rather than the alg worker thread

    def _on_alg_finished(self):
        self.redraw_table()
        self.update_reference_info()

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
        self.view.set_finder_gauge_vol_visible(self.view.get_shape_method() == "Custom Shape")

    def on_create_ref_sample_clicked(self):
        self.model.create_reference_ws(self.rb_num, self.instrument)
        self.update_reference_info()

    def update_reference_info(self):
        self.view.update_reference_info_section(*self.model.get_reference_info())

    def _get_setting(self, setting_name, return_type=str):
        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, setting_name, return_type)

    def add_correction_subscriber(self, obs):
        self.correction_notifier.add_subscriber(obs)
