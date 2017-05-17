from __future__ import (absolute_import, division, print_function)

absorption_correction_params = {
    # These are read directly by the generate absorb corrections functions instead of being parsed.
    # Therefore they cannot be overridden using basic config files or keyword arguments.
    "cylinder_sample_height": 4.0,
    "cylinder_sample_radius": 0.4,
    "cylinder_position": [0., 0., 0.],

    "chemical_formula": "V0.9486 Nb0.0514",  # V 94.86% Nb 5.14%
    "number_density": 0.071
}


gem_adv_config_params = {
    "grouping_file_name": "GEM_Instrument_grouping.cal",
    "raw_tof_cropping_values": (500, 20000),
    "spline_coefficient": 30
}

focused_cropping_values = [(550, 19900),  # Bank 1
                           (550, 19900),  # Bank 2
                           (550, 19900),  # Bank 3
                           (550, 19900),  # Bank 4
                           (550, 19480),  # Bank 5
                           (550, 17980)   # Bank 6
                           ]

vanadium_cropping_values = [(510, 19997),  # Bank 1
                            (510, 19997),  # Bank 2
                            (510, 19997),  # Bank 3
                            (510, 19997),  # Bank 4
                            (510, 19500),  # Bank 5
                            (510, 18000)   # Bank 6
                            ]


all_adv_variables = {
    "raw_tof_cropping_values": gem_adv_config_params,
    "focused_cropping_values": focused_cropping_values,
    "vanadium_cropping_values": vanadium_cropping_values
}


def get_all_adv_variables():
    return all_adv_variables
