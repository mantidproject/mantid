from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os

from isis_powder.routines import absorb_corrections, common, yaml_parser
from isis_powder.routines.RunDetails import RunDetails
from isis_powder.polaris_routines import polaris_advanced_config


def calculate_absorb_corrections(ws_to_correct, multiple_scattering):
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(0, 55)))

    absorb_dict = polaris_advanced_config.absorption_correction_params
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, config_dict=absorb_dict)
    return ws_to_correct


def get_run_details(run_number_string, inst_settings):
    run_number = common.get_first_run_number(run_number_string=run_number_string)
    cal_mapping = yaml_parser.get_run_dictionary(run_number_string=run_number, file_path=inst_settings.cal_mapping_file)

    label = common.cal_map_dictionary_key_helper(cal_mapping, "label")
    offset_file_name = common.cal_map_dictionary_key_helper(cal_mapping, "offset_file_name")

    if inst_settings.chopper_on:
        chopper_config = common.cal_map_dictionary_key_helper(cal_mapping, "chopper_on")
    else:
        chopper_config = common.cal_map_dictionary_key_helper(cal_mapping, "chopper_off")

    err_message = "This must be under the relevant chopper_on / chopper_off section."
    empty_runs = common.cal_map_dictionary_key_helper(chopper_config, "empty_run_numbers", err_message)
    vanadium_runs = common.cal_map_dictionary_key_helper(chopper_config, "vanadium_run_numbers", err_message)

    grouping_full_path = os.path.normpath(os.path.expanduser(inst_settings.calibration_dir))
    grouping_full_path = os.path.join(grouping_full_path, inst_settings.grouping_file_name)

    in_calib_dir = os.path.join(inst_settings.calibration_dir, label)
    offsets_file_full_path = os.path.join(in_calib_dir, offset_file_name)

    # Generate the name of the splined file we will either be loading or saving
    chopper_status = "ChopperOn" if inst_settings.chopper_on else "ChopperOff"
    splined_vanadium_name = common.generate_splined_name(vanadium_runs, chopper_status, offset_file_name)

    splined_vanadium = os.path.join(in_calib_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=run_number)
    run_details.user_input_run_number = run_number_string
    run_details.empty_runs = empty_runs
    run_details.vanadium_run_numbers = vanadium_runs
    run_details.label = label

    run_details.offset_file_path = offsets_file_full_path
    run_details.grouping_file_path = grouping_full_path
    run_details.splined_vanadium_file_path = splined_vanadium

    return run_details


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
