from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines import yaml_parser, InstrumentSettings
from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_advanced_config, polaris_algs, polaris_output, polaris_param_mapping


class Polaris(AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))
        self._inst_settings = InstrumentSettings.InstrumentSettings(
            attr_mapping=polaris_param_mapping.attr_mapping, adv_conf_dict=polaris_advanced_config.variables,
            basic_conf_dict=basic_config_dict, kwargs=kwargs)

        super(Polaris, self).__init__(user_name=self._inst_settings.user_name,
                                      calibration_dir=self._inst_settings.calibration_dir,
                                      output_dir=self._inst_settings.output_dir)

        # Hold the last dictionary later to avoid us having to keep parsing the YAML
        self._run_details_last_run_number = None
        self._run_details_cached_obj = None

        self._ads_workaround = 0

    def focus(self, run_number, input_mode, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(run_number=run_number, input_batching=input_mode,
                           do_van_normalisation=self._inst_settings.do_van_normalisation)

    def create_calibration_vanadium(self, run_in_range, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)

        run_details = self.get_run_details(run_number_string=int(run_in_range))
        run_details.run_number = run_details.vanadium_run_numbers

        return self._create_calibration_vanadium(
            vanadium_runs=run_details.vanadium_run_numbers, empty_runs=run_details.empty_runs,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections,
            gen_absorb_correction=self._inst_settings.gen_absorb_corrections)

    def get_run_details(self, run_number_string):
        input_run_number_list = common.generate_run_numbers(run_number_string=run_number_string)
        first_run = input_run_number_list[0]
        if self._run_details_last_run_number == first_run:
            return self._run_details_cached_obj

        run_details = polaris_algs.get_run_details(run_number=first_run, inst_settings=self._inst_settings)

        # Hold obj in case same run range is requested
        self._run_details_last_run_number = first_run
        self._run_details_cached_obj = run_details

        return run_details

    @staticmethod
    def generate_input_file_name(run_number):
        if isinstance(run_number, list):
            updated_list = ["POL" + str(val) for val in run_number]
            return updated_list
        else:
            return "POL" + str(run_number)

    def generate_output_file_name(self, run_number_string):
        return self.generate_input_file_name(run_number=run_number_string)

    @staticmethod
    def can_auto_gen_vanadium_cal():
        return True

    def normalise_ws_current(self, ws_to_correct, run_details=None):
        normalised_ws = mantid.NormaliseByCurrent(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct)
        return normalised_ws

    def apply_solid_angle_efficiency_corr(self, ws_to_correct, run_details):
        if not self._inst_settings.solid_angle_on:
            return ws_to_correct

        corrections = polaris_algs.generate_solid_angle_corrections(run_details=run_details, instrument=self)
        corrected_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=corrections)
        common.remove_intermediate_workspace(corrections)
        common.remove_intermediate_workspace(ws_to_correct)
        ws_to_correct = corrected_ws
        return ws_to_correct

    def spline_vanadium_ws(self, focused_vanadium_spectra, instrument_version=''):
        masking_file_name = self._inst_settings.masking_file_name
        spline_coeff = self._inst_settings.spline_coeff
        masking_file_path = os.path.join(self.calibration_dir, masking_file_name)
        output = polaris_algs.process_vanadium_for_focusing(bank_spectra=focused_vanadium_spectra,
                                                            spline_number=spline_coeff,
                                                            mask_path=masking_file_path)
        return output

    def generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        return polaris_algs.generate_absorb_corrections(ws_to_match=ws_to_match)

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        d_spacing_group, tof_group = polaris_algs.split_into_tof_d_spacing_groups(processed_spectra)
        output_paths = self.generate_out_file_paths(run_details=run_details)

        polaris_output.save_polaris_focused_data(d_spacing_group=d_spacing_group, tof_group=tof_group,
                                                 output_paths=output_paths, run_number=run_details.run_number)

        return d_spacing_group

    def crop_raw_to_expected_tof_range(self, ws_to_crop):
        cropped_ws = common.crop_in_tof(ws_to_crop=ws_to_crop, x_min=self._inst_settings.raw_data_crop_values[0],
                                        x_max=self._inst_settings.raw_data_crop_values[1])
        return cropped_ws

    def crop_van_to_expected_tof_range(self, van_ws_to_crop):
        cropped_ws = common.crop_in_tof(ws_to_crop=van_ws_to_crop, x_min=self._inst_settings.van_crop_values[0],
                                        x_max=self._inst_settings.van_crop_values[-1])
        return cropped_ws

    def crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_in_tof(focused_banks, self._inst_settings.tof_cropping_values)

    def generate_auto_vanadium_calibration(self, run_details):
        self.create_calibration_vanadium(run_in_range=run_details.run_number)
