from __future__ import (absolute_import, division, print_function)

file_names = {
    "grouping_file_name" : "hrpd_new_072_01_corr.cal"
}

general_params = {
    "spline_coefficient": 70
}


def get_all_adv_variables():
    advanced_config_dict = {}
    advanced_config_dict.update(file_names)
    advanced_config_dict.update(general_params)
    return advanced_config_dict
