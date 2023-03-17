# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from isis_powder.abstract_inst import AbstractInst
from isis_powder.osiris_routines import osiris_advanced_config, osiris_algs, osiris_param_mapping
from isis_powder.routines import instrument_settings, common_output, common, focus
import copy
import mantid.simpleapi as mantid
import os


class Osiris(AbstractInst):
    def __init__(self, **kwargs):
        self._drange_sets = {}
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=osiris_param_mapping.attr_mapping, adv_conf_dict=osiris_advanced_config.get_all_adv_variables(), kwargs=kwargs
        )
        self._default_inst_settings = copy.deepcopy(self._inst_settings)

        super(Osiris, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix="OSIRIS",
        )
        self._run_details_cached_obj = {}

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._setup_drange_sets()

        vanadiums = []
        for drange_idx, drange in self._drange_sets.items():
            run_number_string = drange.get_samples_string()
            vanadium_d = self._create_vanadium(
                run_number_string=run_number_string,
                do_absorb_corrections=False,
                do_spline=False,
            )

            vanadium_d = mantid.CropWorkspace(
                InputWorkspace=vanadium_d,
                XMin=osiris_algs.d_range_alice[drange_idx][0],
                XMax=osiris_algs.d_range_alice[drange_idx][1],
                OutputWorkspace=str(vanadium_d),
            )

            vanadium_d = mantid.ConvertUnits(InputWorkspace=vanadium_d, Target="TOF")

            run_details = self._get_run_details(run_number_string=run_number_string)
            common.save_unsplined_vanadium(
                vanadium_ws=vanadium_d,
                output_path=run_details.unsplined_vanadium_file_path,
                keep_unit=True,
            )
            vanadiums.append(vanadium_d)

        return vanadiums

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)

        if not self._drange_sets:
            self._setup_drange_sets()

        focussed_runs = []

        for drange in self._drange_sets.values():
            run_number_string = drange.get_samples_string()

            processed = self._focus(
                run_number_string=run_number_string,
                do_van_normalisation=self._inst_settings.van_norm,
            )

            processed = [
                [mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="dSpacing") for ws_group in processed for ws in ws_group]
            ]

            focussed_runs.extend(processed)

        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)

        if self._inst_settings.merge_drange:
            merged_runs = osiris_algs.merge_dspacing_runs(
                focussed_runs,
                self._drange_sets,
                self._inst_settings.run_number,
            )

            d_spacing_group, tof_group = self._output_focused_ws(merged_runs, run_details)

            if len(focussed_runs) != 1:
                common.remove_intermediate_workspace([ws for ws_group in focussed_runs for ws in ws_group])
        else:
            d_spacing_group, tof_group = self._output_focused_ws([ws for ws_group in focussed_runs for ws in ws_group], run_details)

            common.remove_intermediate_workspace([ws for ws_group in focussed_runs for ws in ws_group])

        return d_spacing_group, tof_group

    def _focus(
        self,
        run_number_string,
        do_van_normalisation,
    ):
        """
        Override parent _focus function, used to focus samples in a specific drange
        :param run_number_string: The run number(s) of the drange
        :param do_van_normalisation: True to divide by the vanadium run, false to not.
        :return:
        """
        self._is_vanadium = False
        return focus.focus(
            run_number_string=run_number_string,
            perform_vanadium_norm=do_van_normalisation,
            instrument=self,
            absorb=False,
        )

    def _setup_drange_sets(self):
        self._drange_sets = osiris_algs.create_drange_sets(self._inst_settings.run_number, self, self._inst_settings.file_extension)

    def _get_run_details(self, run_number_string):
        """
        Returns a RunDetails object with various properties related to the current run set
        :param run_number_string: The run number to look up the properties of
        :return: A RunDetails object containing attributes relevant to that run_number_string
        """
        run_number_string_key = self._generate_run_details_fingerprint(
            run_number_string, self._inst_settings.file_extension, self._is_vanadium
        )

        if run_number_string_key in self._run_details_cached_obj:
            return self._run_details_cached_obj[run_number_string_key]

        drange = self._get_drange_for_run_number(run_number_string)

        self._run_details_cached_obj[run_number_string_key] = osiris_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium, drange=drange
        )

        return self._run_details_cached_obj[run_number_string_key]

    def _get_drange_for_run_number(self, run_number_string):
        for drange_name, drange_object in self._drange_sets.items():
            if drange_object.get_samples_string() == run_number_string:
                return drange_name
        return None

    def apply_drange_cropping(self, run_number_string, focused_ws):
        """
        Apply dspacing range cropping to a focused workspace.
        :param run_number_string: The run number to look up for the drange
        :param focused_ws: The workspace to be cropped
        :return: The cropped workspace in its drange
        """

        drange = self._get_drange_for_run_number(run_number_string)

        return mantid.CropWorkspace(
            InputWorkspace=focused_ws,
            XMin=osiris_algs.d_range_alice[drange][0],
            XMax=osiris_algs.d_range_alice[drange][1],
            OutputWorkspace=str(focused_ws),
        )

    def get_vanadium_path(self, run_details):
        """
        Get the vanadium path from the run details
        :param run_details: The run details of the run number
        :return: the vanadium path
        """

        return run_details.unsplined_vanadium_file_path

    def _output_focused_ws(self, processed_spectra, run_details):
        """
        Takes a list of focused workspace banks and saves them out in an instrument appropriate format.
        :param processed_spectra: The list of workspace banks to save out
        :param run_details: The run details associated with this run
        :param output_mode: Optional - Sets additional saving/grouping behaviour depending on the instrument
        :return: d-spacing group of the processed output workspaces
        """
        d_spacing_group, tof_group, q_squared_group = common_output.split_into_tof_d_spacing_groups(
            run_details=run_details, processed_spectra=processed_spectra, include_q_squared=True
        )
        for unit, group in {"_d_spacing": d_spacing_group, "_tof": tof_group, "_q_squared": q_squared_group}.items():
            save_data(workspace_group=group, output_paths=self._generate_out_file_paths(run_details=run_details, unit=unit))

        return d_spacing_group, tof_group

    def _generate_out_file_paths(self, run_details, unit=""):
        """
        Generates the various output paths and file names to be used during saving or as workspace names
        :param run_details: The run details associated with this run
        :return: A dictionary containing the various output paths and generated output name
        """
        output_directory = os.path.join(self._output_dir, run_details.label, self._user_name)
        output_directory = os.path.abspath(os.path.expanduser(output_directory))
        dat_files_directory = output_directory
        if self._inst_settings.dat_files_directory:
            dat_files_directory = os.path.join(output_directory, self._inst_settings.dat_files_directory)

        out_file_names = {"output_folder": output_directory}
        format_options = {
            "inst": self._inst_prefix,
            "instlow": self._inst_prefix.lower(),
            "instshort": self._inst_prefix_short,
            "runno": run_details.output_run_string,
            "suffix": run_details.output_suffix if run_details.output_suffix else "",
            "unit": unit,
        }
        format_options = self._add_formatting_options(format_options)

        output_formats = {
            "nxs_filename": output_directory,
            "gss_filename": output_directory,
            "xye_filename": dat_files_directory,
        }
        for key, output_dir in output_formats.items():
            filepath = os.path.join(output_dir, getattr(self._inst_settings, key).format(**format_options))
            out_file_names[key] = filepath

        out_file_names["output_name"] = os.path.splitext(os.path.basename(out_file_names["nxs_filename"]))[0]
        return out_file_names

    def should_subtract_empty_inst(self):
        """
        :return: Whether the empty run should be subtracted from a run being focused
        """
        return False


def save_data(workspace_group, output_paths):
    """
    Saves out data into nxs, GSAS and .dat formats. Requires the grouped workspace
    and the dictionary of output paths generated by abstract_inst.
    :param workspace_group: The workspace group
    :param tof_group: The focused workspace group in TOF
    :param output_paths: A dictionary containing the full paths to save to
    :return: None
    """

    def ensure_dir_exists(filename):
        dirname = os.path.dirname(filename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        return filename

    mantid.SaveGSS(InputWorkspace=workspace_group, Filename=ensure_dir_exists(output_paths["gss_filename"]), SplitFiles=False, Append=False)
    mantid.SaveNexusProcessed(InputWorkspace=workspace_group, Filename=ensure_dir_exists(output_paths["nxs_filename"]), Append=False)

    for bank_index, ws in enumerate(workspace_group):
        bank_index += 1  # Ensure we start at 1 when saving out
        mantid.SaveFocusedXYE(
            InputWorkspace=ws,
            Filename=ensure_dir_exists(output_paths["xye_filename"]).format(bankno=bank_index),
            SplitFiles=False,
            IncludeHeader=False,
        )
