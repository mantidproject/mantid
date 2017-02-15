from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.abstract_inst import AbstractInst
from isis_powder.gem_routines import gem_advanced_config, gem_param_mapping
from isis_powder.routines import InstrumentSettings, yaml_parser


class Gem(AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))

        self._inst_settings = InstrumentSettings.InstrumentSettings(
            attr_mapping=gem_param_mapping.attr_mapping, adv_conf_dict=gem_advanced_config,
            kwargs=kwargs, basic_conf_dict=basic_config_dict)

        super(Gem, self).__init__(user_name=self._inst_settings.user_name,
                                  calibration_dir=self._inst_settings.calibration_dir,
                                  output_dir=self._inst_settings.output_dir)

        self._cached_run_details = None
        self._cached_run_number = None
        raise NotImplementedError()

    def focus(self, **kwargs):
        raise NotImplementedError()

    def create_vanadium(self, **kwargs):
        raise NotImplementedError()

    def _get_run_details(self, run_number_string):
        pass

    def _generate_auto_vanadium_calibration(self, run_details):
        pass

    def _generate_output_file_name(self, run_number_string):
        pass

    @staticmethod
    def _generate_input_file_name(run_number):
        pass

    def _apply_absorb_corrections(self, run_details, van_ws, gen_absorb=False):
        pass

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        pass

