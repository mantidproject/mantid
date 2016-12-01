from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import InputBatchingEnum
import os


def focus(run_number, instrument, input_batching, perform_attenuation=True, perform_vanadium_norm=True):
    if input_batching.lower() == InputBatchingEnum.Individual.lower():
        _individual_run_focusing(input_batching, instrument, perform_attenuation, perform_vanadium_norm, run_number)
    else:
        _batched_run_focusing(input_batching, instrument, perform_attenuation, perform_vanadium_norm, run_number)


def _batched_run_focusing(input_batching, instrument, perform_attenuation, perform_vanadium_norm, run_number):
    import pydevd
    pydevd.settrace('localhost', port=51205, stdoutToServer=True, stderrToServer=True)
    read_ws_list = common.load_current_normalised_ws_list(run_number_string=run_number,
                                                          input_batching=input_batching, instrument=instrument)
    for ws in read_ws_list:
        _focus_one_ws(ws=ws, run_number=run_number, instrument=instrument, perform_attenuation=perform_attenuation,
                      perform_vanadium_norm=perform_vanadium_norm)
        common.remove_intermediate_workspace(ws)


def _divide_sample_by_vanadium(instrument, run_number, input_workspace, perform_vanadium_norm):
    processed_spectra = []
    run_details = instrument.get_run_details(run_number=run_number)
    num_of_banks = instrument.get_num_of_banks(run_details.instrument_version)
    split_ws = common.extract_bank_spectra(ws_to_split=input_workspace, num_banks=num_of_banks)

    if perform_vanadium_norm:
        vanadium_ws_list = mantid.LoadNexus(Filename=run_details.splined_vanadium)
        for focus_spectra, van_spectra in zip(split_ws, vanadium_ws_list[1:]):
            processed_spectra.append(
                instrument.correct_sample_vanadium(focus_spectra=focus_spectra, vanadium_spectra=van_spectra))
        common.remove_intermediate_workspace(vanadium_ws_list[0])

    else:
        for focus_spectra in split_ws:
            processed_spectra.append(instrument.correct_sample_vanadium(focus_spectra=focus_spectra))

    for ws in split_ws:
        common.remove_intermediate_workspace(ws)

    return processed_spectra


def _focus_one_ws(ws, run_number, instrument, perform_attenuation=True, perform_vanadium_norm=True):
    # input_workspace = instrument.pearl_focus_tof_rebinning(read_ws)  # Rebins for PEARL
    run_details = instrument.get_run_details(run_number=run_number)

    # Check the necessary splined vanadium file has been created
    if not os.path.isfile(run_details.splined_vanadium):
        raise ValueError("Processed vanadium runs not found at this path: "
                         + str(run_details.splined_vanadium) +
                         " \n\nHave you created a vanadium calibration with these settings yet?")

    # Compensate for empty sample if specified
    input_workspace = common.subtract_sample_empty(ws_to_correct=ws, instrument=instrument,
                                                   empty_sample_ws_string=run_details.sample_empty)

    # Align / Focus
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                            CalibrationFile=run_details.calibration)

    input_workspace = instrument.apply_solid_angle_efficiency_corr(ws_to_correct=input_workspace,
                                                                   run_details=run_details)

    focused_ws = mantid.DiffractionFocussing(InputWorkspace=input_workspace,
                                             GroupingFileName=run_details.grouping)

    # Process
    rebinning_params = instrument.calculate_focus_binning_params(sample=focused_ws)

    calibrated_spectra = _divide_sample_by_vanadium(instrument=instrument, run_number=run_number,
                                                    input_workspace=focused_ws,
                                                    perform_vanadium_norm=perform_vanadium_norm)

    _apply_binning_to_spectra(spectra_list=calibrated_spectra, binning_list=rebinning_params)

    # Output
    processed_nexus_files = instrument.output_focused_ws(calibrated_spectra, run_details=run_details,
                                                         attenuate=perform_attenuation)

    # Tidy
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(focused_ws)
    common.remove_intermediate_workspace(calibrated_spectra)

    return processed_nexus_files


def _individual_run_focusing(input_batching, instrument, perform_attenuation, perform_vanadium_norm, run_number):
    # Load and process one by one
    run_numbers = common.generate_run_numbers(run_number_string=run_number)
    for run in run_numbers:
        ws = common.load_current_normalised_ws_list(run_number_string=run, instrument=instrument,
                                                    input_batching=input_batching)
        _focus_one_ws(ws=ws[0], run_number=run, instrument=instrument, perform_attenuation=perform_attenuation,
                      perform_vanadium_norm=perform_vanadium_norm)
        common.remove_intermediate_workspace(ws)


def _apply_binning_to_spectra(spectra_list, binning_list):
    if not binning_list:
        return

    for ws, bin_params in zip(spectra_list, binning_list):
        # Starting bin edge / bin width / last bin edge
        rebin_string = bin_params[0] + ',' + bin_params[1] + ',' + bin_params[2]
        mantid.Rebin(InputWorkspace=ws, OutputWorkspace=ws, Params=rebin_string)
