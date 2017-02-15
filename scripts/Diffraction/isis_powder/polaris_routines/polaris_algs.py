from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os
from isis_powder.routines import common, yaml_parser
from isis_powder.routines.RunDetails import RunDetails
from isis_powder.polaris_routines import polaris_advanced_config


def calculate_absorb_corrections(ws_to_correct, multiple_scattering):
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(0, 55)))

    absorb_dict = polaris_advanced_config.absorption_correction_params
    geometry_json = {'Shape': 'Cylinder', 'Height': absorb_dict["cylinder_sample_height"],
                     'Radius': absorb_dict["cylinder_sample_radius"], 'Center': absorb_dict["cylinder_position"]}
    material_json = {'ChemicalFormula': absorb_dict["chemical_formula"]}

    mantid.SetSample(InputWorkspace=ws_to_correct, Geometry=geometry_json, Material=material_json)

    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="TOF")
    ws_to_correct = mantid.MayersSampleCorrection(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                                  MultipleScattering=multiple_scattering)
    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="dSpacing")

    return ws_to_correct


def get_run_details(run_number_string, inst_settings):
    run_number = common.generate_run_numbers(run_number_string=run_number_string)
    if isinstance(run_number, list):
        # Only take first run number
        run_number = run_number[0]
    yaml_dict = yaml_parser.get_run_dictionary(run_number_string=run_number, file_path=inst_settings.cal_mapping_file)

    if inst_settings.chopper_on:
        chopper_config = yaml_dict["chopper_on"]
    else:
        chopper_config = yaml_dict["chopper_off"]

    label = yaml_dict["label"]
    offset_file_name = yaml_dict["offset_file_name"]
    empty_runs = chopper_config["empty_run_numbers"]
    vanadium_runs = chopper_config["vanadium_run_numbers"]

    grouping_full_path = os.path.normpath(os.path.expanduser(inst_settings.calibration_dir))
    grouping_full_path = os.path.join(grouping_full_path, inst_settings.grouping_file_name)

    in_calib_dir = os.path.join(inst_settings.calibration_dir, label)
    calibration_full_path = os.path.join(in_calib_dir, offset_file_name)
    # Generate the name of the splined file we will either be loading or saving
    splined_vanadium_name = _generate_splined_van_filename(chopper_on=inst_settings.chopper_on,
                                                           vanadium_run_string=vanadium_runs,
                                                           offset_file_name=offset_file_name)
    splined_vanadium = os.path.join(in_calib_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=run_number)
    run_details.user_input_run_number = run_number_string
    run_details.empty_runs = empty_runs
    run_details.vanadium_run_numbers = vanadium_runs
    run_details.label = label

    run_details.calibration_file_path = calibration_full_path
    run_details.grouping_file_path = grouping_full_path
    run_details.splined_vanadium_file_path = splined_vanadium

    return run_details


def split_into_tof_d_spacing_groups(run_details, processed_spectra):
    d_spacing_output = []
    tof_output = []
    run_number = str(run_details.user_input_run_number)
    for name_index, ws in enumerate(processed_spectra):
        d_spacing_out_name = run_number + "-ResultD-" + str(name_index + 1)
        tof_out_name = run_number + "-ResultTOF-" + str(name_index + 1)

        d_spacing_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=d_spacing_out_name,
                                                    Target="dSpacing"))
        tof_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=tof_out_name, Target="TOF"))

    # Group the outputs
    d_spacing_group_name = run_number + "-Results-D-Grp"
    d_spacing_group = mantid.GroupWorkspaces(InputWorkspaces=d_spacing_output, OutputWorkspace=d_spacing_group_name)
    tof_group_name = run_number + "-Results-TOF-Grp"
    tof_group = mantid.GroupWorkspaces(InputWorkspaces=tof_output, OutputWorkspace=tof_group_name)

    return d_spacing_group, tof_group


def process_vanadium_for_focusing(bank_spectra, mask_path, spline_number):
    bragg_masking_list = _read_masking_file(mask_path)
    masked_workspace_list = _apply_bragg_peaks_masking(bank_spectra, mask_list=bragg_masking_list)
    output = common.spline_vanadium_for_focusing(focused_vanadium_spectra=masked_workspace_list,
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


def _generate_splined_van_filename(chopper_on, vanadium_run_string, offset_file_name):
    output_string = "SplinedVan_" + str(vanadium_run_string) + "_chopper"
    output_string += "On" if chopper_on else "Off"
    output_string += '_' + offset_file_name
    output_string += ".nxs"
    return output_string


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
