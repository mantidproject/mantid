from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING
import os


def focus(run_number_string, instrument, perform_vanadium_norm, absorb):
    input_batching = instrument._get_input_batching_mode()
    if input_batching == INPUT_BATCHING.Individual:
        return _individual_run_focusing(instrument=instrument, perform_vanadium_norm=perform_vanadium_norm,
                                        run_number=run_number_string, absorb=absorb)
    elif input_batching == INPUT_BATCHING.Summed:
        return _batched_run_focusing(instrument, perform_vanadium_norm, run_number_string, absorb=absorb)
    else:
        raise ValueError("Input batching not passed through. Please contact development team.")


def _focus_one_ws(ws, run_number, instrument, perform_vanadium_norm, absorb):
    run_details = instrument._get_run_details(run_number_string=run_number)
    if perform_vanadium_norm:
        _test_splined_vanadium_exists(instrument, run_details)

    # Subtract empty instrument runs
    input_workspace = common.subtract_summed_runs(ws_to_correct=ws, instrument=instrument,
                                                  empty_sample_ws_string=run_details.empty_runs)
    # Subtract a sample empty if specified
    if run_details.sample_empty:
        input_workspace = common.subtract_summed_runs(ws_to_correct=input_workspace, instrument=instrument,
                                                      empty_sample_ws_string=run_details.sample_empty,
                                                      scale_factor=instrument._inst_settings.sample_empty_scale)

    # Crop to largest acceptable TOF range
    input_workspace = instrument._crop_raw_to_expected_tof_range(ws_to_crop=input_workspace)

    # Correct for absorption / multiple scattering if required
    if absorb:
        input_workspace = instrument._apply_absorb_corrections(run_details=run_details, ws_to_correct=input_workspace)

    # Align
    aligned_ws = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                       CalibrationFile=run_details.offset_file_path)

    # Focus the spectra into banks
    focused_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws,
                                             GroupingFileName=run_details.grouping_file_path)

    calibrated_spectra = _apply_vanadium_corrections(instrument=instrument, run_number=run_number,
                                                     input_workspace=focused_ws,
                                                     perform_vanadium_norm=perform_vanadium_norm)

    output_spectra = instrument._crop_banks_to_user_tof(calibrated_spectra)

    bin_widths = instrument._get_instrument_bin_widths()
    if bin_widths:
        # Reduce the bin width if required on this instrument
        output_spectra = common.rebin_workspace_list(workspace_list=output_spectra,
                                                     bin_width_list=bin_widths)

    # Output
    d_spacing_group, tof_group = instrument._output_focused_ws(output_spectra, run_details=run_details)

    common.keep_single_ws_unit(d_spacing_group=d_spacing_group, tof_group=tof_group,
                               unit_to_keep=instrument._get_unit_to_keep())

    # Tidy workspaces from Mantid
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(aligned_ws)
    common.remove_intermediate_workspace(focused_ws)
    common.remove_intermediate_workspace(output_spectra)

    return d_spacing_group


def _apply_vanadium_corrections(instrument, run_number, input_workspace, perform_vanadium_norm):
    run_details = instrument._get_run_details(run_number_string=run_number)
    input_workspace = mantid.ConvertUnits(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, Target="TOF")
    split_data_spectra = common.extract_ws_spectra(input_workspace)

    if perform_vanadium_norm:
        processed_spectra = _divide_by_vanadium_splines(spectra_list=split_data_spectra,
                                                        spline_file_path=run_details.splined_vanadium_file_path)
    else:
        processed_spectra = split_data_spectra

    return processed_spectra


def _batched_run_focusing(instrument, perform_vanadium_norm, run_number_string, absorb):
    read_ws_list = common.load_current_normalised_ws_list(run_number_string=run_number_string,
                                                          instrument=instrument)
    output = None
    for ws in read_ws_list:
        output = _focus_one_ws(ws=ws, run_number=run_number_string, instrument=instrument,
                               perform_vanadium_norm=perform_vanadium_norm, absorb=absorb)
    return output


def _divide_by_vanadium_splines(spectra_list, spline_file_path):
    vanadium_ws_list = mantid.LoadNexus(Filename=spline_file_path)
    output_list = []
    for data_ws, van_ws in zip(spectra_list, vanadium_ws_list[1:]):
        vanadium_ws = mantid.RebinToWorkspace(WorkspaceToRebin=van_ws, WorkspaceToMatch=data_ws)
        output_ws = mantid.Divide(LHSWorkspace=data_ws, RHSWorkspace=vanadium_ws, OutputWorkspace=data_ws)
        output_list.append(output_ws)
        common.remove_intermediate_workspace(vanadium_ws)
    return output_list


def _individual_run_focusing(instrument, perform_vanadium_norm, run_number, absorb):
    # Load and process one by one
    run_numbers = common.generate_run_numbers(run_number_string=run_number)
    output = None
    for run in run_numbers:
        ws = common.load_current_normalised_ws_list(run_number_string=run, instrument=instrument)
        output = _focus_one_ws(ws=ws[0], run_number=run, instrument=instrument, absorb=absorb,
                               perform_vanadium_norm=perform_vanadium_norm)
    return output


def _test_splined_vanadium_exists(instrument, run_details):
    # Check the necessary splined vanadium file has been created
    if not os.path.isfile(run_details.splined_vanadium_file_path):
        raise ValueError("Processed vanadium runs not found at this path: "
                         + str(run_details.splined_vanadium_file_path) +
                         " \nHave you run the method to create a Vanadium spline with these settings yet?\n")
