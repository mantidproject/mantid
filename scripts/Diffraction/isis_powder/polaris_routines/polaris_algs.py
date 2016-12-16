from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os
from isis_powder.routines import common, yaml_parser
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.routines.RunDetails import RunDetails
from isis_powder.polaris_routines import polaris_advanced_config


def generate_absorb_corrections(ws_to_match):
    absorb_ws = mantid.CloneWorkspace(InputWorkspace=ws_to_match)
    absorb_dict = polaris_advanced_config.absorption_correction_params

    absorb_ws = mantid.CylinderAbsorption(InputWorkspace=absorb_ws,
                                          CylinderSampleHeight=absorb_dict["cylinder_sample_height"],
                                          CylinderSampleRadius=absorb_dict["cylinder_sample_radius"],
                                          AttenuationXSection=absorb_dict["attenuation_cross_section"],
                                          ScatteringXSection=absorb_dict["scattering_cross_section"],
                                          SampleNumberDensity=absorb_dict["sample_number_density"],
                                          NumberOfSlices=absorb_dict["number_of_slices"],
                                          NumberOfAnnuli=absorb_dict["number_of_annuli"],
                                          NumberOfWavelengthPoints=absorb_dict["number_of_wavelength_points"],
                                          ExpMethod=absorb_dict["exponential_method"])
    return absorb_ws


def generate_solid_angle_corrections(run_details, instrument):
    vanadium_ws = common.load_current_normalised_ws_list(run_number_string=run_details.vanadium_run_numbers,
                                                         instrument=instrument, input_batching=InputBatchingEnum.Summed)
    corrections = _calculate_solid_angle_efficiency_corrections(vanadium_ws[0])
    mantid.SaveNexusProcessed(InputWorkspace=corrections, Filename=run_details.solid_angle_corr)
    common.remove_intermediate_workspace(vanadium_ws)
    return corrections


def get_run_details(run_number, inst_settings):
    yaml_dict = yaml_parser.get_run_dictionary(run_number=run_number, file_path=inst_settings.cal_mapping_file)

    if inst_settings.chopper_on:
        chopper_config = yaml_dict["chopper_on"]
    else:
        chopper_config = yaml_dict["chopper_off"]

    label = yaml_dict["label"]
    empty_runs = chopper_config["empty_run_numbers"]
    vanadium_runs = chopper_config["vanadium_run_numbers"]

    solid_angle_file_name = _generate_solid_angle_file_name(chopper_on=inst_settings.chopper_on,
                                                            vanadium_run_string=vanadium_runs)
    splined_vanadium_name = _generate_splined_van_name(chopper_on=inst_settings.chopper_on,
                                                       sac_applied=inst_settings.solid_angle_on,
                                                       vanadium_run_string=vanadium_runs)

    in_calib_dir = os.path.join(inst_settings.calibration_dir, label)
    calibration_full_path = os.path.join(in_calib_dir, yaml_dict["offset_file_name"])
    grouping_full_path = os.path.join(in_calib_dir, yaml_dict["offset_file_name"])
    solid_angle_file_path = os.path.join(in_calib_dir, solid_angle_file_name)
    splined_vanadium = os.path.join(in_calib_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=run_number)
    run_details.empty_runs = empty_runs
    run_details.vanadium_run_numbers = vanadium_runs
    run_details.label = label

    run_details.calibration_file_path = calibration_full_path
    run_details.grouping_file_path = grouping_full_path
    run_details.splined_vanadium_file_path = splined_vanadium
    run_details.solid_angle_corr = solid_angle_file_path

    return run_details


def split_into_tof_d_spacing_groups(processed_spectra):
    name_index = 1
    d_spacing_output = []
    tof_output = []
    for ws in processed_spectra:
        d_spacing_out_name = "ResultD-" + str(name_index)
        tof_out_name = "ResultTOF-" + str(name_index)
        name_index += 1

        d_spacing_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=d_spacing_out_name,
                                                    Target="dSpacing"))
        tof_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=tof_out_name, Target="TOF"))

    # Group the outputs
    d_spacing_group_name = "Results-D-Grp"
    d_spacing_group = mantid.GroupWorkspaces(InputWorkspaces=d_spacing_output, OutputWorkspace=d_spacing_group_name)
    tof_group_name = "Results-TOF-Grp"
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
    index = 0
    output_workspaces = []

    for bank_mask_list in mask_list:
        if not bank_mask_list:
            continue

        output_name = "masked_vanadium-" + str(index + 1)
        out_workspace = None
        for mask_params in bank_mask_list:
            out_workspace = mantid.MaskBins(InputWorkspace=workspaces_to_mask[index], OutputWorkspace=output_name,
                                            XMin=mask_params[0], XMax=mask_params[1])
        if out_workspace:
            output_workspaces.append(out_workspace)

        index += 1

    return output_workspaces


def _calculate_solid_angle_efficiency_corrections(vanadium_ws):
    solid_angle_ws = mantid.SolidAngle(InputWorkspace=vanadium_ws)
    solid_angle_multiplicand = mantid.CreateSingleValuedWorkspace(DataValue=str(100))
    solid_angle_ws = mantid.Multiply(LHSWorkspace=solid_angle_ws, RHSWorkspace=solid_angle_multiplicand)
    common.remove_intermediate_workspace(solid_angle_multiplicand)

    efficiency_ws = mantid.Divide(LHSWorkspace=vanadium_ws, RHSWorkspace=solid_angle_ws)
    efficiency_ws = mantid.ConvertUnits(InputWorkspace=efficiency_ws, Target="Wavelength")
    efficiency_ws = mantid.Integration(InputWorkspace=efficiency_ws)

    corrections_ws = mantid.Multiply(LHSWorkspace=solid_angle_ws, RHSWorkspace=efficiency_ws)
    corrections_divisor_ws = mantid.CreateSingleValuedWorkspace(DataValue=str(100000))
    corrections_ws = mantid.Divide(LHSWorkspace=corrections_ws, RHSWorkspace=corrections_divisor_ws)

    common.remove_intermediate_workspace(corrections_divisor_ws)
    common.remove_intermediate_workspace(solid_angle_ws)
    common.remove_intermediate_workspace(efficiency_ws)

    return corrections_ws


def _generate_solid_angle_file_name(chopper_on, vanadium_run_string):
    if chopper_on:
        return "SAC_" + vanadium_run_string + "_chopperOn"
    else:
        return "SAC_" + vanadium_run_string + "_chopperOff"


def _generate_splined_van_name(chopper_on, sac_applied, vanadium_run_string):
    output_string = "SVan_" + str(vanadium_run_string) + "_chopper"
    if chopper_on:
        output_string += "On"
    else:
        output_string += "Off"

    if sac_applied:
        output_string += "_SAC"
    else:
        output_string += "_noSAC"
    return output_string


def _read_masking_file(masking_file_path):
    all_banks_masking_list = []
    bank_masking_list = []
    ignore_line_prefixes = (' ', '\n', '\t', '#')  # Matches whitespace or # symbol
    with open(masking_file_path) as mask_file:
        for line in mask_file:
            if line.startswith(ignore_line_prefixes):
                # Push back onto new bank
                all_banks_masking_list.append(bank_masking_list)
                bank_masking_list = []
            else:
                line.rstrip()
                bank_masking_list.append(line.split())

    if bank_masking_list:
        all_banks_masking_list.append(bank_masking_list)
    return all_banks_masking_list
