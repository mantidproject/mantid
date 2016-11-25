from __future__ import (absolute_import, division, print_function)

import os
from abc import ABCMeta, abstractmethod

from six import add_metaclass

from isis_powder.routines import calibrate, focus, common


# This class provides common hooks for instruments to override
# if they want to define the behaviour of the hook. Otherwise it
# returns the object passed in without manipulating it as a default


@add_metaclass(ABCMeta)
class AbstractInst(object):
    def __init__(self, user_name=None, calibration_dir=None, output_dir=None, **kwargs):
        # ----- Properties common to ALL instruments -------- #
        if user_name is None:
            raise ValueError("A user name must be specified")
        self._user_name = user_name
        self._calibration_dir = calibration_dir
        self._output_dir = output_dir

        # Advanced settings
        self._default_input_ext = _prefix_dot_to_ext(kwargs.get("default_input_ext", '.raw'))

    @property
    def calibration_dir(self):
        return self._calibration_dir

    @property
    def raw_data_dir(self):
        return self._raw_data_dir

    @property
    def output_dir(self):
        return self._output_dir

    @property
    def default_input_ext(self):
        return self._default_input_ext

    @default_input_ext.setter
    def default_input_ext(self, new_ext):
        self._default_input_ext = _prefix_dot_to_ext(new_ext)

    @property
    def user_name(self):
        return self._user_name

    # --- Public API ---- #

    # Script entry points
    def _focus(self, run_number, do_attenuation, do_van_normalisation):
        return focus.focus(run_number=run_number, perform_attenuation=do_attenuation,
                           perform_vanadium_norm=do_van_normalisation, instrument=self)

    def create_empty_calibration_by_names(self, calibration_numbers, output_file_name, group_names=None):

        if group_names is None:
            group_names = self.get_default_group_names()

        common.create_calibration_by_names(calibration_runs=calibration_numbers, grouping_file_name=output_file_name,
                                           group_names=group_names, startup_objects=self)

    def _create_calibration_vanadium(self, vanadium_runs, empty_runs, output_file_name=None,
                                     do_absorb_corrections=True, gen_absorb_correction=False):
        return calibrate.create_van(instrument=self, van=vanadium_runs, empty=empty_runs,
                                    output_van_file_name=output_file_name,
                                    absorb=do_absorb_corrections, gen_absorb=gen_absorb_correction)

    # ---- Private API ---- #
    # These are to be called from either concrete instruments or common not by users
    # Common steps to all instruments

    def _generate_out_file_paths(self, run_details, output_directory=None):
        if not output_directory:
            output_directory = os.path.join(self._output_dir, run_details.label, self._user_name)
        file_name = self.generate_inst_file_name(run_number=run_details.run_number)
        nxs_file = os.path.join(output_directory, (str(file_name) + ".nxs"))
        gss_file = os.path.join(output_directory, (str(file_name) + ".gss"))
        tof_xye_file = os.path.join(output_directory, (str(file_name) + "_tof_xye.dat"))
        d_xye_file = os.path.join(output_directory, (str(file_name) + "_d_xye.dat"))
        out_name = str(file_name)

        out_file_names = {"nxs_filename": nxs_file,
                          "gss_filename": gss_file,
                          "tof_xye_filename": tof_xye_file,
                          "dspacing_xye_filename": d_xye_file,
                          "output_name": out_name,
                          "output_folder": output_directory}

        return out_file_names

    def _generate_input_full_path(self, run_number, input_dir):
        # Uses runtime polymorphism to generate the full run name
        file_name = self.generate_inst_file_name(run_number)
        extension = self.default_input_ext
        return os.path.join(input_dir, (file_name + extension))

    # Instrument specific properties

    @abstractmethod
    def get_run_details(self, run_number):
        pass

    @staticmethod
    @abstractmethod
    def generate_inst_file_name(run_number):
        pass

    @abstractmethod
    def get_num_of_banks(self, instrument_version=''):
        pass

    # --- Instrument optional hooks ----#

    def apply_solid_angle_efficiency_corr(self, ws_to_correct, run_details):
        return ws_to_correct

    def attenuate_workspace(self, input_workspace):
        return input_workspace

    def calculate_focus_binning_params(self, sample):
        return None

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        raise NotImplementedError("Cannot process the sample with a vanadium run for this instrument")

    def get_default_group_names(self):
        return None

    def get_monitor_spectra_index(self, run_number):
        return str()

    def generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        raise NotImplementedError("Not implemented for this instrument yet")

    @staticmethod
    def get_save_range(instrument_version):
        return None

    def normalise_ws(self, ws_to_correct, run_details=None):
        return None

    def output_focused_ws(self, processed_spectra, run_details, attenuate=False):
        return None

    def pearl_focus_tof_rebinning(self, input_workspace):
        return input_workspace

    def pearl_van_calibration_tof_rebinning(self, vanadium_ws, tof_rebin_pass, return_units):
        return vanadium_ws

    def pearl_rebin_to_workspace(self, ws_to_rebin, ws_to_match):
        return ws_to_rebin

    def spline_vanadium_ws(self, focused_vanadium_ws, instrument_version=''):
        return None

    # Support for old API - can be removed when Pearl_routines is removed

    def _old_api_pearl_filename_is_full_path(self):
        return False

    def _old_api_pearl_setup_input_dirs(self, run_number):
        return None


# ----- Private Implementation ----- #
# These should only be called by the abstract instrument class


def _prefix_dot_to_ext(ext):
    if not ext.startswith('.'):
        return '.' + ext
    else:
        return ext
