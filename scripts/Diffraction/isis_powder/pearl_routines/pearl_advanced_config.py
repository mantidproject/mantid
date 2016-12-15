from __future__ import (absolute_import, division, print_function)

general_params = {
    "monitor_spectrum_number": 1,
    "monitor_spline_coefficient": 20,
    "spline_coefficient": 60
}

long_mode_off_params = {
    "file_names": {
        "vanadium_absorb_file": "pearl_absorp_sphere_10mm_newinst2_long.nxs",
        "tt88_grouping": "pearl_group_12_1_TT88.cal",
        "tt70_grouping": "pearl_group_12_1_TT70.cal",
        "tt35_grouping": "pearl_group_12_1_TT35.cal"
    },

    # This needs to be greater than the bank TOF cropping values or you will get data that divides to 0/inf
    "monitor_lambda_crop_range": (0.03, 6.00),
    "monitor_integration_range": (0.6, 5.0),
    "raw_data_tof_cropping": (0, 19995),
    "tof_cropping_ranges": [
        (1500, 19900),  # Bank 1
        (1500, 19900),  # Bank 2
        (1500, 19900),  # Bank 3
        (1500, 19900),  # Bank 4
        (1500, 19900),  # Bank 5
        (1500, 19900),  # Bank 6
        (1500, 19900),  # Bank 7
        (1500, 19900),  # Bank 8
        (1500, 19900),  # Bank 9
        (1500, 19900),  # Bank 10
        (1500, 19900),  # Bank 11
        (1500, 19900),  # Bank 12
        (1500, 19900),  # Bank 13
        (1500, 19900)   # Bank 14
    ]
}

long_mode_on_params = {
    # This needs to be greater than the bank TOF cropping values or you will get data that divides to 0/inf
    "monitor_lambda_crop_range": (5.9, 12.0),
    "monitor_integration_range": (6, 10),
    "raw_data_tof_cropping": (20295, 39995),
    "tof_cropping_ranges": [
        (20300, 39990),  # Bank 1
        (20300, 39990),  # Bank 2
        (20300, 39990),  # Bank 3
        (20300, 39990),  # Bank 4
        (20300, 39990),  # Bank 5
        (20300, 39990),  # Bank 6
        (20300, 39990),  # Bank 7
        (20300, 39990),  # Bank 8
        (20300, 39990),  # Bank 9
        (20300, 39990),  # Bank 10
        (20300, 39990),  # Bank 11
        (20300, 39990),  # Bank 12
        (20300, 39990),  # Bank 13
        (20300, 39990)   # Bank 14
    ]
}


variable_help = {
    "file_names": {
        "vanadium_absorb_file_name": "Takes the name of the calculated vanadium absorption corrections. This file "
                                     " must be located in the top level of the calibration folder",

        "tt88_grouping_name": "The name of the .cal file that defines the grouping of detectors in banks for TT88. "
                              "This file must be located in the top level of the calibration folder.",

        "tt70_grouping_name": "The name of the .cal file that defines the grouping of detectors in banks for TT70. "
                              "This file must be located in the top level of the calibration folder.",

        "tt35_grouping_name": "The name of the .cal file that defines the grouping of detectors in banks for TT35. "
                              "This file must be located in the top level of the calibration folder.",


    }
}


def get_all_adv_variables(is_long_mode_on=False):
    long_mode_params = long_mode_on_params if is_long_mode_on else long_mode_off_params
    advanced_config_dict = {}
    advanced_config_dict.update(general_params)
    advanced_config_dict.update(long_mode_params)
    return advanced_config_dict


def get_long_mode_dict(is_long_mode):
    return long_mode_on_params if is_long_mode else long_mode_off_params
