from __future__ import (absolute_import, division, print_function)

import mantid.kernel as kernel
import mantid.simpleapi as mantid
from isis_powder.routines.common_enums import InputBatchingEnum

# A small workaround to ensure when reading workspaces in a loop
# the previous workspace does not got overridden
_read_ws_count = 0
global g_ads_workaround
g_ads_workaround = {"read_ws": _read_ws_count}

# --- Public API --- #


def create_calibration_by_names(calibration_runs, startup_objects, grouping_file_name, group_names):
    _create_blank_cal_file(calibration_runs=calibration_runs, group_names=group_names,
                           out_grouping_file_name=grouping_file_name, instrument=startup_objects)


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


def extract_bank_spectra(ws_to_split, num_banks):
    spectra_bank_list = []
    for i in range(0, num_banks):
        output_name = "bank-" + str(i + 1)
        # Have to use crop workspace as extract single spectrum struggles with the variable bin widths
        spectra_bank_list.append(mantid.CropWorkspace(InputWorkspace=ws_to_split, OutputWorkspace=output_name,
                                                      StartWorkspaceIndex=i, EndWorkspaceIndex=i))
    return spectra_bank_list


def generate_run_numbers(run_number_string):

    # Check its not a single run
    if isinstance(run_number_string, int) or run_number_string.isdigit():
        return [int(run_number_string)]  # Cast into a list and return

    # If its a string we must parse it
    run_number_string = run_number_string.strip()
    run_boundaries = run_number_string.replace('_', '-')  # Accept either _ or - delimiters
    run_list = _run_number_generator(processed_string=run_boundaries)
    return run_list


def generate_unique_workspace_name(original_name):
    number_to_append = 1
    unique_name = original_name
    if mantid.mtd.doesExist(unique_name):
        while mantid.mtd.doesExist(original_name + '_' + str(number_to_append)):
            number_to_append += 1
        # Found a unique combination
        unique_name = original_name + '_' + str(number_to_append)

    return unique_name


def get_monitor_ws(ws_to_process, run_number_string, instrument):
    number_list = generate_run_numbers(run_number_string)
    monitor_spectra = instrument.get_monitor_spectra_index(number_list[0])
    load_monitor_ws = mantid.ExtractSingleSpectrum(InputWorkspace=ws_to_process, WorkspaceIndex=monitor_spectra)
    return load_monitor_ws


def load_current_normalised_ws_list(run_number_string, instrument, input_batching):
    run_information = instrument.get_run_details(run_number=run_number_string)
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


def subtract_sample_empty(ws_to_correct, empty_sample_ws_string, instrument):
    if empty_sample_ws_string:
        empty_sample = load_current_normalised_ws_list(run_number_string=empty_sample_ws_string, instrument=instrument,
                                                       input_batching=InputBatchingEnum.Summed)
        mantid.Minus(LHSWorkspace=ws_to_correct, RHSWorkspace=empty_sample[0], OutputWorkspace=ws_to_correct)
        remove_intermediate_workspace(empty_sample)

    return ws_to_correct


def _normalise_workspaces(ws_list, instrument, run_information):
    output_list = []
    for ws in ws_list:
        output_list.append(instrument.normalise_ws(ws_to_correct=ws, run_details=run_information))

    return output_list


def _check_load_range(list_of_runs_to_load):
    MAXIMUM_RANGE_LEN = 1000  # If more than this number of runs is entered probably wrong
    if len(list_of_runs_to_load) > MAXIMUM_RANGE_LEN:
        raise ValueError("More than " + str(MAXIMUM_RANGE_LEN) + " runs were selected."
                         " Found " + str(len(list_of_runs_to_load)) + " Aborting.")


def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, group_names):
    input_ws_list = load_current_normalised_ws_list(calibration_runs, instrument, input_batching=InputBatchingEnum.Summed)
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
        file_name = instrument.generate_inst_file_name(run_number=run_number)
        read_ws = mantid.Load(Filename=file_name)
        ws_name = generate_unique_workspace_name(original_name=file_name)
        read_ws_list.append(mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=ws_name))

    return read_ws_list


def _sum_ws_range(ws_list):
    # Sum all workspaces
    out_ws_name = "summed_" + ws_list[0].name() + '_' + ws_list[-1].name()

    summed_ws = mantid.CloneWorkspace(InputWorkspace=ws_list[0], OutputWorkspace=out_ws_name)

    for ws in ws_list[1:]:
        summed_ws = mantid.Plus(LHSWorkspace=summed_ws, RHSWorkspace=ws, OutputWorkspace=out_ws_name)

    # summed_ws = mantid.MergeRuns(InputWorkspaces=ws_list, OutputWorkspace=out_ws_name)
    return summed_ws


def _run_number_generator(processed_string):
    try:
        number_generator = kernel.IntArrayProperty('array_generator', processed_string)
        return number_generator.value.tolist()
    except RuntimeError:
        raise RuntimeError("Could not generate run numbers from this input: " + processed_string)
