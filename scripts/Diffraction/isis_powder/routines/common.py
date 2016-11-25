from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

# A small workaround to ensure when reading workspaces in a loop
# the previous workspace does not got overridden
_read_ws_count = 0
global g_ads_workaround
g_ads_workaround = {"read_ws": _read_ws_count}

# --- Public API --- #


def create_calibration_by_names(calibration_runs, startup_objects, grouping_file_name, group_names):
    _create_blank_cal_file(calibration_runs=calibration_runs, group_names=group_names,
                           out_grouping_file_name=grouping_file_name, instrument=startup_objects)

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
    run_range_lists = __run_number_generator(processed_string=run_boundaries)
    out_range = []
    for range_list in run_range_lists:
        out_range.extend(range_list)

    return out_range


def get_monitor_ws(ws_to_process, run_number_string, instrument):
    number_list = generate_run_numbers(run_number_string)
    monitor_spectra = instrument.get_monitor_spectra_index(number_list[0])
    load_monitor_ws = mantid.ExtractSingleSpectrum(InputWorkspace=ws_to_process, WorkspaceIndex=monitor_spectra)
    return load_monitor_ws


def remove_intermediate_workspace(workspace_name):
    mantid.DeleteWorkspace(workspace_name)
    del workspace_name  # Mark it as deleted so that more information is preserved on throw


def subtract_sample_empty(ws_to_correct, empty_sample_ws_string, instrument):
    output = ws_to_correct
    if empty_sample_ws_string is not None:
        empty_sample = load_current_normalised_ws(run_number_string=empty_sample_ws_string, instrument=instrument)
        corrected_ws = mantid.Minus(LHSWorkspace=ws_to_correct, RHSWorkspace=empty_sample)
        remove_intermediate_workspace(empty_sample)
        output = corrected_ws
    return output


def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, group_names):
    input_ws = load_current_normalised_ws(calibration_runs, instrument)
    calibration_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_d_spacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_d_spacing_ws)
    remove_intermediate_workspace(input_ws)


def _load_raw_files(run_number_string, instrument):
    run_number_list = generate_run_numbers(run_number_string=run_number_string)
    instrument._old_api_pearl_setup_input_dirs(run_number=run_number_list[0])
    load_raw_ws = _load_sum_file_range(run_number_list, instrument)
    return load_raw_ws


def _load_sum_file_range(run_numbers_list, instrument):
    read_ws_list = []

    _check_load_range(list_of_runs_to_load=run_numbers_list)

    for run_number in run_numbers_list:
        file_name = instrument.generate_inst_file_name(run_number=run_number)
        read_ws = mantid.Load(Filename=file_name, LoadLogFiles="0")
        read_ws_list.append(mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=str(file_name)))

    # Sum all workspaces
    out_ws_name = "summed_range_load-" + str(g_ads_workaround["read_ws"])
    g_ads_workaround["read_ws"] += 1
    summed_ws = mantid.RenameWorkspace(InputWorkspace=read_ws_list[0], OutputWorkspace=out_ws_name)
    for ws in read_ws_list[1:]:  # Skip first member
        summed_ws = mantid.Plus(LHSWorkspace=summed_ws, RHSWorkspace=ws, OutputWorkspace=out_ws_name)
        remove_intermediate_workspace(ws)

    return summed_ws


def _check_load_range(list_of_runs_to_load):
    MAXIMUM_RANGE_LEN = 20  # If more than this number of runs is entered probably wrong
    if len(list_of_runs_to_load) > MAXIMUM_RANGE_LEN:
        raise ValueError("More than " + str(MAXIMUM_RANGE_LEN) + " runs were selected."
                         " Found " + str(len(list_of_runs_to_load)) + " Aborting.")


def __run_number_generator(processed_string):
    # Expands run numbers of the form 1-10, 12, 14-20, 23 to 1,2,3,..,8,9,10,12,14,15,16...,19,20,23
    for entry in processed_string.split(','):
        # Split between comma separated values
        numbers = entry.split('-')
        # Check if we are using a dash separator and return the range between those values
        if len(numbers) == 1:
            yield list(numbers)
        elif len(numbers) == 2:
            # Add 1 so it includes the final number '-' range
            yield range(int(numbers[0]), int(numbers[-1]) + 1)
        else:
            raise ValueError("The run number " + str(entry) + " is incorrect in calibration mapping")


def load_current_normalised_ws(run_number_string, instrument):
    read_in_ws = _load_raw_files(run_number_string=run_number_string, instrument=instrument)

    run_information = instrument.get_run_details(run_number=run_number_string)

    read_ws = instrument.normalise_ws(ws_to_correct=read_in_ws, run_details=run_information)

    output_name = "read_ws_output-" + str(g_ads_workaround["read_ws"])
    g_ads_workaround["read_ws"] += 1
    read_ws = mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=output_name)

    remove_intermediate_workspace(read_in_ws)
    return read_ws
