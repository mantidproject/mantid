# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from isis_powder.abstract_inst import AbstractInst
from isis_powder.osiris_routines import osiris_advanced_config, osiris_algs, osiris_param_mapping
from isis_powder.routines import common, instrument_settings
from mantid.simpleapi import GroupWorkspaces
import copy


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

    def load_sample_runs(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._get_run_details(run_number_string=self._inst_settings.run_number)
        input_ws_list = self._load_runs(sample_runs=self._inst_settings.run_number)

        GroupWorkspaces(InputWorkspaces=input_ws_list,
                        outputWorkspace='OSIRIS' + self._inst_settings.run_number + '_grouped')
        osiris_algs.get_vanadium_runs(self._drange_sets, self._inst_settings)
        osiris_algs.get_empty_runs(self._drange_sets, self._inst_settings)
        return input_ws_list

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)
        osiris_algs.load_raw(self._inst_settings.run_number, self._drange_sets,
                             'sample', self, run_details.file_extension)
        osiris_algs.load_raw(run_details.vanadium_run_numbers, self._drange_sets,
                             'vanadium', self, run_details.file_extension)
        osiris_algs.load_raw(run_details.empty_inst_runs, self._drange_sets,
                             'empty', self, run_details.file_extension)

        vanadium_d = self._create_vanadium(run_number_string=self._inst_settings.run_in_range,
                                           do_absorb_corrections=self._inst_settings.absorb_corrections)
        return vanadium_d

    def run_diffraction_focusing(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)
        osiris_algs.load_raw(self._inst_settings.run_number, self._drange_sets,
                             'sample', self, run_details.file_extension)
        osiris_algs.load_raw(run_details.vanadium_run_numbers, self._drange_sets,
                             'vanadium', self, run_details.file_extension)
        osiris_algs.load_raw(run_details.empty_inst_runs, self._drange_sets,
                             'empty', self, run_details.file_extension)

        focussed_runs = osiris_algs.run_diffraction_focussing(self._inst_settings.run_number,
                                                              self._drange_sets,
                                                              run_details.grouping_file_path,
                                                              van_norm=self._inst_settings.van_norm,
                                                              subtract_empty=self._inst_settings.subtract_empty_inst)
        if self._inst_settings.merge_drange:
            osiris_algs.merge_dspacing_runs(self._inst_settings.run_number, self._drange_sets, focussed_runs)

    def _load_runs(self, sample_runs=None, van_runs=None, empty_runs=None):
        if sample_runs:
            return self._load_raw(sample_runs, 'sample')
        if van_runs:
            return self._load_raw(van_runs, 'vanadium')
        if empty_runs:
            return self._load_raw(empty_runs, 'empty')

    def _load_raw(self, run_number_string, group):
        """
        Create a summed run of Osiris data across multiple dranges
        :param run_number_string: string of run numbers for the sample
        :param instrument: The OSIRIS instrument object
        :param grouping_file_name: Name of grouping calibration file
        """
        file_ext = self._get_run_details(run_number_string=run_number_string).file_extension
        input_ws_list = common.load_raw_files(run_number_string=run_number_string, instrument=self, file_ext=file_ext)

        for ws in input_ws_list:
            drange = osiris_algs.get_osiris_d_range(ws)
            if group == 'sample':
                self._drange_sets[drange].add_sample(ws.name())
            elif group == 'vanadium':
                self._drange_sets[drange].set_vanadium(ws.name())
            elif group == 'empty':
                self._drange_sets[drange].set_empty(ws.name())
        return input_ws_list

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string,
                                                                       self._inst_settings.file_extension)

        if run_number_string_key in self._run_details_cached_obj:
            return self._run_details_cached_obj[run_number_string_key]

        self._run_details_cached_obj[run_number_string_key] = osiris_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium)

        return self._run_details_cached_obj[run_number_string_key]

    def get_sample_strings(self):
        sample_strings = []
        for drange in self._drange_sets:
            sample_strings += self._drange_sets[drange].get_samples()
        return sample_strings

    def get_vanadium_strings(self):
        van_strings = []
        for drange in self._drange_sets:
            van_strings.append(self._drange_sets[drange].get_vanadium())
        return van_strings

    def get_empty_strings(self):
        empty_strings = []
        for drange in self._drange_sets:
            empty_strings.append(self._drange_sets[drange].get_empty())
        return empty_strings
