from __future__ import (absolute_import, division, print_function)

from isis_powder.abstract_inst import AbstractInst
from isis_powder.gem_routines import gem_advanced_config, gem_algs, gem_param_mapping
from isis_powder.routines import InstrumentSettings, yaml_parser


class Gem(AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))

        self._inst_settings = InstrumentSettings.InstrumentSettings(
            attr_mapping=gem_param_mapping.attr_mapping, adv_conf_dict=gem_advanced_config.get_all_adv_variables(),
            kwargs=kwargs, basic_conf_dict=basic_config_dict)

        super(Gem, self).__init__(user_name=self._inst_settings.user_name,
                                  calibration_dir=self._inst_settings.calibration_dir,
                                  output_dir=self._inst_settings.output_dir)

        self._cached_run_details = None
        self._cached_run_number = None

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(run_number_string=self._inst_settings.run_number,
                           do_van_normalisation=self._inst_settings.van_norm)

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
        raise NotImplementedError()

    @staticmethod
    def _generate_input_file_name(run_number):
        raise NotImplementedError()

    def _apply_absorb_corrections(self, run_details, van_ws):
        raise NotImplementedError()

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        raise NotImplementedError()

