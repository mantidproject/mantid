from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod
from six import add_metaclass
import os

from isis_powder import common
from isis_powder import focus
from isis_powder import calibrate

# This class provides common hooks for instruments to override
# if they want to define the behaviour of the hook. Otherwise it
# returns the object passed in without manipulating it as a default


@add_metaclass(ABCMeta)
class AbstractInst(object):
    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None,
                 default_input_ext=".raw"):
        # ----- Properties common to ALL instruments -------- #
        if user_name is None:
            raise ValueError("A user name must be specified")
        self._user_name = user_name
        self._calibration_dir = calibration_dir
        self._raw_data_dir = raw_data_dir
        self._output_dir = output_dir
        self._default_input_ext = _append_dot_to_ext(default_input_ext)
        self._focus_mode = None

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
        self._default_input_ext = _append_dot_to_ext(new_ext)

    @property
    def focus_mode(self):
        return self._focus_mode

    @property
    def user_name(self):
        return self._user_name

    # --- Public API ---- #

    # Script entry points
    def focus(self, run_number, focus_mode, input_ext=None, do_attenuation=True, do_van_normalisation=True):
        self._focus_mode = focus_mode
        if input_ext is not None:
            self.default_input_ext = input_ext

        return focus.focus(instrument=self, number=run_number,
                           attenuate=do_attenuation, van_norm=do_van_normalisation)

    def create_empty_calibration_by_names(self, calibration_numbers, output_file_name, group_names=None):

        if group_names is None:
            group_names = self._get_default_group_names()

        common.create_calibration_by_names(calibration_runs=calibration_numbers, grouping_file_name=output_file_name,
                                           group_names=group_names, startup_objects=self)

    def create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
        self._create_calibration(calibration_runs=calibration_runs,
                                 offset_file_name=offset_file_name, grouping_file_name=grouping_file_name)

    def create_calibration_si(self, calibration_runs, cal_file_name, grouping_file_name):
        self._create_calibration_silicon(calibration_runs=calibration_runs, cal_file_name=cal_file_name,
                                         grouping_file_name=grouping_file_name)

    def create_calibration_vanadium(self, vanadium_runs, empty_runs, output_file_name, num_of_splines=None,
                                    do_absorb_corrections=True, gen_absorb_correction=False):
        calibrate.create_van(instrument=self, van=vanadium_runs, empty=empty_runs,
                             output_van_file_name=output_file_name, num_of_splines=num_of_splines,
                             absorb=do_absorb_corrections, gen_absorb=gen_absorb_correction)

    @staticmethod
    def set_debug_mode(val):
        assert isinstance(val, bool)
        common.set_debug(val)

    # ---- Private API ---- #
    # These are to be called from either concrete instruments or common not by users
    # Common steps to all instruments

    def _generate_out_file_paths(self, run_number, output_directory):
        file_name = self._generate_inst_file_name(run_number=run_number)
        nxs_file = os.path.join(output_directory, (str(file_name) + ".nxs"))
        gss_file = os.path.join(output_directory + (str(file_name) + ".gss"))
        tof_xye_file = os.path.join(output_directory + (str(file_name) + "_tof_xye.dat"))
        d_xye_file = os.path.join(output_directory + (str(file_name) + "_d_xye.dat"))
        out_name = str(file_name)

        out_file_names = {"nxs_filename": nxs_file,
                          "gss_filename": gss_file,
                          "tof_xye_filename": tof_xye_file,
                          "dspacing_xye_filename": d_xye_file,
                          "output_name": out_name}

        return out_file_names

    def _generate_raw_data_cycle_dir(self, run_cycle):
        if self._skip_appending_cycle_to_raw_dir():
            return self.raw_data_dir
        str_run_cycle = str(run_cycle)

        # Append current cycle to raw data directory
        generated_dir = os.path.join(self.raw_data_dir, str_run_cycle)

        return generated_dir

    def _generate_input_full_path(self, run_number, input_dir):
        # Uses runtime polymorphism to generate the full run name
        file_name = self._generate_inst_file_name(run_number)
        extension = self.default_input_ext
        return os.path.join(input_dir, (file_name + extension))

    # Instrument specific properties to be implemented by base classes #

    @abstractmethod
    def _get_lambda_range(self):
        """
        Returns the lower and upper lambda range for this instrument
        @param self: The instrument to query the values of lambda for
        @return: The lower and uppers lambda range (in that order)
        """
        pass

    @abstractmethod
    def _get_create_van_tof_binning(self):
        """
        Holds the TOF rebin params for create vanadium calibration
        @return: The TOF rebin params as a dictionary of strings numbered 1,2,3...n
        """
        pass

    # Instrument default parameters

    @abstractmethod
    def _get_default_group_names(self):
        """
        Returns the default names for creating a blank calibration by names
        @return: The default grouping names as a string
        """

    # Instrument specific methods

    @abstractmethod
    def _get_calibration_full_paths(self, cycle):
        """
        Gets the current calibration file names for this cycle
        @param cycle: The cycle string to lookup for this run
        @return: A dictionary the containing the full paths as values for the following keys:
        "calibration", "grouping", "vanadium_absorption", "vanadium"
        """
        pass

    @staticmethod
    @abstractmethod
    def _generate_inst_file_name(run_number):
        """
        Generates the conforming file names for an instrument
        @param run_number: The run number to turn into a filename
        @return: The filename of the file - Without the path or extension
        """
        pass

    @staticmethod
    @abstractmethod
    def _get_instrument_alg_save_ranges(instrument=''):
        #  TODO we need to move save range out into a separate hook and make alg
        #  range represent the number of banks
        """
        Gets the instruments ranges for running the algorithm and saving
        @param instrument: The version of the instrument if applicable
        @return: The algorithm and save range in that order
        """

    @staticmethod
    @abstractmethod
    def _get_cycle_information(run_number):
        """
        Gets all the information about this run for this cycle and returns it in a dictionary
        @param run_number: The run to match the cycle to
        @return: Dictionary with the following keys: "cycle", "instrument_version"
        """
        pass

    # --- Instrument optional hooks ----#
    # TODO cull some of these hooks once we unify the scripts

    def _attenuate_workspace(self, input_workspace):
        return _empty_hook_return_input(input_workspace)

    def _create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
        # TODO what from
        """
        Creates a calibration run from
        @param calibration_runs: The runs to use to create the calibration
        @param offset_file_name: The calibration filename to be created
        @param grouping_file_name: The grouping filename to be created
        """
        raise NotImplementedError("Create calibration is not yet implemented for this instrument")

    def _create_calibration_silicon(self, calibration_runs, cal_file_name, grouping_file_name):
        """
        Creates a calibration file from a silicon run
        @param calibration_runs: The silicon calibration runs to process
        @param cal_file_name: The calibration filename to be created
        @param grouping_file_name: The grouping filename to be created
        """
        raise NotImplementedError("Create calibration from a silicon run is not yet implemented for this instrument")

    def _normalise_ws(self, ws_to_correct, monitor_ws=None, spline_terms=20):
        return _empty_hook_return_none()

    def _get_monitor_spectra(self, run_number):
        return _empty_hook_return_empty_string()

    def _PEARL_filename_is_full_path(self):
        """
        Only used by PEARL to maintain compatibility with old routines code
        @return: Whether the "filename" is actually a full path
        """
        return False

    def _spline_background(self, focused_vanadium_ws, spline_number, instrument_version=''):
        """
        Splines the background in a way specific to the instrument
        @param focused_vanadium_ws: The workspace to perform spline backgrounds on
        @param instrument_version: (Optional) Used for instruments with multiple versions
        @return: List of workspaces with splined backgrounds
        """
        return _empty_hook_return_none()

    def _skip_appending_cycle_to_raw_dir(self):
        return True
        # TODO set this to False if they just dump their raw files in one folder

    def _mask_noisy_detectors(self, vanadium_run):
        pass

    def _calculate_solid_angle_efficiency_corrections(self, vanadium_ws):
        return _empty_hook_return_none()

    def _do_tof_rebinning_focus(self, input_workspace):
        return input_workspace

    def _process_focus_output(self, processed_spectra, run_number, attenuate=False):
        return _empty_hook_return_none()

    def _subtract_sample_empty(self, input_sample):
        return input_sample

    def _apply_solid_angle_efficiency_corr(self, ws_to_correct, vanadium_number=None, vanadium_path=None):
        return ws_to_correct

    def _load_monitor(self, number, cycle):
        return None

    def _apply_van_calibration_tof_rebinning(self, vanadium_ws, tof_rebin_pass, return_units):
        return vanadium_ws

    def _generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        raise NotImplementedError("Not implemented for this instrument yet")

    def _calibration_rebin_to_workspace(self, ws_to_rebin, ws_to_match):
        return ws_to_rebin

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        raise NotImplementedError("Cannot process the sample with a vanadium run for this instrument")

    def calculate_focus_binning_params(self, sample):
        return None

# ----- Private Implementation ----- #
# These should only be called by the abstract instrument class


def _append_dot_to_ext(ext):
    if not ext.startswith('.'):
        return '.' + ext
    else:
        return ext



# These empty hooks can be used to diagnose when an override hasn't
# fired or if steps are correctly being skipped

def _empty_hook_return_empty_string():
    return str('')


def _empty_hook_return_none():
    return None


def _empty_hook_return_input(param):
    # We should return the input workspace untouched
    return param
