from __future__ import (absolute_import, division, print_function)

file_names = {
    "grouping_file_name" : "hrpd_new_072_01_corr.cal"
}

general_params = {
    "spline_coefficient": 70,
    "focused_bin_widths": [
        -0.0005,
        -0.0005,
        -0.001
    ]
}

coupled_params = {
    "vanadium_tof_cropping": (1e4, 1.2e5),
    "focused_cropping_values" : [
        (10000, 110000),  # Bank 1
        (10000, 115000),  # Bank 2
        (11000, 110000)   # Bank 3
    ]
}

decoupled_params = {
    # TODO: Implement these
}


def get_all_adv_variables(is_decoupled_mode=False):
    advanced_config_dict = {}
    advanced_config_dict.update(file_names)
    advanced_config_dict.update(general_params)
    advanced_config_dict.update(get_decoupled_mode_dict(decoupled_mode=is_decoupled_mode))
    return advanced_config_dict


def get_decoupled_mode_dict(decoupled_mode):
    if decoupled_mode:
        return decoupled_params
    else:
        return coupled_params
