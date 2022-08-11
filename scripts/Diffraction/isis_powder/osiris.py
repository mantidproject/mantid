# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from isis_powder.abstract_inst import AbstractInst
from isis_powder.osiris_routines import osiris_advanced_config, osiris_algs, osiris_param_mapping
from isis_powder.routines import instrument_settings
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

    def load_raw_runs(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        run_details = self._get_run_details(run_number_string=self._inst_settings.run_number)
        sample_ws_list = osiris_algs.load_raw(self._inst_settings.run_number, self._drange_sets,
                                              'sample', self, run_details.file_extension)
        vanadium_ws_list = osiris_algs.load_raw(run_details.vanadium_run_numbers, self._drange_sets,
                                                'vanadium', self, run_details.file_extension)
        empty_ws_list = osiris_algs.load_raw(run_details.empty_inst_runs, self._drange_sets,
                                             'empty', self, run_details.file_extension)
        return sample_ws_list, vanadium_ws_list, empty_ws_list

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
            focussed_runs = [osiris_algs._merge_dspacing_runs(self._inst_settings.run_number, self._drange_sets, focussed_runs)]

        return self._output_focused_ws(focussed_runs, run_details)

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string,
                                                                       self._inst_settings.file_extension)

        if run_number_string_key in self._run_details_cached_obj:
            return self._run_details_cached_obj[run_number_string_key]

        self._run_details_cached_obj[run_number_string_key] = osiris_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium)

        return self._run_details_cached_obj[run_number_string_key]
