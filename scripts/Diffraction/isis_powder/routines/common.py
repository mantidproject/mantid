from __future__ import (absolute_import, division, print_function)

import mantid.kernel as kernel
import mantid.simpleapi as mantid
from isis_powder.routines.common_enums import InputBatchingEnum


def create_calibration_by_names(calibration_runs, startup_objects, grouping_file_name, group_names):
    _create_blank_cal_file(calibration_runs=calibration_runs, group_names=group_names,
                           out_grouping_file_name=grouping_file_name, instrument=startup_objects)


def crop_banks_in_tof(bank_list, crop_values_list):
    if not isinstance(crop_values_list, list):
        raise ValueError("The cropping values were not in a list type")
    if len(bank_list) != len(crop_values_list):
        raise RuntimeError("The number of TOF cropping values does not match the number of banks for this instrument")

    output_list = []
    for spectra, cropping_values in zip(bank_list, crop_values_list):
        output_list.append(crop_in_tof(ws_to_crop=spectra, x_min=cropping_values[0], x_max=cropping_values[-1]))

    return output_list


def crop_in_tof(ws_to_crop, x_min=None, x_max=None):
    if isinstance(ws_to_crop, list):
        cropped_ws = []
        for ws in ws_to_crop:
            cropped_ws.append(_crop_single_ws_in_tof(ws, x_max=x_max, x_min=x_min))
    else:
        cropped_ws = _crop_single_ws_in_tof(ws_to_crop, x_max=x_max, x_min=x_min)

    return cropped_ws


def dictionary_key_helper(dictionary, key, throws=True, exception_msg=None):
    if key in dictionary:
        return dictionary[key]
    elif not throws:
        return None
    elif exception_msg:
        # Print user specified message
        raise KeyError(exception_msg)
    else:
        # Raise default python key error:
        this_throws = dictionary[key]
        return this_throws  # Never gets this far just makes linters happy


def extract_ws_spectra(ws_to_split):
    num_spectra = ws_to_split.getNumberHistograms()
    spectra_bank_list = []
    for i in range(0, num_spectra):
        output_name = "bank-" + str(i + 1)
        # Have to use crop workspace as extract single spectrum struggles with the variable bin widths
        spectra_bank_list.append(mantid.CropWorkspace(InputWorkspace=ws_to_split, OutputWorkspace=output_name,
                                                      StartWorkspaceIndex=i, EndWorkspaceIndex=i))
    return spectra_bank_list


def extract_and_crop_spectra(focused_ws, instrument):
    ws_spectra = extract_ws_spectra(ws_to_split=focused_ws)
    ws_spectra = instrument._crop_banks_to_user_tof(ws_spectra)
    return ws_spectra


def generate_run_numbers(run_number_string):
    # Check its not a single run
    if isinstance(run_number_string, int) or run_number_string.isdigit():
        return [int(run_number_string)]  # Cast into a list and return

    # If its a string we must parse it
    run_number_string = run_number_string.strip()
    run_boundaries = run_number_string.replace('_', '-')  # Accept either _ or - delimiters
    run_list = _run_number_generator(processed_string=run_boundaries)
    return run_list


def get_monitor_ws(ws_to_process, run_number_string, instrument):
    number_list = generate_run_numbers(run_number_string)
    monitor_spectra = instrument._get_monitor_spectra_index(number_list[0])
    load_monitor_ws = mantid.ExtractSingleSpectrum(InputWorkspace=ws_to_process, WorkspaceIndex=monitor_spectra)
    return load_monitor_ws


def load_current_normalised_ws_list(run_number_string, instrument, input_batching):
    run_information = instrument._get_run_details(run_number_string=run_number_string)
    raw_ws_list = _load_raw_files(run_number_string=run_number_string, instrument=instrument)

    if input_batching.lower() == InputBatchingEnum.Summed.lower() and len(raw_ws_list) > 1:
        summed_ws = _sum_ws_range(ws_list=raw_ws_list)
        remove_intermediate_workspace(raw_ws_list)
        raw_ws_list = [summed_ws]

    normalised_ws_list = _normalise_workspaces(ws_list=raw_ws_list, run_information=run_information,
                                               instrument=instrument)

    return normalised_ws_list


def remove_intermediate_workspace(workspaces):
    if isinstance(workspaces, list):
        for ws in workspaces:
            mantid.DeleteWorkspace(ws)
    else:
        mantid.DeleteWorkspace(workspaces)


def spline_vanadium_for_focusing(focused_vanadium_spectra, num_splines):
    bank_index = 1
    tof_ws_list = []
    for ws in focused_vanadium_spectra:
        out_name = "spline_bank_" + str(bank_index)
        bank_index += 1
        tof_ws_list.append(mantid.ConvertUnits(InputWorkspace=ws, Target="TOF", OutputWorkspace=out_name))

    splined_ws_list = []
    for ws in tof_ws_list:
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=ws, OutputWorkspace=ws, NCoeff=num_splines))

    return splined_ws_list


def subtract_sample_empty(ws_to_correct, empty_sample_ws_string, instrument):
    if empty_sample_ws_string:
        empty_sample = load_current_normalised_ws_list(run_number_string=empty_sample_ws_string, instrument=instrument,
                                                       input_batching=InputBatchingEnum.Summed)
        mantid.Minus(LHSWorkspace=ws_to_correct, RHSWorkspace=empty_sample[0], OutputWorkspace=ws_to_correct)
        remove_intermediate_workspace(empty_sample)

    return ws_to_correct


def _crop_single_ws_in_tof(ws_to_rebin, x_max, x_min):
    previous_units = ws_to_rebin.getAxis(0).getUnit().unitID()
    if previous_units != "TOF":
        ws_to_rebin = mantid.ConvertUnits(InputWorkspace=ws_to_rebin, Target="TOF", OutputWorkspace=ws_to_rebin)
    cropped_ws = mantid.CropWorkspace(InputWorkspace=ws_to_rebin, OutputWorkspace=ws_to_rebin,
                                      XMin=x_min, XMax=x_max)
    if previous_units != "TOF":
        cropped_ws = mantid.ConvertUnits(InputWorkspace=cropped_ws, Target=previous_units, OutputWorkspace=cropped_ws)
    return cropped_ws


def _normalise_workspaces(ws_list, instrument, run_information):
    output_list = []
    for ws in ws_list:
        output_list.append(instrument._normalise_ws_current(ws_to_correct=ws, run_details=run_information))

    return output_list


def _check_load_range(list_of_runs_to_load):
    maximum_range_len = 1000  # If more than this number of runs is entered probably wrong
    if len(list_of_runs_to_load) > maximum_range_len:
        raise ValueError("More than " + str(maximum_range_len) + " runs were selected."
                         " Found " + str(len(list_of_runs_to_load)) + " Aborting.")


def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, group_names):
    input_ws_list = load_current_normalised_ws_list(calibration_runs, instrument,
                                                    input_batching=InputBatchingEnum.Summed)
    calibration_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws_list[0], Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_d_spacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_d_spacing_ws)
    remove_intermediate_workspace(input_ws_list)


def _load_raw_files(run_number_string, instrument):
    run_number_list = generate_run_numbers(run_number_string=run_number_string)
    load_raw_ws = _load_list_of_files(run_number_list, instrument)
    return load_raw_ws


def _load_list_of_files(run_numbers_list, instrument):
    read_ws_list = []
    _check_load_range(list_of_runs_to_load=run_numbers_list)

    for run_number in run_numbers_list:
        file_name = instrument._generate_input_file_name(run_number=run_number)
        read_ws = mantid.Load(Filename=file_name)
        read_ws_list.append(mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=file_name))

    return read_ws_list


def _sum_ws_range(ws_list):
    # Sum all workspaces
    out_ws_name = "summed_" + ws_list[0].name() + '_' + ws_list[-1].name()
    summed_ws = mantid.MergeRuns(InputWorkspaces=ws_list, OutputWorkspace=out_ws_name)
    return summed_ws


def _run_number_generator(processed_string):
    try:
        number_generator = kernel.IntArrayProperty('array_generator', processed_string)
        return number_generator.value.tolist()
    except RuntimeError:
        raise RuntimeError("Could not generate run numbers from this input: " + processed_string)
