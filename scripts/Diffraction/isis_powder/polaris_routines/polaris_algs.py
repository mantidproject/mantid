from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import absorb_corrections, common
from isis_powder.routines.RunDetails import create_run_details_object, RunDetailsFuncWrapper, WrappedFunctionsRunDetails
from isis_powder.polaris_routines import polaris_advanced_config


def calculate_absorb_corrections(ws_to_correct, multiple_scattering):
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(0, 55)))

    absorb_dict = polaris_advanced_config.absorption_correction_params
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, config_dict=absorb_dict)
    return ws_to_correct


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    # Get the chopper mode as vanadium and empty run numbers depend on different modes
    chopper_config_callable = RunDetailsFuncWrapper().\
        add_to_func_chain(function=WrappedFunctionsRunDetails.get_cal_mapping_dict, run_number_string=run_number_string,
                          inst_settings=inst_settings).\
        add_to_func_chain(function=polaris_get_chopper_config, inst_settings=inst_settings)

    # Then use the results to set the empty and vanadium runs
    err_message = "this must be under the relevant chopper_on / chopper_off section."

    empty_runs_callable = chopper_config_callable.add_to_func_chain(
        WrappedFunctionsRunDetails.cal_dictionary_key_helper,
        key="empty_run_numbers", append_to_error_message=err_message)

    vanadium_runs_callable = chopper_config_callable.add_to_func_chain(
        WrappedFunctionsRunDetails.cal_dictionary_key_helper, key="vanadium_run_numbers",
        append_to_error_message=err_message)

    run_details = create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                            empty_run_call=empty_runs_callable, is_vanadium_run=is_vanadium_run,
                                            vanadium_run_call=vanadium_runs_callable)

    return run_details


def polaris_get_chopper_config(forwarded_value, inst_settings):
    # The previous result is a cal_mapping
    cal_mapping = forwarded_value

    if inst_settings.chopper_on:
        chopper_config = common.cal_map_dictionary_key_helper(cal_mapping, "chopper_on")
    else:
        chopper_config = common.cal_map_dictionary_key_helper(cal_mapping, "chopper_off")

    return chopper_config


def process_vanadium_for_focusing(bank_spectra, mask_path, spline_number):
    bragg_masking_list = _read_masking_file(mask_path)
    masked_workspace_list = _apply_bragg_peaks_masking(bank_spectra, mask_list=bragg_masking_list)
    output = common.spline_workspaces(focused_vanadium_spectra=masked_workspace_list,
                                      num_splines=spline_number)
    common.remove_intermediate_workspace(masked_workspace_list)
    return output


def _apply_bragg_peaks_masking(workspaces_to_mask, mask_list):
    output_workspaces = list(workspaces_to_mask)

    for ws_index, (bank_mask_list, workspace) in enumerate(zip(mask_list, output_workspaces)):
        output_name = "masked_vanadium-" + str(ws_index + 1)
        for mask_params in bank_mask_list:
            output_workspaces[ws_index] = mantid.MaskBins(InputWorkspace=output_workspaces[ws_index],
                                                          OutputWorkspace=output_name,
                                                          XMin=mask_params[0], XMax=mask_params[1])
    return output_workspaces


def _read_masking_file(masking_file_path):
    all_banks_masking_list = []
    bank_masking_list = []
    ignore_line_prefixes = (' ', '\n', '\t', '#')  # Matches whitespace or # symbol
    with open(masking_file_path) as mask_file:
        for line in mask_file:
            if line.startswith(ignore_line_prefixes):
                # Push back onto new bank
                if bank_masking_list:
                    all_banks_masking_list.append(bank_masking_list)
                bank_masking_list = []
            else:
                # Parse and store in current list
                line.rstrip()
                bank_masking_list.append(line.split())

    if bank_masking_list:
        all_banks_masking_list.append(bank_masking_list)
    return all_banks_masking_list
