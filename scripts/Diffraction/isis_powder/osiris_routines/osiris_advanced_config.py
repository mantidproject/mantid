# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Note all changes in this file require a restart of Mantid
# additionally any long term changes should be sent back to the development team so any changes can be merged
# into future versions of Mantid.


general_params = {
    "monitor_spectrum_number": 1,

    "generate_absorb_corrections": False,

    "file_names": {
         "vanadium_absorb_filename": "osiris_absorp_sphere_10mm_newinst2_long.nxs",
         "grouping_filename": "osiris_grouping.cal",
         "nxs_filename": "{instshort}{runno}{suffix}.nxs",
         "gss_filename": "{instshort}{runno}{suffix}.gsas",
         "dat_files_directory": "",
         "tof_xye_filename": "{instshort}{runno}{suffix}_tof.xye",
         "dspacing_xye_filename": "{instshort}{runno}{suffix}_d.xye"
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
    "custom_focused_bin_widths" : -0.0006
}

calibration_params = {
    "create_cal_rebin_2_params": "0.01,0.002,1.0",
    "create_cal_cross_correlate_params": {
        "cross_corr_reference_spectra": 20,
        "cross_corr_ws_index_min": 0,
        "cross_corr_ws_index_max": 1009,
        "cross_corr_x_min": 0.01,
        "cross_corr_x_max": 1.0
    },
    "create_cal_get_detector_offsets_params": {
        "get_det_offsets_step": 0.002,
        "get_det_offsets_x_min": -200,
        "get_det_offsets_x_max": 200,
        "get_det_offsets_d_ref": 1.912795
    }
}

variable_help = {
    "long_mode_<on/off>_params": {
        "file_names": {
            "vanadium_absorb_filename": "Takes the name of the calculated vanadium absorption corrections. This file "
                                        " must be located in the top level of the calibration folder",

            "grouping_filename": "The name of the .cal file that defines the grouping of detectors in banks for "
                                 "This file must be located in the top level of the calibration folder.",
        },

        "monitor_lambda_crop_range": "The range in wavelength to crop a monitor workspace to before calculating "
                                     "the current normalisation",
        "monitor_integration_range": "The minimum and maximum values to consider whilst integrating the monitor "
                                     "workspace",
        "raw_data_tof_cropping": "The crop values for to apply when loading raw data. This step is applied before any "
                                 "processing takes place. This is to crop from 40,000 microseconds in the "
                                 "raw data to 20,000 microseconds worth of data",
        "focused_cropping_values": "These values are used to determine the TOF range to crop a focused (not Vanadium "
                                   "calibration) workspace to. These are applied on a bank by bank basis. They must "
                                   "be less than the values specified for raw_data_tof_cropping.",
        "monitor_spline_coefficient": "The coefficient to use whilst calculating a spline from the monitor."
                                      "workspace. This is used to normalise the workspace current.",
        "spline_coefficient": "The coefficient to use whilst calculating a spline for each bank during "
                              "a vanadium calibration."
    },

    "general_params": {
        "monitor_spectrum_number": "The spectrum number the monitor is located at in the workspace",
    },

    "calibration_params": {
        "create_cal_rebin_1_params": "The parameters for the first rebin step used to create a calibration file",
        "create_cal_rebin_2_params": "The parameters for the second rebin step used to create a calibration file",
        "cross_corr_reference_spectra": "The Workspace Index of the spectra to correlate all other spectra against",
        "cross_corr_ws_index_min": "The workspace index of the first member of the range of spectra to cross-correlate "
                                   "against",
        "cross_corr_ws_index_max": "The workspace index of the last member of the range of spectra to cross-correlate "
                                   "against",
        "cross_corr_x_min": "The starting point of the region to be cross correlated",
        "cross_corr_x_max": "The ending point of the region to be cross correlated",
        "get_det_offsets_step": "Step size used to bin d-spacing data in GetDetectorOffsets",
        "get_det_offsets_x_min": "Minimum of CrossCorrelation data to search for peak, usually negative",
        "get_det_offsets_x_max": "Maximum of CrossCorrelation data to search for peak, usually positive",
        "get_det_offsets_d_ref": "Center of reference peak in d-space"
    }
}


def get_all_adv_variables():
    advanced_config_dict = {}
    advanced_config_dict.update(calibration_params)
    advanced_config_dict.update(general_params)
    return advanced_config_dict
