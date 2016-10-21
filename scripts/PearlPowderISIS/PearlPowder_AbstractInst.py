from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from abc import ABCMeta, abstractmethod
from six import add_metaclass

import PearlPowder_common

# This class provides common hooks for instruments to override
# if they want to define the behaviour of the hook. Otherwise it
# returns the object passed in without manipulating it as a default


@add_metaclass(ABCMeta)
class AbstractInst(object):
    def __init__(self, calibration_dir=None, raw_data_dir=None, output_dir=None,
                 default_input_ext=".raw", tt_mode=""):
        # ----- Properties common to ALL instruments -------- #
        self._calibration_dir = calibration_dir
        self._raw_data_dir = raw_data_dir
        self._output_dir = output_dir
        self._default_input_ext = default_input_ext
        self._tt_mode = tt_mode

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

    @property
    def tt_mode(self):
        return self._tt_mode

    # --- Methods applicable to all instruments --- #
    # These can be overridden if instrument specific behaviour is needed #

    # Script entry points
    def focus(self, run_number, focus_mode, input_ext=None, do_attenuation=True, do_van_normalisation=True):
        if input_ext is not None:
            self._default_input_ext = input_ext

        PearlPowder_common.focus(instrument=self, number=run_number, fmode=focus_mode,
                                 atten=do_attenuation, van_norm=do_van_normalisation)

    def create_empty_calibration_by_names(self, calibration_numbers, output_file_name, group_names=None):

        if group_names is None:
            group_names = self.get_default_group_names()

        PearlPowder_common.create_calibration_by_names(calruns=calibration_numbers, ngroupfile=output_file_name,
                                                       ngroup=group_names, startup_objects=self)

    def create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
        PearlPowder_common.create_calibration(startup_object=self, calibration_runs=calibration_runs,
                                              offset_file_path=offset_file_name, grouping_file_path=grouping_file_name)

    def create_calibration_Si(self, calibration_runs, out_file_name):
        PearlPowder_common.create_calibration_si(startup_object=self, calibration_runs=calibration_runs,
                                                 out_file_name=out_file_name)

    # TODO rename this to something clearer
    def create_vanadium(self, vanadium_runs, empty_runs, output_file_name, num_of_splines,
                        do_absorb_corrections=True, gen_absorb_correction=False):

        PearlPowder_common.create_vanadium(startup_object=self, vanadium_runs=vanadium_runs,
                                           empty_runs=empty_runs, output_file_name=output_file_name,
                                           num_of_spline_coefficients=num_of_splines,
                                           do_absorp_corrections=do_absorb_corrections,
                                           generate_abosrp_corrections=gen_absorb_correction)

    @staticmethod
    def set_debug_mode(val):
        assert isinstance(val, bool)
        PearlPowder_common.set_debug(val)

    def generate_out_file_paths(self, run_number, output_directory):
        file_name = self.generate_inst_file_name(run_number=run_number)
        nxs_file = output_directory + str(file_name) + ".nxs"
        gss_file = output_directory + str(file_name) + ".gss"
        tof_xye_file = output_directory + str(file_name) + "_tof_xye.dat"
        d_xye_file = output_directory + str(file_name) + "_d_xye.dat"
        out_name = str(file_name)

        out_file_names = {"nxs_filename": nxs_file,
                          "gss_filename": gss_file,
                          "tof_xye_filename": tof_xye_file,
                          "dspacing_xye_filename": d_xye_file,
                          "output_name": out_name}

        return out_file_names

    def generate_cycle_dir(self, raw_data_dir, run_cycle):
        if self._skip_appending_cycle_to_raw_dir():
            return raw_data_dir
        str_run_cycle = str(run_cycle)

        # Append current cycle to raw data directory
        generated_dir = raw_data_dir + str_run_cycle
        generated_dir = _append_path_dividers_to_end(generated_dir, raw_data_dir)

        return generated_dir

    def generate_input_full_path(self, run_number, input_dir):
        # Uses runtime polymorphism to generate the full run name
        file_name = self.generate_inst_file_name(run_number)
        extension = self.get_input_extension()
        return input_dir + file_name + extension

    def get_input_extension(self):
        """
        Gets the extension of input files for this instrument
        @param self: The instrument to query the value for
        @return: The string of the default extension for this file
        """
        return _append_dot_to_ext(self._default_input_ext)

    # Instrument specific properties to be implemented by base classes #

    @abstractmethod
    def get_lambda_range(self):
        """
        Returns the lower and upper lambda range for this instrument
        @param self: The instrument to query the values of lambda for
        @return: The lower and uppers lambda range (in that order)
        """
        pass

    @abstractmethod
    def get_focus_tof_binning(self):
        """
        Returns the TOF binning values
        @param self: The instrument to get TOF binning values for
        @return: TOF binning Values
        """
        pass

    @abstractmethod
    def get_create_van_tof_binning(self):
        """
        Holds the TOF rebin params for create vanadium calibration
        @return: The TOF rebin params as a string
        """
        pass

    # Instrument default parameters

    @abstractmethod
    def get_default_group_names(self):
        """
        Returns the default names for creating a blank calibration by names
        @return: The default grouping names as a string
        """

    # Instrument specific methods

    @abstractmethod
    def get_calibration_full_paths(self, cycle):
        """
        Gets the current calibration file names for this cycle
        @param cycle: The cycle string to lookup for this run
        @return: A dictionary the containing the full paths as values for the following keys:
        "calibration", "grouping", "vanadium_absorption", "vanadium"
        """
        pass

    @abstractmethod
    def spline_background(self, focused_vanadium_ws, spline_number, instrument_version=''):
        """
        Splines the background in a way specific to the instrument
        @param focused_vanadium_ws: The workspace to perform spline backgrounds on
        @param instrument_version: (Optional) Used for instruments with multiple versions
        @return: List of workspaces with splined backgrounds
        """

    @staticmethod
    @abstractmethod
    def generate_inst_file_name(run_number):
        """
        Generates the conforming file names for an instrument
        @param run_number: The run number to turn into a filename
        @return: The filename of the file - Without the path or extension
        """
        pass

    @staticmethod
    @abstractmethod
    def get_instrument_alg_save_ranges(instrument=''):
        #  TODO fix this documentation when we know what alg_range and save_range is used for
        """
        Gets the instruments ranges for running the algorithm and saving
        @param self: The instrument to query for this information
        @param instrument: The version of the instrument if applicable
        @return: The algorithm and save range in that order
        """

    @staticmethod
    @abstractmethod
    def get_cycle_information(run_number):
        """
        Gets all the information about this run for this cycle and returns it in a dictionary
        @param run_number: The run to match the cycle to
        @return: Dictionary with the following keys: "cycle", "instrument_version"
        """
        pass

    # --- Instrument optional hooks ----#
    def attenuate_workspace(self, input_workspace):
        return _empty_hook_return_input(input_workspace)

    def get_monitor(self, run_number, input_dir, spline_terms):
        return _empty_hook()

    def get_monitor_spectra(self, run_number):
        return _empty_hook_return_empty_string()

    def _skip_appending_cycle_to_raw_dir(self):
        return False


def _append_dot_to_ext(ext):
    if not ext.startswith('.'):
        return '.' + ext
    else:
        return ext


def _append_path_dividers_to_end(generated_dir, raw_data_dir):
    if raw_data_dir.endswith('\\'):
        generated_dir += '\\'
    elif raw_data_dir.endswith('/'):
        generated_dir += '/'
    else:
        raise ValueError("Path :" + raw_data_dir + "\n Does not end with a \\ or / character")
    return generated_dir


def _empty_hook():
    pass


def _empty_hook_return_empty_string():
    return str('')


def _empty_hook_return_input(param):
    # We should return the input workspace untouched
    return param
