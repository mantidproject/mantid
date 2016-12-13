from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import common, InstrumentSettings, yaml_parser
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_algs, pearl_output, pearl_advanced_config, pearl_param_mapping


class Pearl(AbstractInst):
    def __init__(self, **kwargs):
        expected_attr = ["user_name", "config_file_name", "calibration_dir", "output_dir", "attenuation_file_name",
                         "cal_map_path", "van_absorb_file"]
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))
        self._inst_settings = InstrumentSettings.InstrumentSettings(
           attr_mapping=pearl_param_mapping.attr_mapping, adv_conf_dict=pearl_advanced_config.variables,
           basic_conf_dict=basic_config_dict, kwargs=kwargs)

        self._inst_settings.check_expected_attributes_are_set(expected_attr_names=expected_attr)

        super(Pearl, self).__init__(user_name=self._inst_settings.user_name,
                                    calibration_dir=self._inst_settings.calibration_dir,
                                    output_dir=self._inst_settings.output_dir)

        self._ads_workaround = 0
        self._cached_run_details = None
        self._cached_run_details_number = None

    def focus(self, run_number, **kwargs):
        self._inst_settings.update_attributes_from_kwargs(kwargs=kwargs)
        expected_attr = ["absorb_corrections", "long_mode", "tt_mode", "perform_atten", "van_norm"]
        self._inst_settings.check_expected_attributes_are_set(expected_attr_names=expected_attr)

        return self._focus(run_number=run_number, input_batching=InputBatchingEnum.Summed,
                           do_van_normalisation=self._inst_settings.van_norm)

    def create_calibration_vanadium(self, run_in_range, **kwargs):
        kwargs["tt_mode"] = "tt88"
        kwargs["perform_attenuation"] = False
        self._inst_settings.update_attributes_from_kwargs(kwargs=kwargs)
        expected_attr = ["long_mode", "van_norm", "absorb_corrections"]
        self._inst_settings.check_expected_attributes_are_set(expected_attr_names=expected_attr)

        run_details = self.get_run_details(run_number_string=int(run_in_range))
        run_details.run_number = run_details.vanadium_run_numbers

        return self._create_calibration_vanadium(vanadium_runs=run_details.vanadium_run_numbers,
                                                 empty_runs=run_details.empty_runs,
                                                 do_absorb_corrections=self._inst_settings.absorb_corrections)

    # Params #
    def get_default_group_names(self):
        return self._default_group_names

    def _get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def get_run_details(self, run_number_string):
        input_run_number_list = common.generate_run_numbers(run_number_string=run_number_string)
        first_run = input_run_number_list[0]
        if self._cached_run_details_number == first_run:
            return self._cached_run_details

        run_details = pearl_algs.get_run_details(run_number_string=run_number_string, inst_settings=self._inst_settings)

        self._cached_run_details_number = first_run
        self._cached_run_details = run_details
        return run_details

    @staticmethod
    def generate_inst_file_name(run_number):
        return _generate_file_name(run_number=run_number)

    # Hook overrides

    def attenuate_workspace(self, input_workspace):
        attenuation_path = self._attenuation_full_path
        return pearl_algs.attenuate_workspace(attenuation_file_path=attenuation_path, ws_to_correct=input_workspace)

    def normalise_ws(self, ws_to_correct, run_details=None):
        if not run_details:
            raise RuntimeError("Run details was not passed into PEARL: normalise_ws")
        monitor_ws = common.get_monitor_ws(ws_to_process=ws_to_correct, run_number_string=run_details.run_number,
                                           instrument=self)
        normalised_ws = pearl_algs.normalise_ws_current(ws_to_correct=ws_to_correct, monitor_ws=monitor_ws,
                                                        spline_coeff=20)
        common.remove_intermediate_workspace(monitor_ws)
        return normalised_ws

    def get_monitor_spectra_index(self, run_number):
        return 1

    def spline_vanadium_ws(self, focused_vanadium_spectra):
        return common.spline_vanadium_for_focusing(focused_vanadium_spectra=focused_vanadium_spectra,
                                                   num_splines=self._inst_settings.spline_coefficient)

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        if not output_mode:
            output_mode = self._inst_settings.focus_mode
        output_spectra = \
            pearl_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                        run_details=run_details, focus_mode=output_mode,
                                                        perform_attenuation=self._inst_settings.perform_atten)
        group_name = "PEARL" + str(run_details.run_number) + "-Results-D-Grp"
        grouped_d_spacing = mantid.GroupWorkspaces(InputWorkspaces=output_spectra, OutputWorkspace=group_name)
        return grouped_d_spacing

    def crop_short_long_mode(self, ws_to_crop):
        out_ws = common.crop_in_tof(ws_to_rebin=ws_to_crop, x_max=19900)
        return out_ws

    def generate_vanadium_absorb_corrections(self, run_details, ws_to_match):
        return pearl_algs.generate_vanadium_absorb_corrections(van_ws=ws_to_match)


def _generate_file_name(run_number):
    digit = len(str(run_number))

    number_of_digits = 8
    filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)
    return filename
