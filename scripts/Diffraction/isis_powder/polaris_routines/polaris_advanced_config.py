file_names = {
    "masking_file_name": "VanaPeaks.dat"
}

script_params = {
    "apply_solid_angle": False,
    "spline_coefficient": 100
}

tof_cropping_ranges = [
    (1500, 19900),  # Bank 1
    (1500, 19900),  # Bank 2
    (1500, 19900),  # Bank 3
    (1500, 19900),  # Bank 4
    (1500, 19900),  # Bank 5
    ]

variables = {
    "file_names_dict": file_names,
    "script_params": script_params,
    "tof_cropping_ranges": tof_cropping_ranges
}

absorption_correction_params = {
    # These are read directly by the generate absorb corrections functions instead of being parsed
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
