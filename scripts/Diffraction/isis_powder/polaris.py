from __future__ import (absolute_import, division, print_function)

import os

from isis_powder.routines import common, InstrumentSettings, yaml_parser
from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_advanced_config, polaris_algs, polaris_param_mapping


class Polaris(AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))
        self._inst_settings = InstrumentSettings.InstrumentSettings(
            param_map=polaris_param_mapping.attr_mapping, adv_conf_dict=polaris_advanced_config.variables,
            basic_conf_dict=basic_config_dict, kwargs=kwargs)

        super(Polaris, self).__init__(user_name=self._inst_settings.user_name,
                                      calibration_dir=self._inst_settings.calibration_dir,
                                      output_dir=self._inst_settings.output_dir, inst_prefix="POL")

        # Hold the last dictionary later to avoid us having to keep parsing the YAML
        self._run_details_last_run_number = None
        self._run_details_cached_obj = None

        self._ads_workaround = 0

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(run_number_string=self._inst_settings.run_number,
                           do_van_normalisation=self._inst_settings.do_van_normalisation)

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        run_details = self._get_run_details(run_number_string=int(self._inst_settings.run_in_range))
        run_details.run_number = run_details.vanadium_run_numbers

        return self._create_vanadium(run_details=run_details,
                                     do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    # Overrides
    def _apply_absorb_corrections(self, run_details, van_ws):
        return polaris_algs.calculate_absorb_corrections(ws_to_correct=van_ws,
                                                         multiple_scattering=self._inst_settings.multiple_scattering)

    @staticmethod
    def _can_auto_gen_vanadium_cal():
        return True

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_in_tof(focused_banks, self._inst_settings.focused_cropping_values)

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        cropped_ws = common.crop_in_tof(ws_to_crop=ws_to_crop, x_min=self._inst_settings.raw_data_crop_values[0],
                                        x_max=self._inst_settings.raw_data_crop_values[1])
        return cropped_ws

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        cropped_ws = common.crop_banks_in_tof(bank_list=van_ws_to_crop,
                                              crop_values_list=self._inst_settings.van_crop_values)
        return cropped_ws

    def _generate_auto_vanadium_calibration(self, run_details):
        self.create_vanadium(run_in_range=run_details.run_number)

    @staticmethod
    def _generate_input_file_name(run_number):
        polaris_old_name = "POL"
        polaris_new_name = "POLARIS"
        first_run_new_name = 96912

        if isinstance(run_number, list):
            # Lists use recursion to deal with individual entries
            updated_list = []
            for run in run_number:
                updated_list.append(Polaris._generate_input_file_name(run_number=run))
            return updated_list
        else:
            # Select between old and new prefix
            # Test if it can be converted to an int or if we need to ask Mantid to do it for us
            if isinstance(run_number, str) and not run_number.isdigit():
                # Convert using Mantid and take the first element which is most likely to be the lowest digit
                use_new_name = True if int(common.generate_run_numbers(run_number)[0]) >= first_run_new_name else False
            else:
                use_new_name = True if int(run_number) >= first_run_new_name else False

            prefix = polaris_new_name if use_new_name else polaris_old_name
            return prefix + str(run_number)

    def _generate_output_file_name(self, run_number_string):
        return self._generate_input_file_name(run_number=run_number_string)

    def _get_input_batching_mode(self):
        return self._inst_settings.input_mode

    def _get_run_details(self, run_number_string):
        if self._run_details_last_run_number == run_number_string:
            return self._run_details_cached_obj

        run_details = polaris_algs.get_run_details(run_number_string=run_number_string,
                                                   inst_settings=self._inst_settings)

        # Hold obj in case same run range is requested
        self._run_details_last_run_number = run_number_string
        self._run_details_cached_obj = run_details

        return run_details

    def _spline_vanadium_ws(self, focused_vanadium_spectra, instrument_version=''):
        masking_file_name = self._inst_settings.masking_file_name
        spline_coeff = self._inst_settings.spline_coeff
        masking_file_path = os.path.join(self.calibration_dir, masking_file_name)
        output = polaris_algs.process_vanadium_for_focusing(bank_spectra=focused_vanadium_spectra,
                                                            spline_number=spline_coeff,
                                                            mask_path=masking_file_path)
        return output
