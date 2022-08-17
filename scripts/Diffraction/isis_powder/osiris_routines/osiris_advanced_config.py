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
         "nxs_filename": "{instshort}{runno}{suffix}{unit}.nxs",
         "gss_filename": "{instshort}{runno}{suffix}{unit}.gsas",
         "dat_files_directory": "",
         "xye_filename": "{instshort}{runno}{suffix}{unit}.xye",
    },

    "subtract_empty_can": True,
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
