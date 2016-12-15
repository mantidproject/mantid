from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import InputBatchingEnum
import os
import warnings


def focus(run_number, instrument, input_batching, perform_vanadium_norm=True):
    if input_batching.lower() == InputBatchingEnum.Individual.lower():
        return _individual_run_focusing(input_batching, instrument, perform_vanadium_norm,
                                        run_number)
    else:
        return _batched_run_focusing(input_batching, instrument, perform_vanadium_norm, run_number)


def _focus_one_ws(ws, run_number, instrument, perform_vanadium_norm):
    run_details = instrument.get_run_details(run_number_string=run_number)

    # Check the necessary splined vanadium file has been created
    if not os.path.isfile(run_details.splined_vanadium_file_path):
        if instrument.can_auto_gen_vanadium_cal():
            warnings.warn("\nAttempting to automatically generate vanadium calibration at this path: "
                          + str(run_details.splined_vanadium_file_path) + " for these settings.\n")
            instrument.generate_auto_vanadium_calibration(run_details=run_details)
        else:
            ValueError("Processed vanadium runs not found at this path: "
                       + str(run_details.splined_vanadium_file_path) +
                       " \nHave you created a vanadium calibration with these settings yet?\n")

    # Compensate for empty sample if specified
    input_workspace = common.subtract_sample_empty(ws_to_correct=ws, instrument=instrument,
                                                   empty_sample_ws_string=run_details.empty_runs)
    input_workspace = instrument.crop_short_long_mode(ws_to_crop=input_workspace)

    # Align / Focus
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                            CalibrationFile=run_details.calibration_file_path)

    input_workspace = instrument.apply_solid_angle_efficiency_corr(ws_to_correct=input_workspace,
                                                                   run_details=run_details)

    focused_ws = mantid.DiffractionFocussing(InputWorkspace=input_workspace,
                                             GroupingFileName=run_details.grouping_file_path)

    calibrated_spectra = _apply_vanadium_corrections(instrument=instrument, run_number=run_number,
                                                     input_workspace=focused_ws,
                                                     perform_vanadium_norm=perform_vanadium_norm)

    # Output
    processed_nexus_files = instrument.output_focused_ws(calibrated_spectra, run_details=run_details)

    # Tidy
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(focused_ws)
    common.remove_intermediate_workspace(calibrated_spectra)

    return processed_nexus_files


def _batched_run_focusing(input_batching, instrument, perform_vanadium_norm, run_number):
    read_ws_list = common.load_current_normalised_ws_list(run_number_string=run_number,
                                                          input_batching=input_batching, instrument=instrument)
    output = None
    for ws in read_ws_list:
        output = _focus_one_ws(ws=ws, run_number=run_number, instrument=instrument,
                               perform_vanadium_norm=perform_vanadium_norm)
    return output


def _apply_vanadium_corrections(instrument, run_number, input_workspace, perform_vanadium_norm):
    run_details = instrument.get_run_details(run_number_string=run_number)
    converted_ws = mantid.ConvertUnits(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, Target="TOF")
    split_data_spectra = common.extract_and_crop_spectra(converted_ws, instrument=instrument)

    if perform_vanadium_norm:
        processed_spectra = _divide_by_vanadium_splines(spectra_list=split_data_spectra,
                                                        spline_file_path=run_details.splined_vanadium_file_path)
    else:
        processed_spectra = split_data_spectra

    return processed_spectra


def _divide_by_vanadium_splines(spectra_list, spline_file_path):
    vanadium_ws_list = mantid.LoadNexus(Filename=spline_file_path)
    output_list = []
    for data_ws, van_ws in zip(spectra_list, vanadium_ws_list[1:]):
        vanadium_ws = mantid.RebinToWorkspace(WorkspaceToRebin=van_ws, WorkspaceToMatch=data_ws)
        output_ws = mantid.Divide(LHSWorkspace=data_ws, RHSWorkspace=vanadium_ws, OutputWorkspace=data_ws)
        output_list.append(output_ws)
    return output_list


def _individual_run_focusing(input_batching, instrument, perform_vanadium_norm, run_number):
    # Load and process one by one
    run_numbers = common.generate_run_numbers(run_number_string=run_number)
    output = None
    for run in run_numbers:
        ws = common.load_current_normalised_ws_list(run_number_string=run, instrument=instrument,
                                                    input_batching=input_batching)
        output = _focus_one_ws(ws=ws[0], run_number=run, instrument=instrument,
                               perform_vanadium_norm=perform_vanadium_norm)
    return output
