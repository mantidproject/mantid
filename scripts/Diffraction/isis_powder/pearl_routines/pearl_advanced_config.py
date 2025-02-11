# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from numpy import array

general_params = {
    "monitor_mask_regions": array([[3.45, 2.96, 2.1, 1.73], [3.7, 3.2, 2.26, 1.98]]),
    "monitor_spectrum_number": 1,
    "generate_absorb_corrections": False,
    "file_names": {
        "vanadium_absorb_filename": "pearl_absorp_sphere_10mm_newinst2_long.nxs",
        "tt88_grouping_filename": "pearl_group_12_1_TT88.cal",
        "tt70_grouping_filename": "pearl_group_12_1_TT70.cal",
        "tt35_grouping_filename": "pearl_group_12_1_TT35.cal",
        "nxs_filename": "{instshort}{runno}{suffix}_{tt_mode}{_long_mode}.nxs",
        "gss_filename": "{instshort}{runno}{suffix}_{tt_mode}{_long_mode}.gsas",
        "dat_files_directory": "",
        "tof_xye_filename": "{instshort}{runno}{suffix}_{tt_mode}{_long_mode}_tof.xye",
        "dspacing_xye_filename": "{instshort}{runno}{suffix}_{tt_mode}{_long_mode}_d.xye",
    },
    "subtract_empty_instrument": True,
    "focused_bin_widths": [
        # Note you want these to be negative for logarithmic (dt / t) binning
        -0.0006,  # Bank 1
        -0.0006,  # Bank 2
        -0.0006,  # Bank 3
        -0.0006,  # Bank 4
        -0.0006,  # Bank 5
        -0.0006,  # Bank 6
        -0.0006,  # Bank 7
        -0.0006,  # Bank 8
        -0.0006,  # Bank 9
        -0.0006,  # Bank 10
        -0.0006,  # Bank 11
        -0.0006,  # Bank 12
        -0.0006,  # Bank 13
        -0.0006,  # Bank 14
    ],
    "custom_focused_bin_widths": -0.0006,
}

long_mode_off_params = {
    "create_cal_rebin_1_params": "100,-0.0006,19950",
    "monitor_lambda_crop_range": (0.03, 6.00),
    "monitor_integration_range": (0.6, 5.0),
    # This needs to be greater than the bank TOF cropping values or you will get data that divides to 0/inf
    "raw_data_tof_cropping": (0, 19995),
    "vanadium_tof_cropping": (1400, 19990),
    "focused_cropping_values": [
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
        (1500, 19900),  # Bank 14
    ],
    "custom_focused_cropping_values": (1500, 19990),
    "monitor_spline_coefficient": 20,
    "spline_coefficient": 60,
}

long_mode_on_params = {
    "create_cal_rebin_1_params": "20300,-0.0006,39990",
    "monitor_lambda_crop_range": (5.9, 12.0),
    "monitor_integration_range": (6, 10),
    # raw_data_tof_cropping needs to be have smaller/larger values than the bank TOF cropping values or
    # you will get data that divides to 0 or inf
    "raw_data_tof_cropping": (20280, 39000),
    "vanadium_tof_cropping": (20295, 39000),
    "focused_cropping_values": [
        (20300, 38830),  # Bank 1
        (20300, 38830),  # Bank 2
        (20300, 38830),  # Bank 3
        (20300, 38830),  # Bank 4
        (20300, 38830),  # Bank 5
        (20300, 38830),  # Bank 6
        (20300, 38830),  # Bank 7
        (20300, 38830),  # Bank 8
        (20300, 38830),  # Bank 9
        (20300, 38830),  # Bank 10
        (20300, 38830),  # Bank 11
        (20300, 38830),  # Bank 12
        (20300, 38830),  # Bank 13
        (20300, 38830),  # Bank 14
    ],
    "custom_focused_cropping_values": (20300, 38830),
    "monitor_spline_coefficient": 20,
    "spline_coefficient": 5,
}

calibration_params = {
    "create_cal_rebin_2_params": "1.8,0.002,2.1",
    "create_cal_cross_correlate_params": {
        "cross_corr_reference_spectra": 20,
        "cross_corr_ws_index_min": 9,
        "cross_corr_ws_index_max": 1063,
        "cross_corr_x_min": 1.8,
        "cross_corr_x_max": 2.1,
    },
    "create_cal_get_detector_offsets_params": {
        "get_det_offsets_step": 0.002,
        "get_det_offsets_x_min": -200,
        "get_det_offsets_x_max": 200,
        "get_det_offsets_d_ref": 1.912795,
    },
}

variable_help = {
    "long_mode_<on/off>_params": {
        "file_names": {
            "vanadium_absorb_filename": "Takes the name of the calculated vanadium absorption corrections. This file "
            " must be located in the top level of the calibration folder",
            "tt88_grouping_filename": "The name of the .cal file that defines the grouping of detectors in banks for "
            "TT88. This file must be located in the top level of the calibration folder.",
            "tt70_grouping_filename": "The name of the .cal file that defines the grouping of detectors in banks for "
            "TT70. This file must be located in the top level of the calibration folder.",
            "tt35_grouping_filename": "The name of the .cal file that defines the grouping of detectors in banks for "
            "TT35. This file must be located in the top level of the calibration folder.",
        },
        "monitor_lambda_crop_range": "The range in wavelength to crop a monitor workspace to before calculating the current normalisation",
        "monitor_integration_range": "The minimum and maximum values to consider whilst integrating the monitor workspace",
        "raw_data_tof_cropping": "The crop values for to apply when loading raw data. This step is applied before any "
        "processing takes place. This is to crop from 40,000 microseconds in the "
        "raw data to 20,000 microseconds worth of data",
        "focused_cropping_values": "These values are used to determine the TOF range to crop a focused (not Vanadium "
        "calibration) workspace to. These are applied on a bank by bank basis. They must "
        "be less than the values specified for raw_data_tof_cropping.",
        "monitor_spline_coefficient": "The coefficient to use whilst calculating a spline from the monitor."
        "workspace. This is used to normalise the workspace current.",
        "spline_coefficient": "The coefficient to use whilst calculating a spline for each bank during a vanadium calibration.",
    },
    "general_params": {
        "monitor_spectrum_number": "The spectrum number the monitor is located at in the workspace",
    },
    "calibration_params": {
        "create_cal_rebin_1_params": "The parameters for the first rebin step used to create a calibration file",
        "create_cal_rebin_2_params": "The parameters for the second rebin step used to create a calibration file",
        "cross_corr_reference_spectra": "The Workspace Index of the spectra to correlate all other spectra against",
        "cross_corr_ws_index_min": "The workspace index of the first member of the range of spectra to cross-correlate against",
        "cross_corr_ws_index_max": "The workspace index of the last member of the range of spectra to cross-correlate against",
        "cross_corr_x_min": "The starting point of the region to be cross correlated",
        "cross_corr_x_max": "The ending point of the region to be cross correlated",
        "get_det_offsets_step": "Step size used to bin d-spacing data in GetDetectorOffsets",
        "get_det_offsets_x_min": "Minimum of CrossCorrelation data to search for peak, usually negative",
        "get_det_offsets_x_max": "Maximum of CrossCorrelation data to search for peak, usually positive",
        "get_det_offsets_d_ref": "Center of reference peak in d-space",
    },
}


def get_all_adv_variables(is_long_mode_on=False):
    long_mode_params = long_mode_on_params if is_long_mode_on else long_mode_off_params
    advanced_config_dict = {}
    advanced_config_dict.update(calibration_params)
    advanced_config_dict.update(general_params)
    advanced_config_dict.update(long_mode_params)
    return advanced_config_dict


def get_long_mode_dict(is_long_mode):
    return long_mode_on_params if is_long_mode else long_mode_off_params
