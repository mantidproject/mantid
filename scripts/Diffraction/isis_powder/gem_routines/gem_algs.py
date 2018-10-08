from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import absorb_corrections, common
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict
from isis_powder.gem_routines import gem_advanced_config


def calculate_van_absorb_corrections(ws_to_correct, multiple_scattering, is_vanadium):
    # First 100 detectors are monitors or not connected to DAE
    mantid.MaskDetectors(ws_to_correct, SpectraList=range(1, 101))

    absorb_dict = gem_advanced_config.absorption_correction_params
    sample_details_obj = absorb_corrections.create_vanadium_sample_details_obj(config_dict=absorb_dict)
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, sample_details_obj=sample_details_obj,
        is_vanadium=True)
    return ws_to_correct


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    # Get empty and vanadium

    mode_run_numbers = _get_current_mode_dictionary(run_number_string, inst_settings)

    empty_runs = _get_run_numbers_for_key(current_mode_run_numbers=mode_run_numbers, key="empty_run_numbers")
    vanadium_runs = _get_run_numbers_for_key(current_mode_run_numbers=mode_run_numbers, key="vanadium_run_numbers")

    grouping_file_name = inst_settings.grouping_file_name

    return create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                     is_vanadium_run=is_vanadium_run, empty_run_number=empty_runs,
                                     grouping_file_name=grouping_file_name, vanadium_string=vanadium_runs)


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key,
                                                append_to_error_message=err_message)


def _get_current_mode_dictionary(run_number_string, inst_settings):
    mapping_dict = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    # Get the current mode "Rietveld" or "PDF" run numbers
    return common.cal_map_dictionary_key_helper(mapping_dict, inst_settings.mode)


def save_maud(d_spacing_group, output_path):
    for i, ws in enumerate(d_spacing_group):
        mantid.SaveFocusedXYE(InputWorkspace=ws, Filename=output_path, SplitFiles=False, StartAtBankNumber=i,
                              Append=i > 0, IncludeHeader=True, Format="MAUD")


def save_angles(d_spacing_group, output_path):
    mantid.SaveBankScatteringAngles(InputWorkspace=d_spacing_group, Filename=output_path)


def save_maud_calib(d_spacing_group, output_path, gsas_calib_filename, grouping_scheme):
    mantid.SaveGEMMAUDParamFile(InputWorkspace=d_spacing_group,
                                GSASParamFile=gsas_calib_filename,
                                GroupingScheme=grouping_scheme,
                                OutputFilename=output_path)
