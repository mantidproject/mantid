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
    "full_calibration": str,
    "logs": str,
    "primary_log": str,
    "sort_ascending": bool,
    "default_peak": str,
    "path_to_gsas2": str,
    "timeout": str,
    "dSpacing_min": str,
}

DEFAULT_SETTINGS = {
    "full_calibration": path.join(CALIB_DIR, DEFAULT_FULL_INST_CALIB),
    "save_location": path.join(path.expanduser("~"), "Engineering_Mantid"),
    "logs": ",".join(["Temp_1", "W_position", "X_position", "Y_position", "Z_position", "stress", "strain", "stressrig_go"]),
    "primary_log": "strain",
    "sort_ascending": True,
    "default_peak": "BackToBackExponential",
    "path_to_gsas2": GSAS2_PATH_ON_IDAAAS if path.exists(GSAS2_PATH_ON_IDAAAS) else "",
    "timeout": "10",  # seconds
    "dSpacing_min": "1.0",  # angstroms
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

    def validate_gsas2_path(self):
        # FileFinderWidget doesn't validate that the directory exists if isForDirectory=true (probably to support save
        # locations where directory gets created on save). So check the GSAS2 path is valid here
        if not self.validate_path_empty_or_valid(self.view.get_path_to_gsas2()):
            self.view.finder_path_to_gsas2.setFileProblem("Path does not exist")

    @staticmethod
    def validate_path_empty_or_valid(path_to_check):
        if path_to_check:
            if not path.exists(path_to_check):
                return False
        return True

    def save_new_settings(self):
        self._collect_new_settings_from_view()
        self._save_settings_to_file(set_nullables_to_default=False)

    def _collect_new_settings_from_view(self):
        self._validate_settings()
        self.settings["save_location"] = self.view.get_save_location()
        self.settings["full_calibration"] = self.view.get_full_calibration()
        self.settings["logs"] = self.view.get_checked_logs()
        self.settings["primary_log"] = self.view.get_primary_log()
        self.settings["sort_ascending"] = self.view.get_ascending_checked()
        self.settings["default_peak"] = self.view.get_peak_function()
        self.settings["path_to_gsas2"] = self.view.get_path_to_gsas2()
        self.settings["timeout"] = self.view.get_timeout()
        self.settings["dSpacing_min"] = self.view.get_dSpacing_min()

    def _show_settings_in_view(self):
        self._validate_settings(set_nullables_to_default=False)
        self.view.set_save_location(self.settings["save_location"])
        self.view.set_full_calibration(self.settings["full_calibration"])
        self.view.set_checked_logs(self.settings["logs"])
        self.view.set_primary_log_combobox(self.settings["primary_log"])
        self.view.set_ascending_checked(self.settings["sort_ascending"])
        self.view.set_peak_function(self.settings["default_peak"])
        self.view.set_path_to_gsas2(self.settings["path_to_gsas2"])
        self.view.set_timeout(self.settings["timeout"])
        self.view.set_dSpacing_min(self.settings["dSpacing_min"])
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

    def check_and_populate_with_default(self, name):
        if name not in self.settings or self.settings[name] == "":
            self.settings[name] = DEFAULT_SETTINGS[name]

    def _validate_settings(self, set_nullables_to_default=True):
        for key in list(self.settings):
            if key not in DEFAULT_SETTINGS.keys():
                del self.settings[key]
        self.check_and_populate_with_default("default_peak")
        if self.settings["default_peak"] not in ALL_PEAKS:
            self.settings["default_peak"] = DEFAULT_SETTINGS["default_peak"]
        self.check_and_populate_with_default("full_calibration")
        if not path.isfile(self.settings["full_calibration"]):
            self.settings["full_calibration"] = DEFAULT_SETTINGS["full_calibration"]
        self.check_and_populate_with_default("save_location")
        self.check_and_populate_with_default("logs")

        if set_nullables_to_default:
            self.check_and_populate_with_default("primary_log")

        # boolean values already checked to be "" or True or False in settings_helper
        self.check_and_populate_with_default("sort_ascending")
        self.check_and_populate_with_default("path_to_gsas2")
        self.check_and_populate_with_default("timeout")
        self.check_and_populate_with_default("dSpacing_min")

    # -----------------------
    # Observers / Observables
    # -----------------------
    class SavedirNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
