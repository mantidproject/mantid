file_names = {
    "vanadium_absorb_file": "pearl_absorp_sphere_10mm_newinst2_long.nxs",
    "tt88_grouping": "pearl_group_12_1_TT88.cal",
    "tt70_grouping": "pearl_group_12_1_TT70.cal",
    "tt35_grouping": "pearl_group_12_1_TT35.cal"
}

tof_cropping_ranges = [
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

script_params = {
    "spline_coefficient": 60,
    "bank_tof_crop_values": tof_cropping_ranges,
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

variables = {
    "file_names_dict": file_names,
    "script_params_dict": script_params
}
