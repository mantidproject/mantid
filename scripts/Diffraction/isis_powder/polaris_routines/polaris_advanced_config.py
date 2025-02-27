# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Note all changes in this file require a restart of Mantid
# additionally any long term changes should be sent back to the development team so any changes can be merged
# into future versions of Mantid.
import copy

from isis_powder.routines.common import ADVANCED_CONFIG as COMMON_ADVANCED_CONFIG


absorption_correction_params = {
    # These are read directly by the generate absorb corrections functions instead of being parsed.
    # Therefore they cannot be overridden using basic config files or keyword arguments.
    "cylinder_sample_height": 4.0,
    "cylinder_sample_radius": 0.25,
    "cylinder_position": [0.0, 0.0, 0.0],
    "chemical_formula": "V",
}

file_names = {
    "vanadium_peaks_masking_file": "VanaPeaks.dat",
    "grouping_file_name": "Master_copy_of_grouping_file_with_essential_masks.cal",
}

script_params = {"raw_data_cropping_values": (750, 20000), "spline_coefficient": 100, "spline_coeff_per_detector": 10}

pdf_focused_cropping_values = [
    (700, 30000),  # Bank 1
    (1200, 24900),  # Bank 2
    (1100, 19950),  # Bank 3
    (1100, 19950),  # Bank 4
    (1100, 19950),  # Bank 5
]

rietveld_focused_cropping_values = [
    (700, 30000),  # Bank 1
    (1200, 24900),  # Bank 2
    (1100, 19950),  # Bank 3
    (1100, 19950),  # Bank 4
    (1100, 19950),  # Bank 5
]

focused_bin_widths = [
    # Note you want these to be negative for logarithmic (dt / t) binning
    # else the output file will be larger than 1GB
    -0.0050,  # Bank 1
    -0.0010,  # Bank 2
    -0.0010,  # Bank 3
    -0.0010,  # Bank 4
    -0.0005,  # Bank 5
]

vanadium_cropping_values = [
    (600, 31000),  # Bank 1
    (1000, 24950),  # Bank 2
    (1000, 19975),  # Bank 3
    (900, 19975),  # Bank 4
    (800, 19975),  # Bank 5
]

sample_empty_scale = 1.0
# fmt: off
variable_help = {
    "file_names": {
        "vanadium_peaks_masking_file": "Specifies the name of the of the file containing the positions of the vanadium "
        "Bragg Peaks to mask out. This must be located at the root of the calibration "
        "folder the user has specified."
    },
    "script_params": {
        "raw_data_cropping_values": "This specifies the valid range in TOF of the raw data. This is applied before any "
        "processing takes place to remove negative counts at very low TOF values",
        "spline_coefficient": "The coefficient to use when calculating the vanadium splines during the calibration step.",
        "spline_coefficient_per_detector":
            "The coefficient to use when calculating the vanadium splines during the per detector calibration ",
    },
    "focused_cropping_values": "These values are used to determine the TOF range to crop a focused (not Vanadium Cal.) "
    "workspace to. These are applied on a bank by bank basis. They must be less than "
    "the values specified for raw_data_cropping_values.",
    "vanadium_cropping_values": "These values are use to determine the TOF range to crop a vanadium workspace to during"
    " calibration step. These are applied on a bank by bank basis and must be smaller than"
    " the range specified in raw_data_cropping_values and larger than the values specified"
    " in focused_cropping_values.",
}
# fmt: on
variables = {
    # Used by the script to find the dictionaries in advanced config.
    "file_names_dict": file_names,
    "script_params": script_params,
    "vanadium_cropping_values": vanadium_cropping_values,
    "focused_bin_widths": focused_bin_widths,
    "sample_empty_scale": sample_empty_scale,
}


def get_mode_specific_dict(mode):
    if mode is None:
        return {"focused_cropping_values": "auto", "van_normalisation_method": "Relative"}
    mode = mode.lower()
    if mode == "pdf":
        return {"focused_cropping_values": pdf_focused_cropping_values, "van_normalisation_method": "Relative"}
    if mode == "pdf_norm":
        # In long run this will replace gudrun
        return {"focused_cropping_values": pdf_focused_cropping_values, "van_normalisation_method": "Absolute"}
    if mode == "rietveld":
        return {"focused_cropping_values": rietveld_focused_cropping_values, "van_normalisation_method": "Relative"}
    raise ValueError('Invalid chopper mode: "{}"'.format(mode))


def get_all_adv_variables(mode=None):
    advanced_config_dict = copy.copy(COMMON_ADVANCED_CONFIG)
    advanced_config_dict.update(variables)
    advanced_config_dict.update(get_mode_specific_dict(mode))
    return advanced_config_dict
