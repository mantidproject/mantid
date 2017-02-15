from __future__ import (absolute_import, division, print_function)

import os
from isis_powder.routines import calibrate, focus, common_enums


# This class provides common hooks for instruments to override
# if they want to define the behaviour of the hook. Otherwise it
# returns the object passed in without manipulating it as a default

# Anything we don't expect the user to call is prepended with an underscore
# '_'. This way they are hidden in the ipython window (or at least go to the bottom)
# to denote internal methods to abstract_inst we will use '_abs_' to denote it as a
# private method for the scripts

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
    def output_dir(self):
        return self._output_dir

    @property
    def user_name(self):
        return self._user_name

    def _create_vanadium(self, vanadium_runs, empty_runs,
                         do_absorb_corrections=True, gen_absorb_correction=False):
        """
        Creates a vanadium calibration - should be called by the concrete instrument
        :param vanadium_runs: The vanadium run or run in range (depending on instrument) to process
        :param empty_runs: The empty run to process
        :param do_absorb_corrections: Set to true if absorption corrections should be applied
        :param gen_absorb_correction: Set to true if absorption corrections should be recalculated
        :return: d_spacing focused vanadium group
        """
        return calibrate.create_van(instrument=self, van=vanadium_runs, empty=empty_runs,
                                    absorb=do_absorb_corrections, gen_absorb=gen_absorb_correction)

    def _focus(self, run_number_string, do_van_normalisation):
        """
        Focuses the user specified run - should be called by the concrete instrument
        :param run_number_string: The run number(s) to be processed
        :param do_van_normalisation: True to divide by the vanadium run, false to not.
        :return:
        """
        return focus.focus(run_number_string=run_number_string, perform_vanadium_norm=do_van_normalisation,
                           instrument=self)

    # Mandatory overrides

    def _get_run_details(self, run_number_string):
        """
        Returns a RunDetails object with various properties related to the current run set
        :param run_number_string: The run number to look up the properties of
        :return: A RunDetails object containing attributes relevant to that run_number_string
        """
        raise NotImplementedError("get_run_details must be implemented per instrument")

    @staticmethod
    def _generate_input_file_name(run_number):
        """
        Generates a name which Mantid uses within Load to find the file.
        :param run_number: The run number to convert into a valid format for Mantid
        :return: A filename that will allow Mantid to find the correct run for that instrument.
        """
        raise NotImplementedError("generate_input_file_name must be implemented per instrument")

    def _generate_output_file_name(self, run_number_string):
        """
        Generates the filename which is used to uniquely identify and save the workspace. This should include any
        distinguishing properties such as if absorption corrections were performed.
        :param run_number_string: The run string to uniquely identify the run
        :return: The file name which identifies the run and appropriate parameter settings
        """
        raise NotImplementedError("generate_output_file_name must be implemented per instrument")

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        """
        Takes a background spline of the list of processed vanadium banks
        :param focused_vanadium_banks: The list processed (and cropped) vanadium banks to take a spline of
        :return: The splined vanadium workspaces as a list
        """
        # XXX: Although this could be moved to common if enough instruments spline the same way and have
        # the instrument override the optional behaviour
        raise NotImplementedError("spline_vanadium_ws must be implemented per instrument")

    # Optional overrides
    def _apply_absorb_corrections(self, run_details, van_ws, gen_absorb=False):
        """
        Generates vanadium absorption corrections to compensate for the container
        :param van_ws: A reference vanadium workspace to match the binning of or correct
        :return: A workspace containing the corrections
        """
        raise NotImplementedError("apply_absorb_corrections Not implemented for this instrument yet")

    def _attenuate_workspace(self, input_workspace):
        """
        Applies an attenuation correction to the workspace
        :param input_workspace: The workspace to correct
        :return: The corrected workspace
        """
        return input_workspace

    @staticmethod
    def _can_auto_gen_vanadium_cal():
        """
        Can be overridden and returned true by instruments who can automatically run generate vanadium calculation
        if the splines cannot be found during the focus routine
        :return: False by default, True by instruments who can automatically generate these
        """
        return False

    def _crop_banks_to_user_tof(self, focused_banks):
        """
        Crops to a user specified TOF range on a bank-by-bank basis. This is called after focusing a sample and
        performing the various corrections required
        :param focused_banks: The processed banks as a list to be cropped
        :return: A list of cropped banks
        """
        return focused_banks

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        """
        Crops the raw data to a sensible TOF range before processing. This is so that instruments (e.g. PEARL)
        who capture double the data only process one 'window' of data at a time.
        :param ws_to_crop: The raw workspace to crop in TOF
        :return: The cropped workspace ready for processing
        """
        return ws_to_crop

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        """
        Crops the vanadium workspace to a user specified TOF, this is to prevent the b-spline being affected by
        values after diffraction which are set to 0 as there was no data for that TOF.
        :param van_ws_to_crop: A list of focused vanadium banks to crop
        :return: A list of cropped vanadium workspace banks
        """
        return van_ws_to_crop

    def _generate_auto_vanadium_calibration(self, run_details):
        """
        Used by focus if a vanadium spline was not found to automatically generate said spline if the instrument
        has indicated support by overriding can_auto_gen_vanadium_cal
        :param run_details: The run details of the current run
        :return: None
        """
        # If the instrument overrides can_auto_gen_vanadium_cal it needs to implement this method to perform the
        # automatic calibration
        raise NotImplementedError("Automatic vanadium corrections have not been implemented for this instrument.")

    def _get_input_batching_mode(self):
        """
        Returns the user specified input batching (e.g. summed or individual processing). This is set to summed
        by default for instruments who do not with to specify it
        :return: The current input batching type from the InputBatchingEnum
        """
        return common_enums.InputBatchingEnum.Summed

    def _get_monitor_spectra_index(self, run_number):
        """
        Returns the spectra number a monitor is located at
        :param run_number: The run number to locate the monitor spectra of
        :return: The monitor spectra for the current workspace
        """
        return str()

    def _normalise_ws_current(self, ws_to_correct, run_details=None):
        """
        Normalises the workspace by the beam current at the time it was taken
        :param ws_to_correct: The workspace to normalise the current of
        :param run_details: The run details associated to the run
        :return: The normalised workspace
        """
        return None

    def _output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        """
        Takes a list of focused workspace banks and saves them out in an instrument appropriate format.
        :param processed_spectra: The list of workspace banks to save out
        :param run_details: The run details associated with this run
        :param output_mode: Optional - Sets additional saving/grouping behaviour depending on the instrument
        :return: d-spacing group of the processed output workspaces
        """
        return None

    # Steps applicable to all instruments

    def _generate_out_file_paths(self, run_details):
        """
        Generates the various output paths and file names to be used during saving or as workspace names
        :param run_details: The run details associated with this run
        :return: A dictionary containing the various output paths and generated output name
        """
        output_directory = os.path.join(self._output_dir, run_details.label, self._user_name)
        output_directory = os.path.abspath(os.path.expanduser(output_directory))
        file_name = str(self._generate_output_file_name(run_number_string=run_details.user_input_run_number))
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
