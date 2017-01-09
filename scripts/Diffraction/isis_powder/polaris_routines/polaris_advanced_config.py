file_names = {
    "masking_file_name": "VanaPeaks.dat"
}

script_params = {
    "apply_solid_angle": False,
    "raw_data_cropping_values": (750, 20000),
    "spline_coefficient": 100,
    "vanadium_cropping_values": (800, 19995)
}

tof_cropping_ranges = [
    (1500, 19900),  # Bank 1
    (1500, 19900),  # Bank 2
    (1500, 19900),  # Bank 3
    (1500, 19900),  # Bank 4
    (1500, 19900),  # Bank 5
    ]

absorption_correction_params = {
    # These are read directly by the generate absorb corrections functions instead of being parsed.
    # Therefore they cannot be overridden using basic config files or keyword arguments.

    # For documentation on their behaviour please see:
    # http://docs.mantidproject.org/nightly/algorithms/CylinderAbsorption-v1.html
    "cylinder_sample_height": 4.0,
    "cylinder_sample_radius": 0.4,

    "attenuation_cross_section": 4.88350,
    "scattering_cross_section": 5.15775,
    "sample_number_density": 0.0718956,

    "number_of_slices": 10,
    "number_of_annuli": 10,
    "number_of_wavelength_points": 100,
    "exponential_method": "Normal"
}

variable_help = {
    "file_names": {
        "masking_file_name": "Specifies the name of the of the file containing the positions of the  Bragg Peaks to "
                             "mask out. This must be located at the root of the calibration folder the user has "
                             "specified."
    },

    "script_params": {
        "apply_solid_angle": "Specifies if the script should perform solid angle corrections. This is usually "
                             "defaulted to off and should be overridden in the basic configuration when necessary to "
                             "avoid them being performed accidentally.",
        "raw_data_cropping_values": "This specifies the valid range in TOF of the raw data. This is applied before any "
                                    "processing takes place to remove negative counts at very low TOF values",
        "spline_coefficient": "The coefficient to use when calculating the vanadium splines during the calibration "
                              "step."
    },

    "tof_cropping_ranges": "These values are used to determine the TOF range to crop a focused (not Vanadium Cal.) "
                           "workspace to. These are applied on a bank by bank basis. They must be less than "
                           "the values specified for raw_data_cropping_values."
}

variables = {
    # Used by the script to find the dictionaries in advanced config.
    "file_names_dict": file_names,
    "script_params": script_params,
    "tof_cropping_ranges": tof_cropping_ranges
}
