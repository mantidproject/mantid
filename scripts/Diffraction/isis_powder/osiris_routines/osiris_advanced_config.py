# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Note all changes in this file require a restart of Mantid
# additionally any long term changes should be sent back to the development team so any changes can be merged
# into future versions of Mantid.


general_params = {"file_names": {
         "grouping_filename": "osiris_grouping.cal",
         "nxs_filename": "{instshort}{runno}{suffix}{unit}.nxs",
         "gss_filename": "{instshort}{runno}{suffix}{unit}.gsas",
         "dat_files_directory": "",
         "xye_filename": "{instshort}{runno}{suffix}{unit}.xye",
    }
}


def get_all_adv_variables():
    advanced_config_dict = {}
    advanced_config_dict.update(general_params)
    return advanced_config_dict
