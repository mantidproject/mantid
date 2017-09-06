from __future__ import (absolute_import, division, print_function)

from isis_powder.abstract_inst import AbstractInst
from isis_powder.gem_routines import gem_advanced_config, gem_algs, gem_param_mapping
from isis_powder.routines import absorb_corrections, common, instrument_settings


class Gem(AbstractInst):
    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=gem_param_mapping.attr_mapping, adv_conf_dict=gem_advanced_config.get_all_adv_variables(),
            kwargs=kwargs)

        super(Gem, self).__init__(user_name=self._inst_settings.user_name,
                                  calibration_dir=self._inst_settings.calibration_dir,
                                  output_dir=self._inst_settings.output_dir, inst_prefix="GEM")

        self._cached_run_details = {}
        self._sample_details = None

    # Public API

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(
            run_number_string=self._inst_settings.run_number, do_van_normalisation=self._inst_settings.do_van_norm,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)

        return self._create_vanadium(run_number_string=self._inst_settings.run_in_range,
                                     do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def set_sample_details(self, **kwargs):
        kwarg_name = "sample"
        sample_details_obj = common.dictionary_key_helper(
            dictionary=kwargs, key=kwarg_name,
            exception_msg="The argument containing sample details was not found. Please"
                          " set the following argument: " + kwarg_name)
        self._sample_details = sample_details_obj

    # Private methods

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string,
                                                                       self._inst_settings.file_extension)
        if run_number_string_key in self._cached_run_details:
            return self._cached_run_details[run_number_string_key]

        self._cached_run_details[run_number_string_key] = gem_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium)
        return self._cached_run_details[run_number_string_key]

    def _generate_output_file_name(self, run_number_string):
        return self._generate_input_file_name(run_number_string)

    @staticmethod
    def _generate_input_file_name(run_number):
        return _gem_generate_inst_name(run_number=run_number)

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        if self._is_vanadium:
            return gem_algs.calculate_van_absorb_corrections(
                ws_to_correct=ws_to_correct, multiple_scattering=self._inst_settings.multiple_scattering)
        else:
            return absorb_corrections.run_cylinder_absorb_corrections(
                ws_to_correct=ws_to_correct, multiple_scattering=self._inst_settings.multiple_scattering,
                sample_details_obj=self._sample_details)

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.focused_cropping_values)

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        raw_cropping_values = self._inst_settings.raw_tof_cropping_values
        return common.crop_in_tof(ws_to_crop, raw_cropping_values[0], raw_cropping_values[1])

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_banks_using_crop_list(van_ws_to_crop, self._inst_settings.vanadium_cropping_values)

    def _get_input_batching_mode(self):
        return self._inst_settings.input_batching

    def _get_unit_to_keep(self):
        return self._inst_settings.unit_to_keep

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        return common.spline_vanadium_workspaces(focused_vanadium_spectra=focused_vanadium_banks,
                                                 spline_coefficient=self._inst_settings.spline_coeff)


def _gem_generate_inst_name(run_number):
    if isinstance(run_number, list):
        # Use recursion on lists
        updated_list = []
        for run in run_number:
            updated_list.append(_gem_generate_inst_name(run))
        return updated_list
    else:
        # Individual entry
        return "GEM" + str(run_number)
