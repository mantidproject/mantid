# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from os import path
from mantidqt.utils.observer_pattern import Observable
from Engineering.EnggUtils import CALIB_DIR

DEFAULT_FULL_INST_CALIB = "ENGINX_full_instrument_calibration_193749.nxs"
GSAS2_PATH_ON_IDAAAS = "/opt/gsas2"
SETTINGS_DICT = {
    "save_location": str,
    "rd_name": str,
    "nd_name": str,
    "td_name": str,
    "rd_dir": str,
    "nd_dir": str,
    "td_dir": str,
    "full_calibration": str,
    "logs": str,
    "primary_log": str,
    "sort_ascending": bool,
    "default_peak": str,
    "path_to_gsas2": str,
    "timeout": str,
    "dSpacing_min": str,
    "monte_carlo_params": str,
    "clear_absorption_ws_after_processing": bool,
    "cost_func_thresh": str,
    "peak_pos_thresh": str,
    "use_euler_angles": bool,
    "euler_angles_scheme": str,
    "euler_angles_sense": str,
    "plot_exp_pf": bool,
    "contour_kernel": str,
    "auto_pop_texture": bool,
}

DEFAULT_SETTINGS = {
    "full_calibration": path.join(CALIB_DIR, DEFAULT_FULL_INST_CALIB),
    "rd_name": "RD",
    "nd_name": "ND",
    "td_name": "TD",
    "rd_dir": "1,0,0",
    "nd_dir": "0,1,0",
    "td_dir": "0,0,1",
    "save_location": path.join(path.expanduser("~"), "Engineering_Mantid"),
    "logs": ",".join(["Temp_1", "W_position", "X_position", "Y_position", "Z_position", "stress", "strain", "stressrig_go"]),
    "primary_log": "strain",
    "sort_ascending": True,
    "default_peak": "BackToBackExponential",
    "path_to_gsas2": GSAS2_PATH_ON_IDAAAS if path.exists(GSAS2_PATH_ON_IDAAAS) else "",
    "timeout": "10",  # seconds
    "dSpacing_min": "1.0",  # angstroms
    "monte_carlo_params": "SparseInstrument:True",
    "clear_absorption_ws_after_processing": True,
    "cost_func_thresh": "0.0",
    "peak_pos_thresh": "0.0",
    "use_euler_angles": False,
    "euler_angles_scheme": "YZY",
    "euler_angles_sense": "1,-1,1",
    "plot_exp_pf": True,
    "contour_kernel": "2.0",
    "auto_pop_texture": False,
}

ALL_LOGS = ",".join(
    [
        "ADC1_0",
        "ADC1_2",
        "ADC1_3",
        "C6_master_freq",
        "C6_master_phase",
        "C6_slave_freq",
        "C6_slave_phase",
        "C9_master_freq",
        "C9_master_phase",
        "C9_slave_freq",
        "C9_slave_phase",
        "Cyba_a",
        "Cyba_b",
        "Cyba_c",
        "ICP_DAE_TD",
        "ICP_SYS_TD",
        "Max_output_1",
        "Max_output_2",
        "Max_output_3",
        "North_X",
        "North_X_hlim",
        "North_X_llim",
        "Output_1",
        "Output_2",
        "RampRate",
        "Rampon_3",
        "Ramprate_3",
        "S_position",
        "South_X",
        "South_X_hlim",
        "South_X_llim",
        "Temp_1",
        "Temp_2",
        "Temp_3",
        "W_position",
        "X_position",
        "Y_position",
        "Z_position",
        "amplitude",
        "control_channel",
        "control_channel_string",
        "count_rate",
        "cycle_rbv",
        "cycles",
        "dae_beam_current",
        "frequency",
        "good_uah_log",
        "jaws1_hgap",
        "jaws2_hgap",
        "jaws3_hgap",
        "jaws3_vgap",
        "np_ratio",
        "pos_step_time",
        "position",
        "position_sp_rbv",
        "proton_charge",
        "raw_uah_log",
        "strain",
        "strain_sp_rbv",
        "strain_step_time",
        "stress",
        "stress_sp_rbv",
        "stress_step_time",
        "stressrig_go",
        "wave_running",
        "wave_start",
        "wave_type",
    ]
)

ALL_PEAKS = ",".join(["BackToBackExponential", "Gaussian", "Lorentzian", "Voigt"])


class SettingsPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.settings = {}
        self.savedir_notifier = self.SavedirNotifier(self)

        # populate lists in view
        self.view.add_log_checkboxs(ALL_LOGS)
        self.view.populate_peak_function_list(ALL_PEAKS)

        # Connect view signals
        self.view.set_on_apply_clicked(self.save_new_settings)
        self.view.set_on_ok_clicked(self.save_and_close_dialog)
        self.view.set_on_cancel_clicked(self.close_dialog)
        self.view.set_on_log_changed(self.update_logs)
        self.view.set_on_check_ascending_changed(self.ascending_changed)
        self.view.set_on_check_descending_changed(self.descending_changed)
        self.view.set_on_gsas2_path_edited(self.validate_gsas2_path)
        self.view.on_orientation_type_toggled(self.set_euler_options_enabled)
        self.view.on_scatter_pf_toggled(self.set_contour_option_enabled)

        # ensure the initial state of enabled settings is correct
        self.set_euler_options_enabled()
        self.set_contour_option_enabled()

    def show(self):
        self._show_settings_in_view()
        self.view.show()

    def close_dialog(self):
        self.view.close()

    def save_and_close_dialog(self):
        self.save_new_settings()
        self.close_dialog()

    def update_logs(self):
        self.view.set_primary_log_combobox(self.settings["primary_log"])

    def ascending_changed(self, state):
        self.view.set_descending_checked(not bool(state))

    def descending_changed(self, state):
        self.view.set_ascending_checked(not bool(state))

    def save_new_settings(self):
        self._collect_new_settings_from_view()
        self._save_settings_to_file(set_nullables_to_default=False)

    def _collect_new_settings_from_view(self):
        self.settings["save_location"] = self.view.get_save_location()
        self.settings["rd_name"] = self.view.get_rd_name()
        self.settings["nd_name"] = self.view.get_nd_name()
        self.settings["td_name"] = self.view.get_td_name()
        self.settings["rd_dir"] = self.view.get_rd_dir()
        self.settings["nd_dir"] = self.view.get_nd_dir()
        self.settings["td_dir"] = self.view.get_td_dir()
        self.settings["full_calibration"] = self.view.get_full_calibration()
        self.settings["logs"] = self.view.get_checked_logs()
        self.settings["primary_log"] = self.view.get_primary_log()
        self.settings["sort_ascending"] = self.view.get_ascending_checked()
        self.settings["default_peak"] = self.view.get_peak_function()
        self.settings["path_to_gsas2"] = self.view.get_path_to_gsas2()
        self.settings["timeout"] = self.view.get_timeout()
        self.settings["dSpacing_min"] = self.view.get_dSpacing_min()
        self.settings["monte_carlo_params"] = self.view.get_monte_carlo_params()
        self.settings["clear_absorption_ws_after_processing"] = self.view.get_remove_corr_ws_after_processing()
        self.settings["cost_func_thresh"] = self.view.get_cost_func_thresh()
        self.settings["peak_pos_thresh"] = self.view.get_peak_pos_thresh()
        self.settings["use_euler_angles"] = self.view.get_use_euler_angles()
        self.settings["euler_angles_scheme"] = self.view.get_euler_angles_scheme()
        self.settings["euler_angles_sense"] = self.view.get_euler_angles_sense()
        self.settings["plot_exp_pf"] = self.view.get_plot_exp_pf()
        self.settings["contour_kernel"] = self.view.get_contour_kernel()
        self.settings["auto_pop_texture"] = self.view.get_auto_populate_texture()
        self._validate_settings(set_nullables_to_default=False)

    def _show_settings_in_view(self):
        self._validate_settings(set_nullables_to_default=False)
        self.view.set_save_location(self.settings["save_location"])
        self.view.set_rd_name(self.settings["rd_name"])
        self.view.set_nd_name(self.settings["nd_name"])
        self.view.set_td_name(self.settings["td_name"])
        self.view.set_rd_dir(self.settings["rd_dir"])
        self.view.set_nd_dir(self.settings["nd_dir"])
        self.view.set_td_dir(self.settings["td_dir"])
        self.view.set_full_calibration(self.settings["full_calibration"])
        self.view.set_checked_logs(self.settings["logs"])
        self.view.set_primary_log_combobox(self.settings["primary_log"])
        self.view.set_ascending_checked(self.settings["sort_ascending"])
        self.view.set_peak_function(self.settings["default_peak"])
        self.view.set_path_to_gsas2(self.settings["path_to_gsas2"])
        self.view.set_timeout(self.settings["timeout"])
        self.view.set_dSpacing_min(self.settings["dSpacing_min"])
        self.view.set_monte_carlo_params(self.settings["monte_carlo_params"])
        self.view.set_remove_corr_ws_after_processing(self.settings["clear_absorption_ws_after_processing"])
        self.view.set_cost_func_thresh(self.settings["cost_func_thresh"])
        self.view.set_peak_pos_thresh(self.settings["peak_pos_thresh"])
        self.view.set_use_euler_angles(self.settings["use_euler_angles"])
        self.view.set_euler_angles_scheme(self.settings["euler_angles_scheme"])
        self.view.set_euler_angles_sense(self.settings["euler_angles_sense"])
        self.view.set_plot_exp_pf(self.settings["plot_exp_pf"])
        self.view.set_contour_kernel(self.settings["contour_kernel"])
        self.view.set_auto_populate_texture(self.settings["auto_pop_texture"])
        self._find_files()

    def _find_files(self):
        self.view.find_full_calibration()
        self.view.find_save()
        self.view.find_path_to_gsas2()
        self.validate_gsas2_path()

    def _save_settings_to_file(self, set_nullables_to_default=True):
        self._validate_settings(set_nullables_to_default)
        self.model.set_settings_dict(self.settings)
        self.savedir_notifier.notify_subscribers(self.settings["save_location"])

    def load_settings_from_file_or_default(self):
        self.settings = self.model.get_settings_dict(SETTINGS_DICT)

        self._validate_settings()

        if self.settings != self.model.get_settings_dict(SETTINGS_DICT):
            self._save_settings_to_file()
        self._find_files()

    # def validation intermediates

    def validate_gsas2_path(self):
        valid, msg = self.model.validate_gsas2_path(self.view.get_path_to_gsas2())
        if not valid:
            self.view.finder_path_to_gsas2.setFileProblem(msg)

    def validate_reference_frame(self):
        self.model.validate_reference_frame(self.settings)

    def validate_euler_settings(self):
        self.model.validate_euler_settings(self.settings, self.view.get_use_euler_angles())

    def _validate_settings(self, set_nullables_to_default=True):
        self.settings = self.model.validate_settings(self.settings, DEFAULT_SETTINGS, ALL_PEAKS, set_nullables_to_default)
        self.validate_euler_settings()

    def set_euler_options_enabled(self):
        self.view.eulerAngles_lineedit.setEnabled(self.view.get_use_euler_angles())
        self.view.eulerAnglesSense_lineedit.setEnabled(self.view.get_use_euler_angles())

    def set_contour_option_enabled(self):
        self.view.contourKernel_lineedit.setEnabled(not self.view.get_plot_exp_pf())

    # -----------------------
    # Observers / Observables
    # -----------------------
    class SavedirNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
