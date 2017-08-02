from __future__ import (absolute_import, division, print_function)

from isis_powder.routines.run_details import create_run_details_object, \
                                             RunDetailsWrappedCommonFuncs, CustomFuncForRunDetails

def hrpd_get_inst_mode(forwarded_value, inst_settings):
    raise NotImplementedError("hrpd_get_inst_mode")

def get_run_details(run_number_string, inst_settings, is_vanadium):
    cal_mapping_callable = CustomFuncForRunDetails().add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.get_cal_mapping_dict,
        run_number_string=run_number_string, inst_settings=inst_settings)

    mapping_dict_callable = cal_mapping_callable.add_to_func_chain(user_function=hrpd_get_inst_mode,
                                                                   inst_settings=inst_settings)

    empty_run_callable = mapping_dict_callable.add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.cal_dictionary_key_helper, key="empty_run_numbers")

    vanadium_run_callable = mapping_dict_callable.add_to_func_chain(
        user_function=RunDetailsWrappedCommonFuncs.cal_dictionary_key_helper, key="vanadium_run_numbers")

    return create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                     is_vanadium_run=is_vanadium, empty_run_call=empty_run_callable,
                                     vanadium_run_call=vanadium_run_callable)
