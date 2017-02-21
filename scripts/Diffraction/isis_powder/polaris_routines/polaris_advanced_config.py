# Note all changes in this file require a restart of Mantid
# additionally any long term changes should be sent back to the development team so any changes can be merged
# into future versions of Mantid.

absorption_correction_params = {
    # These are read directly by the generate absorb corrections functions instead of being parsed.
    # Therefore they cannot be overridden using basic config files or keyword arguments.
    "cylinder_sample_height": 4.0,
    "cylinder_sample_radius": 0.4,
    "cylinder_position": [0., 0., 0.],

    "chemical_formula": "V",
}

file_names = {
    "masking_file_name": "VanaPeaks.dat",
    "grouping_file_name": "Master_copy_of_grouping_file_with_essential_masks.cal"
}

script_params = {
    "raw_data_cropping_values": (750, 20000),
    "spline_coefficient": 100,
}

focused_cropping_values = [
    (1500, 19900),  # Bank 1
    (1500, 19900),  # Bank 2
    (1500, 19900),  # Bank 3
    (1500, 19900),  # Bank 4
    (1500, 19900),  # Bank 5
    ]

vanadium_cropping_values = [
    (800, 19995),  # Bank 1
    (800, 19995),  # Bank 2
    (800, 19995),  # Bank 3
    (800, 19995),  # Bank 4
    (800, 19995),  # Bank 5
]

variable_help = {
    "file_names": {
        "masking_file_name": "Specifies the name of the of the file containing the positions of the  Bragg Peaks to "
                             "mask out. This must be located at the root of the calibration folder the user has "
                             "specified."
    },

    "script_params": {
        "raw_data_cropping_values": "This specifies the valid range in TOF of the raw data. This is applied before any "
                                    "processing takes place to remove negative counts at very low TOF values",
        "spline_coefficient": "The coefficient to use when calculating the vanadium splines during the calibration "
                              "step."
    },

    "focused_cropping_values": "These values are used to determine the TOF range to crop a focused (not Vanadium Cal.) "
                               "workspace to. These are applied on a bank by bank basis. They must be less than "
                               "the values specified for raw_data_cropping_values.",

    "vanadium_cropping_values": "These values are use to determine the TOF range to crop a vanadium workspace to during"
                                " calibration step. These are applied on a bank by bank basis and must be smaller than"
                                " the range specified in raw_data_cropping_values and larger than the values specified"
                                " in focused_cropping_values."
}

variables = {
    # Used by the script to find the dictionaries in advanced config.
    "file_names_dict": file_names,
    "script_params": script_params,
    "focused_cropping_values": focused_cropping_values,
    "vanadium_cropping_values": vanadium_cropping_values
}
