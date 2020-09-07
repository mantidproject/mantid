# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from os import path
from mantidqt.utils.observer_pattern import Observable

SETTINGS_DICT = {"save_location": str, "full_calibration": str, "recalc_vanadium": bool, "logs": str}

DEFAULT_SETTINGS = {
    "full_calibration": "",
    "save_location": path.join(path.expanduser("~"), "Engineering_Mantid"),
    "recalc_vanadium": False,
    "logs": ','.join(
        ['Temp_1', 'W_position', 'X_position', 'Y_position', 'Z_position', 'stress', 'strain', 'stressrig_go'])
}

ALL_LOGS = ','.join(
    ['ADC1_0', 'ADC1_2', 'ADC1_3', 'C6_master_freq', 'C6_master_phase', 'C6_slave_freq', 'C6_slave_phase',
     'C9_master_freq', 'C9_master_phase', 'C9_slave_freq', 'C9_slave_phase', 'Cyba_a', 'Cyba_b', 'Cyba_c',
     'ICP_DAE_TD', 'ICP_SYS_TD', 'Max_output_1', 'Max_output_2', 'Max_output_3', 'North_X', 'North_X_hlim',
     'North_X_llim', 'Output_1', 'Output_2', 'RampRate', 'Rampon_3', 'Ramprate_3', 'S_position', 'South_X',
     'South_X_hlim', 'South_X_llim', 'Temp_1', 'Temp_2', 'Temp_3', 'W_position', 'X_position', 'Y_position',
     'Z_position', 'amplitude', 'control_channel', 'control_channel_string', 'count_rate', 'cycle_rbv', 'cycles',
     'dae_beam_current', 'frequency', 'good_uah_log', 'jaws1_hgap', 'jaws2_hgap', 'jaws3_hgap', 'jaws3_vgap',
     'np_ratio', 'pos_step_time', 'position', 'position_sp_rbv', 'proton_charge', 'raw_uah_log', 'strain',
     'strain_sp_rbv', 'strain_step_time', 'stress', 'stress_sp_rbv', 'stress_step_time', 'stressrig_go',
     'wave_running', 'wave_start', 'wave_type'])


class SettingsPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.settings = {}
        self.savedir_notifier = self.SavedirNotifier(self)

        # add logs to list in view
        self.view.add_log_checkboxs(ALL_LOGS)

        # Connect view signals
        self.view.set_on_apply_clicked(self.save_new_settings)
        self.view.set_on_ok_clicked(self.save_and_close_dialog)
        self.view.set_on_cancel_clicked(self.close_dialog)
        self.view.set_on_log_changed(self.save_new_settings)

    def show(self):
        self.view.show()

    def load_existing_settings(self):
        self.load_settings_from_file_or_default()
        self._show_settings_in_view()

    def close_dialog(self):
        self.view.close()

    def save_and_close_dialog(self):
        self.save_new_settings()
        self.close_dialog()

    def save_new_settings(self):
        self._collect_new_settings_from_view()
        self._save_settings_to_file()

    def _collect_new_settings_from_view(self):
        self.settings["save_location"] = self.view.get_save_location()
        self.settings["full_calibration"] = self.view.get_full_calibration()
        self.settings["recalc_vanadium"] = self.view.get_van_recalc()
        self.settings["logs"] = self.view.get_checked_logs()

    def _show_settings_in_view(self):
        if self._validate_settings(self.settings):
            self.view.set_save_location(self.settings["save_location"])
            self.view.set_full_calibration(self.settings["full_calibration"])
            self.view.set_van_recalc(self.settings["recalc_vanadium"])
            self.view.set_checked_logs(self.settings["logs"])
        self._find_files()

    def _find_files(self):
        self.view.find_full_calibration()
        self.view.find_save()

    def _save_settings_to_file(self):
        if self._validate_settings(self.settings):
            self.model.set_settings_dict(self.settings)
            self.savedir_notifier.notify_subscribers(self.settings["save_location"])

    def load_settings_from_file_or_default(self):
        self.settings = self.model.get_settings_dict(SETTINGS_DICT)
        if not self._validate_settings(self.settings):
            self.settings = DEFAULT_SETTINGS.copy()
            self._save_settings_to_file()

    @staticmethod
    def _validate_settings(settings):
        try:
            all_keys = settings.keys() == SETTINGS_DICT.keys()
            save_location = str(settings["save_location"])
            save_valid = save_location != "" and save_location is not None
            recalc_valid = settings["recalc_vanadium"] is not None
            log_valid = settings["logs"] != ""
            return all_keys and save_valid and recalc_valid and log_valid
        except KeyError:  # Settings contained invalid key.
            return False

    # -----------------------
    # Observers / Observables
    # -----------------------
    class SavedirNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
