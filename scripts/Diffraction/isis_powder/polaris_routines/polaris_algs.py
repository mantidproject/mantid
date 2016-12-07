from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import os
import isis_powder.routines.common as common
from isis_powder.routines import yaml_parser
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.routines.RunDetails import RunDetails


def calculate_focus_binning_params(sample_ws):
    # TODO remove this if they only want sane TOF values and not consistent binning
    focus_bin_widths = [-0.0050, -0.0010, -0.0010, -0.0010, -0.00050]
    focus_crop_start = 2  # These are used when calculating binning range
    focus_crop_end = 0.95

    calculated_binning_params = []
    num_of_banks = sample_ws.getNumberHistograms()
    for i in range(0, num_of_banks):
        sample_data = sample_ws.readX(i)
        starting_bin = sample_data[0] * (1 + focus_crop_start)
        ending_bin = sample_data[-1] * focus_crop_end
        bin_width = focus_bin_widths[i]

        bank_binning_params = [str(starting_bin), str(bin_width), str(ending_bin)]
        calculated_binning_params.append(bank_binning_params)

    return calculated_binning_params


def generate_absorb_corrections(ws_to_match):
    absorb_ws = mantid.CloneWorkspace(InputWorkspace=ws_to_match)

    # TODO move all of this into defaults
    cylinder_sample_height = str(4)
    cylinder_sample_radius = str(0.4)

    attenuation_cross_section = str(4.88350)
    scattering_cross_section = str(5.15775)
    sample_number_density = str(0.0718956)

    number_of_slices = str(10)
    number_of_annuli = str(10)
    number_of_wavelength_points = str(100)

    exp_method = "Normal"
    # TODO move all of the above into defaults

    absorb_ws = mantid.CylinderAbsorption(InputWorkspace=absorb_ws,
                                          CylinderSampleHeight=cylinder_sample_height,
                                          CylinderSampleRadius=cylinder_sample_radius,
                                          AttenuationXSection=attenuation_cross_section,
                                          ScatteringXSection=scattering_cross_section,
                                          SampleNumberDensity=sample_number_density,
                                          NumberOfSlices=number_of_slices,
                                          NumberOfAnnuli=number_of_annuli,
                                          NumberOfWavelengthPoints=number_of_wavelength_points,
                                          ExpMethod=exp_method)
    return absorb_ws


def generate_solid_angle_corrections(run_details, instrument):
    vanadium_ws = common.load_current_normalised_ws_list(run_number_string=run_details.vanadium_run_numbers,
                                                         instrument=instrument, input_batching=InputBatchingEnum.Summed)
    corrections = _calculate_solid_angle_efficiency_corrections(vanadium_ws[0])
    mantid.SaveNexusProcessed(InputWorkspace=corrections, Filename=run_details.solid_angle_corr)
    common.remove_intermediate_workspace(vanadium_ws)
    return corrections


def get_run_details(chopper_on, sac_on, run_number_string, calibration_dir, mapping_path):
    input_run_number_list = common.generate_run_numbers(run_number_string=run_number_string)
    yaml_dict = yaml_parser.get_run_dictionary(run_number=input_run_number_list[0], file_path=mapping_path)

    if chopper_on:
        chopper_config = yaml_dict["chopper_on"]
    else:
        chopper_config = yaml_dict["chopper_off"]

    label = yaml_dict["label"]
    empty_runs = chopper_config["empty_run_numbers"]
    vanadium_runs = chopper_config["vanadium_run_numbers"]

    solid_angle_file_name = _generate_solid_angle_file_name(chopper_on=chopper_on,
                                                            vanadium_run_string=vanadium_runs)
    splined_vanadium_name = _generate_splined_van_name(chopper_on=chopper_on, sac_applied=sac_on,
                                                       vanadium_run_string=vanadium_runs)

    in_calib_dir = os.path.join(calibration_dir, label)
    calibration_full_path = os.path.join(in_calib_dir, yaml_dict["offset_file_name"])
    grouping_full_path = os.path.join(in_calib_dir, yaml_dict["offset_file_name"])
    solid_angle_file_path = os.path.join(in_calib_dir, solid_angle_file_name)
    splined_vanadium = os.path.join(in_calib_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=run_number_string)
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


def process_vanadium_for_focusing(bank_spectra, mode, mask_path, spline_number=None):
    # TODO move spline number/mode out of params passed and instead get this to read it itself
    if mode == "spline":  # TODO support more modes
        bragg_masking_list = _read_masking_file(mask_path)
        output = _spline_vanadium_for_focusing(vanadium_spectra_list=bank_spectra,
                                               spline_coefficient=spline_number, mask_list=bragg_masking_list)
    else:
        raise NotImplementedError("Other vanadium processing methods not yet implemented")

    return output


def _apply_bragg_peaks_masking(workspaces_to_mask, mask_list):

    index = 0
    output_workspaces = []
    for ws in workspaces_to_mask:
        output_workspaces.append(ws)

    for bank_mask_list in mask_list:
        if not bank_mask_list:
            continue

        output_name = "masked_vanadium-" + str(index + 1)

        for mask_params in bank_mask_list:
            out_workspace = mantid.MaskBins(InputWorkspace=output_workspaces[index], OutputWorkspace=output_name,
                                            XMin=mask_params[0], XMax=mask_params[1])
            output_workspaces[index] = out_workspace

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
    return all_banks_masking_list


def _spline_vanadium_for_focusing(vanadium_spectra_list, spline_coefficient, mask_list):
        masked_workspace = _apply_bragg_peaks_masking(workspaces_to_mask=vanadium_spectra_list,
                                                      mask_list=mask_list)
        index = 0
        output_list = []
        for ws in masked_workspace:
            index += 1
            output_ws_name = "splined_vanadium_ws-" + str(index)
            splined_ws = mantid.SplineBackground(InputWorkspace=ws, OutputWorkspace=output_ws_name,
                                                 WorkspaceIndex=0, NCoeff=spline_coefficient)
            output_list.append(splined_ws)

        return output_list
