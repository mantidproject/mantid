# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from isis_powder.routines import calibrate, focus, common, common_enums, common_output
from mantid.kernel import config, logger
import mantid.simpleapi as mantid

# This class provides common hooks for instruments to override
# if they want to define the behaviour of the hook. Otherwise it
# returns the object passed in without manipulating it as a default

# Anything we don't expect the user to call is prepended with an underscore
# '_'. This way they are hidden in the ipython window (or at least go to the bottom)
# to denote internal methods to abstract_inst we will use '_abs_' to denote it as a
# private method for the scripts


class AbstractInst(object):
    def __init__(self, user_name, calibration_dir, output_dir, inst_prefix):
        # ----- Properties common to ALL instruments -------- #
        if user_name is None:
            raise ValueError("A user name must be specified")
        self._user_name = user_name
        self._calibration_dir = calibration_dir
        self._inst_prefix = inst_prefix
        try:
            self._inst_prefix_short = config.getInstrument(inst_prefix).shortName()
        except RuntimeError:
            logger.warning("Unknown instrument {}. Setting short prefix equal to full prefix".format(inst_prefix))
            self._inst_prefix_short = inst_prefix
        self._output_dir = output_dir
        self._is_vanadium = None
        self._beam_parameters = None

    @property
    def calibration_dir(self):
        return self._calibration_dir

    @property
    def output_dir(self):
        return self._output_dir

    @property
    def user_name(self):
        return self._user_name

    def _create_vanadium(self, run_number_string, do_absorb_corrections, do_spline=True, per_detector=False):
        """
        Creates a vanadium calibration - should be called by the concrete instrument
        :param run_number_string : The user input string for any run within the cycle
        to help us determine the correct vanadium to create later
        :param do_absorb_corrections: Set to true if absorption corrections should be applied
        :param per_detector: Whether the Vanadium correction will be done per detector or per bank
        :return: d_spacing focused vanadium group
        """
        self._is_vanadium = True
        run_details = self._get_run_details(run_number_string)

        if per_detector:
            return calibrate.create_van_per_detector(instrument=self, run_details=run_details, absorb=do_absorb_corrections)

        return calibrate.create_van(
            instrument=self,
            run_details=run_details,
            absorb=do_absorb_corrections,
            spline=do_spline,
        )

    def _focus(
        self,
        run_number_string,
        do_van_normalisation,
        do_absorb_corrections,
        sample_details=None,
        empty_can_subtraction_method=None,
        paalman_pings_events_per_point=None,
    ):
        """
        Focuses the user specified run(s) - should be called by the concrete instrument.
        :param run_number_string: The run number(s) to be processed.
        :param do_van_normalisation: Whether to divide by the vanadium run or not.
        :param do_absorb_corrections: Whether to apply absorption correction or not.
        :param sample_details: Sample details for the run number(s).
        :param empty_can_subtraction_method: The method for absorption correction. Can be 'Simple' or 'PaalmanPings'.
        :param paalman_pings_events_per_point: The number of events used in Paalman Pings Monte Carlo absorption correction.
        :return: the focussed run(s).
        """
        self._is_vanadium = False
        focused_runs = focus.focus(
            run_number_string=run_number_string,
            perform_vanadium_norm=do_van_normalisation,
            instrument=self,
            absorb=do_absorb_corrections,
            sample_details=sample_details,
            empty_can_subtraction_method=empty_can_subtraction_method,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )

        return self._output_focused_runs(focused_runs, run_number_string)

    def _output_focused_runs(self, focused_runs, run_number_string):
        run_details = self._get_run_details(run_number_string)
        input_batching = self._get_input_batching_mode()
        for irun, focused_run in enumerate(focused_runs):
            if input_batching == common_enums.INPUT_BATCHING.Individual:
                run_details.output_run_string = str(common.generate_run_numbers(run_number_string=run_number_string)[irun])
            d_spacing_group, tof_group = self._output_focused_ws(focused_run, run_details=run_details)
            common.keep_single_ws_unit(d_spacing_group=d_spacing_group, tof_group=tof_group, unit_to_keep=self._get_unit_to_keep())

            common.remove_intermediate_workspace(focused_run)

        return d_spacing_group

    def mask_prompt_pulses_if_necessary(self, ws_list):
        """
        Mask prompt pulses in a list of input workspaces,
        disabled for all instrument except HRPD
        """
        pass

    def set_beam_parameters(self, height, width):
        """
        Set the height and width of the beam. Currently only supports rectangular (or square) beam shapes.
        Implementation should be common across all instruments so no need for overriding in instrument classes.
        :param height: Height of the beam (mm).
        :param width: Width of the beam (mm).
        :return:
        """
        try:
            height = float(height)
            width = float(width)
        except ValueError:
            raise ValueError("Beam height and width must be numbers.")
        if height <= 0 or width <= 0:
            raise ValueError("Beam height and width must be more than 0.")
        else:
            self._beam_parameters = {"height": height, "width": width}

    def should_subtract_empty_inst(self):
        """
        :return: Whether the empty run should be subtracted from a run being focused
        """
        return True

    def should_subtract_empty_inst_from_vanadium(self):
        """
        :return: Whether the empty run should be subtracted from a vandium run
        """
        return True

    def perform_abs_vanadium_norm(self):
        """
        :return: Whether the sample run should undergo an absolute normalisation to
        produce a differential cross section
        """
        return False

    def apply_drange_cropping(self, run_number_string, focused_ws):
        """
        Apply dspacing range cropping to a focused workspace. It is now only used with OSIRIS script
        :param run_number_string: The run number to look up for the drange
        :param focused_ws: The workspace to be cropped
        :return: The cropped workspace in its drange
        """

        return focused_ws

    def get_vanadium_path(self, run_details):
        """
        Get the vanadium path from the run details
        :param run_details: The run details of the run number
        :return: the vanadium path
        """

        return run_details.splined_vanadium_file_path

    # Mandatory overrides

    def _get_run_details(self, run_number_string):
        """
        Returns a RunDetails object with various properties related to the current run set
        :param run_number_string: The run number to look up the properties of
        :return: A RunDetails object containing attributes relevant to that run_number_string
        """
        raise NotImplementedError("get_run_details must be implemented per instrument")

    def _generate_input_file_name(self, run_number, file_ext=None):
        """
        Generates a name which Mantid uses within Load to find the file.
        :param run_number: The run number to convert into a valid format for Mantid
        :param file_ext: An optional file extension to add to force a particular format
        :return: A filename that will allow Mantid to find the correct run for that instrument.
        """
        return self._generate_inst_filename(run_number=run_number, file_ext=file_ext)

    def _check_sample_details(self):
        # note vanadium sample details are set using advanced configs
        if self._sample_details is None and not self._is_vanadium:
            raise ValueError(
                "Absorption corrections cannot be run without sample details."
                " Please set sample details using set_sample before running absorption corrections."
            )

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        """
        Generates absorption corrections to compensate for the container. The overriding instrument
        should handle the difference between a vanadium workspace and regular workspace
        :param ws_to_correct: A reference vanadium workspace to match the binning of or correct
        :return: A workspace containing the corrections
        """
        raise NotImplementedError("apply_absorb_corrections Not implemented for this instrument yet")

    def _apply_paalmanpings_absorb_and_subtract_empty(self, workspace, summed_empty, sample_details, paalman_pings_events_per_point=None):
        """
        Generates absorption corrections to compensate for the container.
        :param workspace: the workspace to correct
        :param summed_empty: The summed and normalised empty runs
        :param sample_details: the sample details that may include container details
        :param run_number: focus run number
        :return: A workspace containing the corrections
        """
        raise NotImplementedError("apply_paalmanpings_absorb_and_subtract_empty Not implemented for this instrument yet")

    def _generate_output_file_name(self, run_number_string):
        """
        Generates the filename which is used to uniquely identify and save the workspace. This should include any
        distinguishing properties such as if absorption corrections were performed.
        :param run_number_string: The run string to uniquely identify the run
        :return: The file name which identifies the run and appropriate parameter settings
        """
        return self._generate_input_file_name(run_number=run_number_string)

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        """
        Takes a background spline of the list of processed vanadium banks
        :param focused_vanadium_banks: The list processed (and cropped) vanadium banks to take a spline of
        :return: The splined vanadium workspaces as a list
        """
        if self._inst_settings.masking_file_name is not None:
            masking_file_path = os.path.join(self.calibration_dir, self._inst_settings.masking_file_name)
            bragg_mask_list = common.read_masking_file(masking_file_path)
            focused_vanadium_banks = common.apply_bragg_peaks_masking(focused_vanadium_banks, x_values_to_mask_list=bragg_mask_list)
        output = common.spline_workspaces(focused_vanadium_spectra=focused_vanadium_banks, num_splines=self._inst_settings.spline_coeff)
        return output

    def _spline_vanadium_ws_per_detector(self, vanadium_ws, grouping_file_path):
        """
        Fits a spline to each of the spectra in the supplied vanadium workspace. Used for per detector V corrections
        :param vanadium_ws: workspace that splines will be fitted to
        :param grouping_file_path: path to the grouping file
        :return: The splined vanadium workspace
        """
        if self._inst_settings.masking_file_name is not None:
            masking_file_path = os.path.join(self.calibration_dir, self._inst_settings.masking_file_name)
            cal_workspace = mantid.LoadCalFile(
                InputWorkspace=vanadium_ws,
                CalFileName=grouping_file_path,
                Workspacename="cal_workspace",
                MakeOffsetsWorkspace=False,
                MakeMaskWorkspace=False,
                MakeGroupingWorkspace=True,
            )
            det_ids_on_bank_to_mask = {}
            for ws_index in range(cal_workspace.getNumberHistograms()):
                grouping = cal_workspace.dataY(ws_index)[0]
                if grouping > 0:
                    det_id = cal_workspace.getDetectorIDs(ws_index)[0]
                    if grouping in det_ids_on_bank_to_mask:
                        det_ids_on_bank_to_mask[grouping].append(det_id)
                    else:
                        det_ids_on_bank_to_mask[grouping] = [det_id]
            bragg_mask_list = common.read_masking_file(masking_file_path)
            for bank_number, peaks_on_bank in enumerate(bragg_mask_list, 1):
                ws_indices_on_bank_to_mask = []
                for workspace_index in range(vanadium_ws.getNumberHistograms()):
                    det_id = vanadium_ws.getSpectrum(workspace_index).getDetectorIDs()[0]
                    if bank_number in det_ids_on_bank_to_mask:
                        if det_id in det_ids_on_bank_to_mask[bank_number]:
                            ws_indices_on_bank_to_mask.append(workspace_index)
                common.apply_bragg_peaks_masking([vanadium_ws], [peaks_on_bank], ws_indices_on_bank_to_mask)
            common.remove_intermediate_workspace(cal_workspace)

        vanadium_ws = mantid.ConvertUnits(InputWorkspace=vanadium_ws, Target="TOF", OutputWorkspace=vanadium_ws)
        vanadium_ws = mantid.SplineBackground(
            InputWorkspace=vanadium_ws,
            WorkspaceIndex=0,
            EndWorkspaceIndex=vanadium_ws.getNumberHistograms() - 1,
            NCoeff=self._inst_settings.spline_coeff_per_detector,
            OutputWorkspace=vanadium_ws,
            EnableLogging=False,
        )
        return vanadium_ws

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

    def _get_unit_to_keep(self):
        """
        Returns the unit to keep once focusing has completed. E.g. a setting of
        TOF would keep TOF units and remove d_spacing units
        :return: Unit to keep, if one isn't specified none
        """
        return None

    def _get_instrument_bin_widths(self):
        """
        Returns the bin widths to rebin the focused workspace to. If
        the instrument does not want this step a value of None should
        not rebin the workspace
        :return: List of bin widths or None if no rebinning should take place
        """
        return None

    def get_instrument_prefix(self):
        """
        Returns the instrument prefix which tells this abstract instrument what instrument it is
        :return: The instrument prefix
        """
        return self._inst_prefix

    def _get_input_batching_mode(self):
        """
        Returns the user specified input batching (e.g. summed or individual processing). This is set to summed
        by default for instruments who do not with to specify it
        :return: The current input batching type from the InputBatchingEnum
        """
        return common_enums.INPUT_BATCHING.Summed

    def _get_current_tt_mode(self):
        """
        Returns the current tt_mode this is only applicable
        to PEARL. Otherwise returns None
        :return: Current tt_mode on PEARL, otherwise None
        """
        return None

    def _normalise_ws_current(self, ws_to_correct):
        """
        Normalises the workspace by the beam current at the time it was taken using
        normalise by current unless the instrument overrides it with its own custom
        method of normalising by current.
        :param ws_to_correct: The workspace to normalise the current of
        :return: The normalised workspace
        """
        return common.run_normalise_by_current(ws_to_correct)

    def _output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        """
        Takes a list of focused workspace banks and saves them out in an instrument appropriate format.
        :param processed_spectra: The list of workspace banks to save out
        :param run_details: The run details associated with this run
        :param output_mode: Optional - Sets additional saving/grouping behaviour depending on the instrument
        :return: d-spacing group of the processed output workspaces
        """
        d_spacing_group, tof_group = common_output.split_into_tof_d_spacing_groups(
            run_details=run_details, processed_spectra=processed_spectra
        )
        common_output.save_focused_data(
            d_spacing_group=d_spacing_group, tof_group=tof_group, output_paths=self._generate_out_file_paths(run_details=run_details)
        )

        return d_spacing_group, tof_group

    def create_solid_angle_corrections(self, vanadium, run_details):
        """
        Creates and saves the solid angle corrections from a vanadium run, only applicable on HRPD otherwise return None
        :param vanadium: The vanadium used to create this
        :param run_details: The run details used
        """
        return None

    def get_solid_angle_corrections(self, vanadium, run_details):
        """
        loads the solid angle corrections from a vanadium run, only applicable on HRPD otherwise return None
        :param vanadium: The vanadium run number used to create the solid angle corrections
        :param run_details: The run details used
        :return: the sold angle correction workspace on hrpd, otherwise none
        """
        return None

    def apply_calibration_to_focused_data(self, focused_ws):
        # convert back to TOF based on engineered detector positions
        mantid.ApplyDiffCal(InstrumentWorkspace=focused_ws, ClearCalibration=True)

    # Steps applicable to all instruments

    @staticmethod
    def _generate_run_details_fingerprint(*args):
        out_key = ""
        for arg in args:
            out_key += str(arg)
        return out_key

    def _generate_out_file_paths(self, run_details):
        """
        Generates the various output paths and file names to be used during saving or as workspace names
        :param run_details: The run details associated with this run
        :return: A dictionary containing the various output paths and generated output name
        """
        output_directory = os.path.join(self._output_dir, run_details.label, self._user_name)
        output_directory = os.path.abspath(os.path.expanduser(output_directory))
        xye_files_directory = output_directory
        if self._inst_settings.dat_files_directory:
            xye_files_directory = os.path.join(output_directory, self._inst_settings.dat_files_directory)

        file_type = "" if run_details.file_extension is None else run_details.file_extension.lstrip(".")
        out_file_names = {"output_folder": output_directory}
        format_options = {
            "inst": self._inst_prefix,
            "instlow": self._inst_prefix.lower(),
            "instshort": self._inst_prefix_short,
            "runno": run_details.output_run_string,
            "fileext": file_type,
            "_fileext": "_" + file_type if file_type else "",
            "suffix": run_details.output_suffix if run_details.output_suffix else "",
        }
        format_options = self._add_formatting_options(format_options)

        output_formats = self._get_output_formats(output_directory, xye_files_directory)
        for key, output_dir in output_formats.items():
            filepath = os.path.join(output_dir, getattr(self._inst_settings, key).format(**format_options))
            out_file_names[key] = filepath

        out_file_names["output_name"] = os.path.splitext(os.path.basename(out_file_names["nxs_filename"]))[0]
        return out_file_names

    def _get_output_formats(self, output_directory, xye_files_directory):
        return {
            "nxs_filename": output_directory,
            "gss_filename": output_directory,
            "tof_xye_filename": xye_files_directory,
            "dspacing_xye_filename": xye_files_directory,
        }

    def _generate_inst_filename(self, run_number, file_ext):
        if isinstance(run_number, list):
            # Multiple entries
            return [self._generate_inst_filename(run, file_ext) for run in run_number]
        else:
            # Individual entry
            return self._inst_prefix + str(run_number) + file_ext

    def _add_formatting_options(self, format_options):
        """
        Add any instrument-specific format options to the given
        list
        :param format_options: A dictionary of string format keys mapped to their expansions
        :return: format_options as it is passed in
        """
        return format_options

    def apply_additional_per_detector_corrections(self, input_workspace, sample_details, run_details):
        return input_workspace
