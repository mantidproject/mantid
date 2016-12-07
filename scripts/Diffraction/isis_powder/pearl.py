from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid

from isis_powder.routines import common, yaml_parser
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_algs, pearl_output, pearl_spline, PearlRunSettings


class Pearl(AbstractInst):

    # Instrument default settings
    _default_input_ext = '.raw'
    _default_group_names = "bank1,bank2,bank3,bank4"

    _focus_tof_binning = "1500,-0.0006,19900"

    _create_van_tof_binning = "1500,-0.0006,19900"

    def __init__(self, user_name, config_file=None, **kwargs):

        expected_keys = ["calibration_directory", "output_directory", "attenuation_file_name",
                         "calibration_mapping_file"]
        yaml_parser.set_kwargs_from_config_file(config_path=config_file, kwargs=kwargs, keys_to_find=expected_keys)

        super(Pearl, self).__init__(user_name=user_name, calibration_dir=kwargs["calibration_directory"],
                                    output_dir=kwargs["output_directory"])

        self._basic_config_file_path = config_file
        self._calibration_mapping_path = kwargs["calibration_mapping_file"]
        attenuation_file_name = kwargs["attenuation_file_name"]  # "PRL112_DC25_10MM_FF.OUT"
        self._attenuation_full_path = os.path.join(self._calibration_dir, attenuation_file_name)

        self._run_settings = None
        self._ads_workaround = 0

    def focus(self, run_number, **kwargs):
        self._run_settings = _get_settings_focus_kwargs(config_file_path=self._basic_config_file_path, kwargs=kwargs)
        return self._focus(run_number=run_number, input_batching=InputBatchingEnum.Summed,
                           do_van_normalisation=self._run_settings.divide_by_vanadium)

    def create_calibration_vanadium(self, vanadium_runs, empty_runs, **kwargs):
        self._run_settings = _get_settings_van_calib_kwargs(config_file_path=self._basic_config_file_path,
                                                            kwargs=kwargs)
        self._run_settings.number_of_splines = kwargs.get("num_of_splines", 60)

        return self._create_calibration_vanadium(vanadium_runs=vanadium_runs, empty_runs=empty_runs,
                                                 do_absorb_corrections=self._run_settings.absorption_corrections)

    # Params #
    def get_default_group_names(self):
        return self._default_group_names

    def _get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def get_run_details(self, run_number):
        # TODO once we migrate this to another format (i.e. not the if/elif/else) implement cached val
        run_settings = self._run_settings
        run_details = pearl_algs.get_run_details(absorb_on=run_settings.absorption_corrections,
                                                 long_mode_on=run_settings.divide_by_vanadium,
                                                 run_number_string=run_number,
                                                 calibration_dir=self._calibration_dir,
                                                 mapping_file=self._calibration_mapping_path)

        run_details = pearl_algs.set_advanced_run_details(run_details=run_details, tt_mode=self._run_settings.tt_mode,
                                                          calibration_dir=self._calibration_dir)
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

    def pearl_focus_tof_rebinning(self, workspace):
        out_name = workspace.name() + "_rebinned"
        out_ws = mantid.Rebin(InputWorkspace=workspace, Params=self._focus_tof_binning,
                              OutputWorkspace=out_name)
        return out_ws

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        if not output_mode:
            output_mode = self._run_settings.tt_mode
        output_spectra = \
            pearl_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                        run_details=run_details, focus_mode=output_mode,
                                                        perform_attenuation=self._run_settings.perform_attenuation)
        group_name = "PEARL" + run_details.run_number + "-Results-D-Grp"
        grouped_d_spacing = mantid.GroupWorkspaces(InputWorkspaces=output_spectra, OutputWorkspace=group_name)
        return grouped_d_spacing

    def crop_to_sane_tof(self, ws_to_crop):
        out_ws = common.crop_in_tof(ws_to_rebin=ws_to_crop, x_min=1500, x_max=19900)
        return out_ws

    def generate_vanadium_absorb_corrections(self, run_details, ws_to_match):
        return pearl_algs.generate_vanadium_absorb_corrections(van_ws=ws_to_match)

    def pearl_rebin_to_workspace(self, ws_to_rebin, ws_to_match):
        rebinned_ws = mantid.RebinToWorkspace(WorkspaceToRebin=ws_to_rebin, WorkspaceToMatch=ws_to_match)
        common.remove_intermediate_workspace(ws_to_rebin)
        ws_to_rebin = rebinned_ws
        return ws_to_rebin

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

    # Implementation of instrument specific steps


def _get_settings_common_kwargs(config_file_path, kwargs):
    expected_keys = ["long_mode"]
    yaml_parser.set_kwargs_from_config_file(config_path=config_file_path, kwargs=kwargs, keys_to_find=expected_keys)
    run_settings = PearlRunSettings.PearlRunSettings()
    run_settings.long_mode = kwargs["long_mode"]
    return run_settings


def _get_settings_focus_kwargs(config_file_path, kwargs):
    run_settings = _get_settings_common_kwargs(config_file_path=config_file_path, kwargs=kwargs)
    expected_keys = ["divide_vanadium", "tt_mode", "output_mode", "attenuate"]
    yaml_parser.set_kwargs_from_config_file(config_path=config_file_path, kwargs=kwargs, keys_to_find=expected_keys)
    run_settings.tt_mode = kwargs["tt_mode"]
    run_settings.focus_mode = kwargs["output_mode"]
    run_settings.perform_attenuation = kwargs["attenuate"]
    run_settings.divide_by_vanadium = kwargs["divide_vanadium"]
    return run_settings


def _get_settings_van_calib_kwargs(config_file_path, kwargs):
    run_settings = _get_settings_common_kwargs(config_file_path=config_file_path, kwargs=kwargs)
    expected_keys = ["absorption_corrections"]
    yaml_parser.set_kwargs_from_config_file(config_path=config_file_path, kwargs=kwargs, keys_to_find=expected_keys)
    run_settings.absorption_corrections = kwargs["absorption_corrections"]
    run_settings.tt_mode = "tt88"  # Use full range in vanadium mode
    return run_settings


def _generate_file_name(run_number):
    digit = len(str(run_number))

    number_of_digits = 8
    filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)
    return filename
