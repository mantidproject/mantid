# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from isis_powder.abstract_inst import AbstractInst
from isis_powder.osiris_routines import osiris_advanced_config, osiris_algs, osiris_param_mapping
from isis_powder.routines import instrument_settings, common_output
import copy
import mantid.simpleapi as mantid
import os


class Osiris(AbstractInst):

    def __init__(self, **kwargs):
        self._drange_sets = {
            'drange1': osiris_algs.DrangeData('drange1'),
            'drange2': osiris_algs.DrangeData('drange2'),
            'drange3': osiris_algs.DrangeData('drange3'),
            'drange4': osiris_algs.DrangeData('drange4'),
            'drange5': osiris_algs.DrangeData('drange5'),
            'drange6': osiris_algs.DrangeData('drange6'),
            'drange7': osiris_algs.DrangeData('drange7'),
            'drange8': osiris_algs.DrangeData('drange8'),
            'drange9': osiris_algs.DrangeData('drange9'),
            'drange10': osiris_algs.DrangeData('drange10'),
            'drange11': osiris_algs.DrangeData('drange11'),
            'drange12': osiris_algs.DrangeData('drange12'),
        }
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=osiris_param_mapping.attr_mapping,
            adv_conf_dict=osiris_advanced_config.get_all_adv_variables(),
            kwargs=kwargs)
        self._default_inst_settings = copy.deepcopy(self._inst_settings)

        super(Osiris, self).__init__(user_name=self._inst_settings.user_name,
                                     calibration_dir=self._inst_settings.calibration_dir,
                                     output_dir=self._inst_settings.output_dir,
                                     inst_prefix="OSIRIS")
        self._run_details_cached_obj = {}

    def load_raw_runs(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)
        return self._load_raw_runs(run_details)

    def run_diffraction_focusing(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)
        self._load_raw_runs(run_details)

        focussed_runs = osiris_algs.run_diffraction_focussing(self._inst_settings.run_number,
                                                              self._drange_sets,
                                                              run_details.grouping_file_path,
                                                              van_norm=self._inst_settings.van_norm,
                                                              subtract_empty=self._inst_settings.subtract_empty_can)
        if self._inst_settings.merge_drange:
            focussed_runs = [osiris_algs._merge_dspacing_runs(self._inst_settings.run_number, self._drange_sets, focussed_runs)]

        return self._output_focused_ws(focussed_runs, run_details)

    def _load_raw_runs(self, run_details):
        sample_ws_list = osiris_algs.load_raw(self._inst_settings.run_number, self._drange_sets,
                                              'sample', self, run_details.file_extension)
        van_numbers = osiris_algs.get_van_runs_for_samples(self._inst_settings.run_number,
                                                           self._inst_settings, self._drange_sets)
        vanadium_ws_list = osiris_algs.load_raw(van_numbers, self._drange_sets,
                                                'vanadium', self, run_details.file_extension)
        empty_numbers = osiris_algs.get_empty_runs_for_samples(self._inst_settings.run_number,
                                                               self._inst_settings, self._drange_sets)
        empty_ws_list = osiris_algs.load_raw(empty_numbers, self._drange_sets,
                                             'empty', self, run_details.file_extension)
        return sample_ws_list, vanadium_ws_list, empty_ws_list

    def _get_run_details(self, run_number_string):
        """
        Returns a RunDetails object with various properties related to the current run set
        :param run_number_string: The run number to look up the properties of
        :return: A RunDetails object containing attributes relevant to that run_number_string
        """
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string,
                                                                       self._inst_settings.file_extension)

        if run_number_string_key in self._run_details_cached_obj:
            return self._run_details_cached_obj[run_number_string_key]

        self._run_details_cached_obj[run_number_string_key] = osiris_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium)

        return self._run_details_cached_obj[run_number_string_key]

    def _output_focused_ws(self, processed_spectra, run_details):
        """
        Takes a list of focused workspace banks and saves them out in an instrument appropriate format.
        :param processed_spectra: The list of workspace banks to save out
        :param run_details: The run details associated with this run
        :param output_mode: Optional - Sets additional saving/grouping behaviour depending on the instrument
        :return: d-spacing group of the processed output workspaces
        """
        d_spacing_group, tof_group, q_squared_group = common_output.split_into_tof_d_spacing_groups(
            run_details=run_details, processed_spectra=processed_spectra, include_q_squared=True)
        for unit, group in {"_d_spacing": d_spacing_group, "_tof": tof_group, "_q_squared": q_squared_group}.items():
            save_data(workspace_group=group,
                      output_paths=self._generate_out_file_paths(run_details=run_details, unit=unit))

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
            dat_files_directory = os.path.join(output_directory,
                                               self._inst_settings.dat_files_directory)

        out_file_names = {"output_folder": output_directory}
        format_options = {
            "inst": self._inst_prefix,
            "instlow": self._inst_prefix.lower(),
            "instshort": self._inst_prefix_short,
            "runno": run_details.output_run_string,
            "suffix": run_details.output_suffix if run_details.output_suffix else "",
            "unit": unit
        }
        format_options = self._add_formatting_options(format_options)

        output_formats = {
            "nxs_filename": output_directory,
            "gss_filename": output_directory,
            "xye_filename": dat_files_directory,
        }
        for key, output_dir in output_formats.items():
            filepath = os.path.join(output_dir,
                                    getattr(self._inst_settings, key).format(**format_options))
            out_file_names[key] = filepath

        out_file_names['output_name'] = os.path.splitext(
            os.path.basename(out_file_names['nxs_filename']))[0]
        return out_file_names


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

    mantid.SaveGSS(InputWorkspace=workspace_group,
                   Filename=ensure_dir_exists(output_paths["gss_filename"]),
                   SplitFiles=False,
                   Append=False)
    mantid.SaveNexusProcessed(InputWorkspace=workspace_group,
                              Filename=ensure_dir_exists(output_paths["nxs_filename"]),
                              Append=False)

    for bank_index, ws in enumerate(workspace_group):
        bank_index += 1  # Ensure we start at 1 when saving out
        mantid.SaveFocusedXYE(InputWorkspace=ws,
                              Filename=ensure_dir_exists(output_paths["xye_filename"]).format(bankno=bank_index),
                              SplitFiles=False,
                              IncludeHeader=False)
