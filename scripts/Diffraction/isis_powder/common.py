from __future__ import (absolute_import, division, print_function)

from six.moves import xrange

import mantid.simpleapi as mantid

# --- Public API --- #


def create_calibration_by_names(calibration_runs, startup_objects, grouping_file_name, group_names):
    _create_blank_cal_file(calibration_runs=calibration_runs, group_names=group_names,
                           out_grouping_file_name=grouping_file_name, instrument=startup_objects)


def set_debug(debug_on=False):
    global g_debug
    g_debug = debug_on


def remove_intermediate_workspace(workspace_name):
    _remove_ws(ws_to_remove=workspace_name)

# --- Private Implementation --- #

# Please note these functions can change in any way at any time.
# For this reason please do not call them directly and instead use the Public API provided.

# If this doesn't quite provide what you need please let a developer know so we can create
# another API or update an existing one


_read_ws_count = 0
global g_ads_workaround
g_ads_workaround = {"read_ws": _read_ws_count}


def _create_blank_cal_file(calibration_runs, out_grouping_file_name, instrument, group_names):
    input_ws = _load_current_normalised_ws(calibration_runs, instrument)
    calibration_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.CreateCalFileByNames(InstrumentWorkspace=calibration_d_spacing_ws,
                                GroupingFileName=out_grouping_file_name, GroupNames=group_names)
    remove_intermediate_workspace(calibration_d_spacing_ws)
    remove_intermediate_workspace(input_ws)


def load_monitor(run_numbers, instrument):
    number_list = generate_run_numbers(run_numbers)
    monitor_spectra = instrument._get_monitor_spectra(number_list[0])

    if len(number_list) == 1:
        file_name = instrument._generate_inst_file_name(run_number=number_list[0])
        load_monitor_ws = mantid.Load(Filename=file_name, LoadLogFiles="0",
                                      SpectrumMin=monitor_spectra, SpectrumMax=monitor_spectra)
    else:
        load_monitor_ws = _load_sum_file_range(run_numbers=number_list, instrument=instrument)
        load_monitor_ws = mantid.ExtractSingleSpectrum(InputWorkspace=load_monitor_ws, WorkspaceIndex=monitor_spectra)

    return load_monitor_ws


def _load_raw_files(run_number, instrument):
    run_number_list = generate_run_numbers(run_number_string=run_number)
    instrument.PEARL_setup_input_directories(run_number=run_number_list[0])
    if len(run_number_list) == 1:
        run_number = instrument._generate_inst_file_name(run_number=run_number_list[0])
        load_raw_ws = mantid.Load(Filename=run_number, LoadLogFiles="0")
    else:
        load_raw_ws = _load_sum_file_range(run_number_list, instrument)
    return load_raw_ws


def _load_sum_file_range(run_numbers, instrument):
    read_ws_list = []
    for run_number in run_numbers:
        file_name = instrument._generate_inst_file_name(run_number=run_number)
        read_ws = mantid.Load(Filename=file_name, LoadLogFiles="0")
        output_name = "read_ws_output-" + str(g_ads_workaround["read_ws"])
        g_ads_workaround["read_ws"] += 1
        read_ws_list.append(mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=output_name))

    # Sum all workspaces
    out_ws_name = "summed_range_load-" + str(g_ads_workaround["read_ws"])
    g_ads_workaround["read_ws"] += 1

    summed_ws = mantid.RenameWorkspace(InputWorkspace=read_ws_list[0], OutputWorkspace=out_ws_name)
    for ws in read_ws_list[1:]:  # Skip first member
        summed_ws = mantid.Plus(LHSWorkspace=summed_ws, RHSWorkspace=ws, OutputWorkspace=out_ws_name)
        remove_intermediate_workspace(ws)

    return summed_ws


def generate_run_numbers(run_number_string):
    # Check its not a single run
    if isinstance(run_number_string, int) or run_number_string.isdigit():
        return [int(run_number_string)]  # Cast into a list and return

    # If its a string we must parse it
    run_boundaries = run_number_string.replace('_', '-')  # Accept either _ or - delimiters
    run_boundaries = run_boundaries.split("-")
    run_range = xrange(int(run_boundaries[0]), int(run_boundaries[-1]) + 1)  # TODO test for skip conditions
    return run_range


def _load_current_normalised_ws(run_number, instrument):
    read_in_ws = _load_raw_files(run_number=run_number, instrument=instrument)

    run_information = instrument._get_run_details(run_number_input=run_number)

    read_ws = instrument._normalise_ws(ws_to_correct=read_in_ws, run_details=run_information)

    output_name = "read_ws_output-" + str(g_ads_workaround["read_ws"])
    g_ads_workaround["read_ws"] += 1
    read_ws = mantid.RenameWorkspace(InputWorkspace=read_ws, OutputWorkspace=output_name)

    remove_intermediate_workspace(read_in_ws)
    return read_ws


def _remove_ws(ws_to_remove):
    """
    Removes any intermediate workspaces if debug is set to false
        @param ws_to_remove: The workspace to remove from the ADS
    """
    try:
        if not g_debug:
            _remove_ws_wrapper(ws=ws_to_remove)
    except NameError:  # If g_debug has not been set
        _remove_ws_wrapper(ws=ws_to_remove)


def _remove_ws_wrapper(ws):
    mantid.DeleteWorkspace(ws)
    del ws  # Mark it as deleted so that Python can throw before Mantid preserving more information
