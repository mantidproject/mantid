from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_algs, pearl_calibration_algs, pearl_output, pearl_cycle_factory, \
    pearl_spline


class Pearl(AbstractInst):

    # Instrument default settings
    _default_input_ext = '.raw'
    _default_group_names = "bank1,bank2,bank3,bank4"

    _focus_tof_binning = "1500,-0.0006,19900"

    _create_van_first_tof_binning = "100,-0.0006,19990"
    _create_van_second_tof_binning = "150,-0.0006,19900"

    def __init__(self, user_name, tt_mode="TT88", calibration_dir=None, output_dir=None, **kwargs):

        super(Pearl, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                    output_dir=output_dir, kwargs=kwargs)

        self._tt_mode = tt_mode
        self._focus_mode = None
        self._spline_coeff = None

        # File names
        pearl_mc_absorption_file_name = "PRL112_DC25_10MM_FF.OUT"  # TODO how often does this change
        self._attenuation_full_path = os.path.join(calibration_dir, pearl_mc_absorption_file_name)
        self._ads_workaround = 0

    # --- Abstract Implementation ---- #

    def focus(self, run_number, focus_mode, do_attenuation=True, do_van_normalisation=True):
        self._focus_mode = focus_mode
        # TODO come back and allow PEARL to select their input batching method
        return self._focus(run_number=run_number, input_batching=InputBatchingEnum.Summed,
                           do_attenuation=do_attenuation, do_van_normalisation=do_van_normalisation)

    def create_calibration_vanadium(self, vanadium_runs, empty_runs, output_file_name=None, num_of_splines=60,
                                    do_absorb_corrections=True, gen_absorb_correction=False):
        self._spline_coeff = num_of_splines

        self._create_calibration_vanadium(vanadium_runs=vanadium_runs, empty_runs=empty_runs,
                                          output_file_name=output_file_name,
                                          do_absorb_corrections=do_absorb_corrections,
                                          gen_absorb_correction=gen_absorb_correction)

    def create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
        pearl_calibration_algs.create_calibration(self, calibration_runs=calibration_runs,
                                                  offset_file_name=offset_file_name,
                                                  grouping_file_name=grouping_file_name)

    def create_calibration_si(self, calibration_runs, cal_file_name, grouping_file_name):
        pearl_calibration_algs.do_silicon_calibration(self, calibration_runs, cal_file_name, grouping_file_name)

    # Params #
    def get_default_group_names(self):
        return self._default_group_names

    def _get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def get_run_details(self, run_number):
        # TODO once we migrate this to another format (i.e. not the if/elif/else) implement cached val
        cycle_dict = self._get_cycle_factory_dict(run_number=run_number)

        run_details = pearl_algs.get_run_details(tt_mode=self._tt_mode, run_number_string=run_number,
                                                 label=cycle_dict["cycle"], calibration_dir=self._calibration_dir)
        run_details.instrument_version = cycle_dict["instrument_version"]
        return run_details

    @staticmethod
    def _get_cycle_factory_dict(run_number):
        # TODO remove this when we move to combining CAL/RUN factories
        run_input = ""
        if isinstance(run_number, int) or run_number.isdigit():
            run_input = int(run_number)
        else:
            # Only take first valid number as it is probably of the form 12345_12350
            for character in run_number:
                if character.isdigit():
                    run_input += character
                else:
                    break

        cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_input)

        cycle_information = {'cycle': cycle,
                             'instrument_version': instrument_version}
        return cycle_information

    def get_num_of_banks(self, instrument_version=''):
        num_of_banks, save_range = pearl_algs.get_instrument_ranges(instrument_version=instrument_version)
        return num_of_banks

    @staticmethod
    def get_save_range(instrument_version):
        num_of_banks, save_range = pearl_algs.get_instrument_ranges(instrument_version=instrument_version)
        return save_range

    @staticmethod
    def generate_inst_file_name(run_number):
        return generate_file_name(run_number=run_number)

    # Hook overrides

    def attenuate_workspace(self, input_workspace):
        try:
            old_full_path = self._old_atten_file
        except AttributeError:
            old_full_path = None

        if not old_full_path:
            # If the old API hasn't set the path use the standard default
            attenuation_path = self._attenuation_full_path
        else:
            # Otherwise respect the one set through the API
            attenuation_path = self._old_atten_file
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
        return get_monitor_spectra(run_number=run_number, focus_mode=self._focus_mode)

    def spline_vanadium_ws(self, focused_vanadium_ws, instrument_version=''):
        # TODO move spline number into the class
        return pearl_spline.spline_vanadium_for_focusing(focused_vanadium_ws=focused_vanadium_ws,
                                                         spline_number=self._spline_coeff,
                                                         instrument_version=instrument_version)

    def pearl_focus_tof_rebinning(self, workspace):
        out_name = workspace.name() + "_rebinned"
        out_ws = mantid.Rebin(InputWorkspace=workspace, Params=self._focus_tof_binning,
                              OutputWorkspace=out_name)
        return out_ws

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def output_focused_ws(self, processed_spectra, run_details, attenuate=False):
        return pearl_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                           run_details=run_details, focus_mode=self._focus_mode,
                                                           perform_attenuation=attenuate)

    def pearl_van_calibration_tof_rebinning(self, vanadium_ws, tof_rebin_pass, return_units):
        if tof_rebin_pass == 1:
            tof_rebin_param = self._create_van_first_tof_binning
        elif tof_rebin_pass == 2:
            tof_rebin_param = self._create_van_second_tof_binning
        else:
            raise ValueError("Got a value that didn't match the expected number of passes")

        out_ws = pearl_algs.apply_tof_rebinning(ws_to_rebin=vanadium_ws, tof_params=tof_rebin_param,
                                                return_units=return_units)

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


def get_monitor_spectra(run_number, focus_mode):
    if run_number < 71009:
        if focus_mode == "trans":
            monitor_spectra = 1081
        elif focus_mode == "all":
            monitor_spectra = 2721
        elif focus_mode == "novan":
            monitor_spectra = 2721
        else:
            raise ValueError("Mode not set or supported")
    else:
        monitor_spectra = 1
    return monitor_spectra


def generate_file_name(run_number):
    digit = len(str(run_number))

    if run_number < 71009:
        number_of_digits = 5
        filename = "PRL"
    else:
        number_of_digits = 8
        filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)
    return filename
