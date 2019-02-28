# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.hrpd_routines import hrpd_advanced_config
from isis_powder.routines import common, absorb_corrections, sample_details, common_enums
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict


def calculate_van_absorb_corrections(ws_to_correct, multiple_scattering):
    absorb_dict = hrpd_advanced_config.absorption_correction_params
    sample_details_obj = absorb_corrections.create_vanadium_sample_details_obj(config_dict=absorb_dict)
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, sample_details_obj=sample_details_obj,
        is_vanadium=True)
    return ws_to_correct


def calculate_slab_absorb_corrections(ws_to_correct, sample_details_obj):
    """
    Sets a slab sample from the user specified dictionary and performs HRPDSlabCanAbsorption on the workspace.
    The SampleDetails object defines the sample, material and associated properties.
    :param ws_to_correct: The workspace to do corrections on
    :param sample_details_obj: The object containing the sample details
    :return: The corrected workspace
    """

    if not isinstance(sample_details_obj, sample_details.SampleDetails):
        raise RuntimeError("A SampleDetails object was not set or a different object type was found when sample"
                           " absorption corrections were requested. If you want sample absorption corrections please "
                           "create a SampleDetails object and set the relevant properties it. "
                           "Then set the new sample by calling set_sample_details().")
    if not sample_details_obj.is_material_set():
        raise RuntimeError("The material for this sample has not been set yet. Please call"
                           " set_material on the SampleDetails object to set the material")

    geometry_json = {"Shape": "FlatPlate", "Thick": sample_details_obj.thickness(), "Width": sample_details_obj.width(),
                     "Height": sample_details_obj.height(), "Center": sample_details_obj.center(),
                     "Angle": sample_details_obj.angle()}
    material = sample_details_obj.material_object
    # See SetSampleMaterial for documentation on this dictionary
    material_json = {"ChemicalFormula": material.chemical_formula}
    if material.number_density:
        material_json["SampleNumberDensity"] = material.number_density
    if material.absorption_cross_section:
        material_json["AttenuationXSection"] = material.absorption_cross_section
    if material.scattering_cross_section:
        material_json["ScatteringXSection"] = material.scattering_cross_section

    mantid.SetSample(InputWorkspace=ws_to_correct, Geometry=geometry_json, Material=material_json)
    print(str(material_json))
    previous_units = ws_to_correct.getAxis(0).getUnit().unitID()
    ws_units = common_enums.WORKSPACE_UNITS

    # HRPDSlabCanAbsorption must be completed in units of wavelength - convert if needed, than convert back afterwards
    if previous_units != ws_units.wavelength:
        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                            Target=ws_units.wavelength)

    absorb_factors = mantid.HRPDSlabCanAbsorption(InputWorkspace=ws_to_correct,
                                                  Thickness=str(sample_details_obj.thickness()))
    ws_to_correct = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=absorb_factors,
                                  OutputWorkspace=ws_to_correct)
    mantid.DeleteWorkspace(Workspace=absorb_factors)

    if previous_units != ws_units.wavelength:
        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                            Target=previous_units)

    return ws_to_correct


def get_run_details(run_number_string, inst_settings, is_vanadium):
    # Drill down to relevant section
    run_mapping_dict = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    inst_mode_dict = common.cal_map_dictionary_key_helper(run_mapping_dict, key=inst_settings.mode)
    tof_window = common.cal_map_dictionary_key_helper(dictionary=inst_mode_dict, key=inst_settings.tof_window)

    empty_run = _get_run_numbers_for_key(tof_window, key="empty_run_numbers")
    vanadium_run = _get_run_numbers_for_key(tof_window, key="vanadium_run_numbers")

    grouping_file_name = inst_settings.grouping_file_name

    return create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                     is_vanadium_run=is_vanadium, empty_run_number=empty_run,
                                     vanadium_string=vanadium_run, grouping_file_name=grouping_file_name)


def _get_run_numbers_for_key(tof_dict, key):
    err_message = "this must be under 'coupled' or 'decoupled' and the time of flight window eg 10-110."
    return common.cal_map_dictionary_key_helper(tof_dict, key=key,
                                                append_to_error_message=err_message)


def process_vanadium_for_focusing(bank_spectra, spline_number):
    output = common.spline_workspaces(num_splines=spline_number, focused_vanadium_spectra=bank_spectra)
    return output


# The following 2 functions may be moved to common
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
