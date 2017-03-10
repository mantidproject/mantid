from __future__ import (absolute_import, division, print_function)

from isis_powder.abstract_inst import AbstractInst
from isis_powder.gem_routines import gem_advanced_config, gem_algs, gem_param_mapping
from isis_powder.routines import common, InstrumentSettings, yaml_parser


class Gem(AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))

        self._inst_settings = InstrumentSettings.InstrumentSettings(
            param_map=gem_param_mapping.attr_mapping, adv_conf_dict=gem_advanced_config.get_all_adv_variables(),
            kwargs=kwargs, basic_conf_dict=basic_config_dict)

        super(Gem, self).__init__(user_name=self._inst_settings.user_name,
                                  calibration_dir=self._inst_settings.calibration_dir,
                                  output_dir=self._inst_settings.output_dir, inst_prefix="GEM")

        self._cached_run_details = None
        self._cached_run_number = None

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(run_number_string=self._inst_settings.run_number,
                           do_van_normalisation=self._inst_settings.do_van_norm)

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        # First get a run_details object to find out the vanadium number
        run_details = self._get_run_details(run_number_string=self._inst_settings.run_in_range)
        # Set the run and vanadium run equal
        run_details.run_number = run_details.vanadium_run_numbers

        return self._create_vanadium(run_details=run_details,
                                     do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def _get_run_details(self, run_number_string):
        return gem_algs.get_run_details(run_number_string=run_number_string, inst_settings=self._inst_settings)

    def _generate_auto_vanadium_calibration(self, run_details):
        raise NotImplementedError()

    def _generate_output_file_name(self, run_number_string):
        return self._generate_input_file_name(run_number_string)

    @staticmethod
    def _generate_input_file_name(run_number):
        return _gem_generate_inst_name(run_number=run_number)

    def _apply_absorb_corrections(self, run_details, van_ws):
        return gem_algs.calculate_absorb_corrections(ws_to_correct=van_ws,
                                                     multiple_scattering=self._inst_settings.multiple_scattering)

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_in_tof(focused_banks, self._inst_settings.focused_cropping_values)

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        raw_cropping_values = self._inst_settings.raw_tof_cropping_values
        return common.crop_in_tof(ws_to_crop, raw_cropping_values[0], raw_cropping_values[1])

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_banks_in_tof(van_ws_to_crop, self._inst_settings.vanadium_cropping_values)

    def _get_sample_empty(self):
        sample_empty = self._inst_settings.sample_empty
        if sample_empty:
            raise NotImplementedError("Subtracting s-empty is not implemented yet.")
        return sample_empty

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
