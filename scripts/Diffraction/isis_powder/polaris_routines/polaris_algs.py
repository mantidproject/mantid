from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import absorb_corrections, common
from isis_powder.routines.run_details import create_run_details_object, \
                                             CustomFuncForRunDetails, RunDetailsWrappedCommonFuncs
from isis_powder.polaris_routines import polaris_advanced_config


def calculate_van_absorb_corrections(ws_to_correct, multiple_scattering):
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(1, 55)))

    absorb_dict = polaris_advanced_config.absorption_correction_params
    sample_details_obj = absorb_corrections.create_vanadium_sample_details_obj(config_dict=absorb_dict)
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, sample_details_obj=sample_details_obj)
    return ws_to_correct


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    cal_mapping_callable = CustomFuncForRunDetails().add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.get_cal_mapping_dict, run_number_string=run_number_string,
        inst_settings=inst_settings
    ).add_to_func_chain(user_function=polaris_get_chopper_config, inst_settings=inst_settings)

    # Get empty and vanadium
    err_message = "this must be under the relevant Rietveld or PDF mode."

    empty_run_callable = cal_mapping_callable.add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.cal_dictionary_key_helper, key="empty_run_numbers",
        append_to_error_message=err_message)

    vanadium_run_callable = cal_mapping_callable.add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.cal_dictionary_key_helper, key="vanadium_run_numbers",
        append_to_error_message=err_message)

    return create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                     is_vanadium_run=is_vanadium_run, empty_run_call=empty_run_callable,
                                     vanadium_run_call=vanadium_run_callable)


def polaris_get_chopper_config(forwarded_value, inst_settings):
    # Forwarded value should be a cal mapping
    cal_mapping = forwarded_value
    return common.cal_map_dictionary_key_helper(cal_mapping, inst_settings.mode)


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
