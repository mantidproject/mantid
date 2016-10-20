from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from abc import ABCMeta, abstractmethod
from six import add_metaclass

# This class provides common hooks for instruments to override
# if they want to define the behaviour of the hook. Otherwise it
# returns the object passed in without manipulating it as a default


@add_metaclass(ABCMeta)
class AbstractInst(object):
    def __init__(self, calibration_dir=None, raw_data_dir=None, output_dir=None):
        # ----- Properties common to ALL instruments -------- #
        self._calibration_dir = calibration_dir
        self._raw_data_dir = raw_data_dir
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

    # Instrument specific properties #

    @abstractmethod
    def get_input_extension(self):
        """
        Gets the extension of input files for this instrument
        @param self: The instrument to query the value for
        @return: The string of the default extension for this file
        """
        pass

    @abstractmethod
    def get_lambda_range(self):
        """
        Returns the lower and upper lambda range for this instrument
        @param self: The instrument to query the values of lambda for
        @return: The lower and uppers lambda range (in that order)
        """
        pass

    @abstractmethod
    def get_tof_binning(self):
        """
        Returns the TOF binning values
        @param self: The instrument to get TOF binning values for
        @return: TOF binning Values
        """
        pass

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
    def get_calibration_file_names(cycle, tt_mode=''):
        """
        Gets the current calibration file names for this cycle
        @param cycle: The cycle string to lookup for this run
        @param tt_mode: If multiple modes allow for different calibration files this is used as an additional selector
        @return: calibration_file, grouping_file, vanadium_absorptions_file, vanadium_file, instrument version
        in that order
        """
        pass

    @staticmethod
    @abstractmethod
    def get_cycle_information(run_number):
        """
        Gets all the information about this run for this cycle and returns it in a dictionary
        @param run_number: The run to match the cycle to
        @return: Dictionary with the following keys: "cycle", "instrument_version"
        """
        pass



    # --- Methods applicable to all instruments --- #
    # These can be overridden if instrument specific behaviour is needed#
    @staticmethod
    def generate_out_file_paths(file_name, output_directory):
        nxs_file = output_directory + str(file_name) + ".nxs"
        gssfile = output_directory + str(file_name) + ".gss"
        tof_xye_file = output_directory + str(file_name) + "_tof_xye.dat"
        d_xye_file = output_directory + str(file_name) + "_d_xye.dat"
        out_name = str(file_name)

        out_file_names = {"nxs_filename": nxs_file,
                          "gss_filename": gssfile,
                          "tof_xye_filename": tof_xye_file,
                          "dspacing_xye_filename": d_xye_file,
                          "output_name": out_name}

        return out_file_names

    def generate_input_full_path(self, run_number, input_dir):
        # Uses runtime polymorphism to generate the full run name
        file_name = self.generate_inst_file_name(run_number)
        extension = self.get_input_extension()
        return input_dir + file_name + extension

    # --- Instrument optional hooks ----#
    def attenuate_workspace(self, input_workspace):
        return _empty_hook_return_input(input_workspace)

    def get_monitor(self, run_number, input_dir, spline_terms):
        return _empty_hook()

    def get_monitor_spectra(self, run_number):
        return _empty_hook_return_empty_string()



def _empty_hook():
    pass

def _empty_hook_return_empty_string():
    return str('')

def _empty_hook_return_input(param):
    # We should return the input workspace untouched
    return param
