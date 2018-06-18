from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import absorb_corrections, common
from isis_powder.routines.run_details import create_run_details_object, \
                                             RunDetailsWrappedCommonFuncs, CustomFuncForRunDetails
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


def gem_get_chopper_config(forwarded_value, inst_settings):
    # Forwarded value should be a cal mapping
    cal_mapping = forwarded_value
    return common.cal_map_dictionary_key_helper(cal_mapping, inst_settings.mode)


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    cal_mapping_callable = CustomFuncForRunDetails().add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.get_cal_mapping_dict, run_number_string=run_number_string,
        inst_settings=inst_settings
    ).add_to_func_chain(user_function=gem_get_chopper_config, inst_settings=inst_settings)

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


def save_maud(d_spacing_group, output_path):
    for i, ws in enumerate(d_spacing_group):
        mantid.SaveFocusedXYE(InputWorkspace=ws, Filename=output_path, SplitFiles=False, StartAtBankNumber=i,
                              Append=i > 0, IncludeHeader=True, Format="MAUD")


def save_angles(d_spacing_group, output_path):
    mantid.SaveBankScatteringAngles(InputWorkspace=d_spacing_group, Filename=output_path)
