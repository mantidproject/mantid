from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import common, InstrumentSettings, yaml_parser
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_algs, pearl_output, pearl_spline, pearl_advanced_config


class Pearl(AbstractInst):
    def __init__(self, **kwargs):
        expected_attr = ["user_name", "config_file", "calibration_dir", "output_dir", "attenuation_file_name",
                         "calibration_mapping_file"]
        import pydevd
        pydevd.settrace('localhost', port=51205, stdoutToServer=True, stderrToServer=True)
        # Parse all possible locations that the parameters can be set from
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))
        self._inst_settings = InstrumentSettings.InstrumentSettings(
                              attr_mapping_dict=self.attr_mapping, adv_conf_dict=pearl_advanced_config.variables,
                              basic_conf_dict=basic_config_dict, kwargs=kwargs)

        self._inst_settings.check_expected_attributes_are_set(attr_mapping=self.attr_mapping,
                                                              expected_attr_names=expected_attr)

        super(Pearl, self).__init__(user_name=self._inst_settings.user_name,
                                    calibration_dir=self._inst_settings.calibration_dir,
                                    output_dir=self._inst_settings.output_dir)

        self._ads_workaround = 0
        self._cached_run_details = None
        self._cached_run_details_number = None

    def focus(self, run_number, **kwargs):
        return self._focus(run_number=run_number, input_batching=InputBatchingEnum.Summed,
                           do_van_normalisation=self._run_settings.divide_by_vanadium)

    def create_calibration_vanadium(self, run_in_range, **kwargs):
        kwargs["tt_mode"] = "tt88"
        self._inst_settings.update_attributes_from_kwargs(attr_mapping_dict=self.attr_mapping, kwargs=kwargs)
        expected_attr = ["long_mode", "van_norm", "absorb_corrections"]
        self._inst_settings.check_expected_attributes_are_set(attr_mapping=self.attr_mapping,
                                                              expected_attr_names=expected_attr)

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

        # TODO justt pass in inst settings instead
        run_details = pearl_algs.get_run_details(absorb_on=self._inst_settings.absorb_corrections,
                                                 long_mode_on=self._inst_settings.long_mode,
                                                 run_number_string=run_number_string,
                                                 calibration_dir=self._calibration_dir,
                                                 mapping_file=self._inst_settings.calibration_mapping_file)

        run_details = pearl_algs.set_advanced_run_details(run_details=run_details, tt_mode=self._inst_settings.tt_mode,
                                                          calibration_dir=self._calibration_dir)
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
        return pearl_spline.spline_vanadium_for_focusing(focused_vanadium_spectra=focused_vanadium_spectra,
                                                         num_splines=self._run_settings.number_of_splines)

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        if not output_mode:
            output_mode = self._run_settings.tt_mode
        output_spectra = \
            pearl_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                        run_details=run_details, focus_mode=output_mode,
                                                        perform_attenuation=self._inst_settings.perform_atten)
        group_name = "PEARL" + str(run_details.run_number) + "-Results-D-Grp"
        grouped_d_spacing = mantid.GroupWorkspaces(InputWorkspaces=output_spectra, OutputWorkspace=group_name)
        return grouped_d_spacing

    def crop_to_sane_tof(self, ws_to_crop):
        out_ws = common.crop_in_tof(ws_to_rebin=ws_to_crop, x_min=1000, x_max=19900)
        return out_ws

    def generate_vanadium_absorb_corrections(self, run_details, ws_to_match):
        return pearl_algs.generate_vanadium_absorb_corrections(van_ws=ws_to_match)

    def correct_sample_vanadium(self, focus_spectra, vanadium_spectra=None):
        data_ws = mantid.ConvertUnits(InputWorkspace=focus_spectra, Target="TOF")
        data_ws = mantid.Rebin(InputWorkspace=data_ws, Params=self._focus_tof_binning)

        if vanadium_spectra:
            # Workaround for Mantid overwriting the WS in a loop
            data_processed = "van_processed" + str(self._ads_workaround)
            vanadium_ws = mantid.Rebin(InputWorkspace=vanadium_spectra, Params=self._focus_tof_binning)
            data_ws = mantid.Divide(LHSWorkspace=data_ws, RHSWorkspace=vanadium_ws, OutputWorkspace=data_processed)
        else:
            data_processed = "processed-" + str(self._ads_workaround)

        self._ads_workaround += 1
        mantid.CropWorkspace(InputWorkspace=data_ws, XMin=0.1, OutputWorkspace=data_processed)

        if vanadium_spectra:
            mantid.Scale(InputWorkspace=data_processed, Factor=10, OutputWorkspace=data_processed)

        return data_processed

    # Maps parameter/config name -> script names
    attr_mapping = [("absorb_corrections", "absorb_corrections"),
                    ("attenuation_file_name", "attenuation_file_name"),
                    ("config_file", "config_file_name"),
                    ("calibration_config_file", "calibration_mapping_file"),
                    ("calibration_directory", "calibration_dir"),
                    ("long_mode", "long_mode"),
                    ("tt_mode", "tt_mode"),
                    ("output_directory", "output_dir"),
                    ("perform_atten", "perform_attenuation"),
                    ("user_name", "user_name"),
                    ("vanadium_normalisation", "van_norm")]


def _generate_file_name(run_number):
    digit = len(str(run_number))

    number_of_digits = 8
    filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)
    return filename
