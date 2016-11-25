from __future__ import (absolute_import, division, print_function)

import os
from isis_powder.pearl import Pearl
from mantid import config

# This class provides a compatibility layer translating the old scripts into the new API
# especially because the old scripts made heavy use of globals so params can be set at
# any point up.


class PearlRoutinesWrapper(Pearl):
    def __init__(self, user_name, tt_mode="TT88", calibration_dir=None, output_dir=None):
        super(PearlRoutinesWrapper, self).__init__(user_name=user_name, tt_mode=tt_mode,
                                                   calibration_dir=calibration_dir, output_dir=output_dir)

        self._disable_appending_cycle_to_raw_dir = False

        # Old API support
        self._old_atten_file = None
        self._existing_config = None

    # Support for old API - All below can be removed when PEARL_Routines is removed

    def _old_api_constructor_set(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None,
                                 input_file_ext=None, tt_mode=None):
        # Any param can be set so check each individually
        if user_name is not None:
            self._user_name = user_name
        if calibration_dir is not None:
            self._calibration_dir = calibration_dir
        if raw_data_dir is not None:
            self._raw_data_dir = raw_data_dir
        if output_dir is not None:
            self._output_dir = output_dir
        if input_file_ext is not None:
            self.default_input_ext = input_file_ext
        if tt_mode is not None:
            self._tt_mode = tt_mode

    def _old_api_set_tt_mode(self, tt_mode):
        self._tt_mode = tt_mode

    def _old_api_set_calib_dir(self, calib_dir):
        self._calibration_dir = calib_dir

    def _old_api_set_raw_data_dir(self, raw_data_dir):
        self._old_api_uses_full_paths = True
        self._raw_data_dir = raw_data_dir

    def _old_api_set_output_dir(self, output_dir):
        self._output_dir = output_dir

    def _old_api_set_ext(self, ext):
        self.default_input_ext = ext

    def _old_api_set_atten(self, atten_path):
        self._old_atten_file = os.path.basename(atten_path)

    def _old_api_set_full_paths(self, val):
        self._old_api_uses_full_paths = val

    def _old_api_pearl_filename_is_full_path(self):
        return self._old_api_uses_full_paths

    def _old_api_pearl_setup_input_dirs(self, run_number):
        run_details = self.get_run_details(run_number=run_number)
        generated_path = self._generate_raw_data_cycle_dir(run_cycle=run_details.label)
        user_dirs = config['datasearch.directories']
        user_dirs_list = user_dirs.split(';')
        if generated_path not in user_dirs_list:
            config['datasearch.directories'] += ';' + generated_path

    def _old_api_get_focus_tof_binning(self):
        return self._focus_tof_binning

    def _generate_raw_data_cycle_dir(self, run_cycle):
        if self._disable_appending_cycle_to_raw_dir:
            return self.raw_data_dir
        str_run_cycle = str(run_cycle)

        # Append current cycle to raw data directory
        generated_dir = os.path.join(self.raw_data_dir, str_run_cycle)
        generated_dir += '/'

        return generated_dir
