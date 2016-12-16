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
    def __init__(self, user_name=None, calibration_dir=None, output_dir=None):
        # ----- Properties common to ALL instruments -------- #
        if user_name is None:
            raise ValueError("A user name must be specified")
        self._user_name = user_name
        self._calibration_dir = calibration_dir
        self._output_dir = output_dir

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
    def user_name(self):
        return self._user_name

    # --- Public API ---- #

    # Script entry points
    def create_empty_calibration_by_names(self, calibration_numbers, output_file_name, group_names=None):

        if group_names is None:
            group_names = self.get_default_group_names()

        common.create_calibration_by_names(calibration_runs=calibration_numbers, grouping_file_name=output_file_name,
                                           group_names=group_names, startup_objects=self)

    # ---- Private API ---- #
    # These are to be called from either concrete instruments or common not by users
    # Common steps to all instruments

    def _create_calibration_vanadium(self, vanadium_runs, empty_runs,
                                     do_absorb_corrections=True, gen_absorb_correction=False):
        return calibrate.create_van(instrument=self, van=vanadium_runs, empty=empty_runs,
                                    absorb=do_absorb_corrections, gen_absorb=gen_absorb_correction)

    def _focus(self, run_number, input_batching, do_van_normalisation):
        return focus.focus(run_number=run_number, input_batching=input_batching,
                           perform_vanadium_norm=do_van_normalisation, instrument=self)

    def generate_out_file_paths(self, run_details, output_directory=None):
        if not output_directory:
            output_directory = os.path.join(self._output_dir, run_details.label, self._user_name)
        file_name = str(self.generate_output_file_name(run_number=run_details.run_number))
        nxs_file = os.path.join(output_directory, (file_name + ".nxs"))
        gss_file = os.path.join(output_directory, (file_name + ".gsas"))
        tof_xye_file = os.path.join(output_directory, (file_name + "_tof_xye.dat"))
        d_xye_file = os.path.join(output_directory, (file_name + "_d_xye.dat"))
        out_name = file_name

        out_file_names = {"nxs_filename": nxs_file,
                          "gss_filename": gss_file,
                          "tof_xye_filename": tof_xye_file,
                          "dspacing_xye_filename": d_xye_file,
                          "output_name": out_name,
                          "output_folder": output_directory}

        return out_file_names

    def _generate_input_full_path(self, run_number, input_dir):
        file_name = self.generate_input_file_name(run_number)
        return os.path.join(input_dir, file_name)

    # Instrument specific properties

    @staticmethod
    def can_auto_gen_vanadium_cal():
        return False

    @abstractmethod
    def get_run_details(self, run_number_string):
        pass

    @staticmethod
    @abstractmethod
    def generate_input_file_name(run_number):
        pass

    # --- Instrument optional hooks ----#

    @abstractmethod
    def generate_output_file_name(self, run_number):
        raise NotImplementedError("Output names not implemented")

    def apply_solid_angle_efficiency_corr(self, ws_to_correct, run_details):
        return ws_to_correct

    def attenuate_workspace(self, input_workspace):
        return input_workspace

    def get_default_group_names(self):
        return None

    def get_monitor_spectra_index(self, run_number):
        return str()

    def generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        raise NotImplementedError("Not implemented for this instrument yet")

    def normalise_ws(self, ws_to_correct, run_details=None):
        return None

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        return None

    def crop_raw_to_expected_tof_range(self, ws_to_crop):
        return ws_to_crop

    def crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return van_ws_to_crop

    def spline_vanadium_ws(self, focused_vanadium_ws):
        return None

    def crop_banks_to_user_tof(self, focused_banks):
        return focused_banks

    def generate_auto_vanadium_calibration(self, run_details):
        raise NotImplementedError("Automatic vanadium corrections have not been implemented for this instrument.")
