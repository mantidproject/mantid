# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.routines.common import PARAM_MAPPING


# Maps friendly user name (ext_name) -> script name (int_name)
attr_mapping = [
    ParamMapEntry(ext_name="config_file", int_name="config_file_name"),
    ParamMapEntry(ext_name="calibration_mapping_file", int_name="cal_mapping_path"),
    ParamMapEntry(ext_name="calibration_directory", int_name="calibration_dir"),
    ParamMapEntry(ext_name="file_ext", int_name="file_extension", optional=True),
    ParamMapEntry(ext_name="merge_drange", int_name="merge_drange"),
    ParamMapEntry(ext_name="output_directory", int_name="output_dir"),
    ParamMapEntry(ext_name="run_number", int_name="run_number"),
    ParamMapEntry(ext_name="subtract_empty_can", int_name="subtract_empty_can"),
    ParamMapEntry(ext_name="suffix", int_name="suffix", optional=True),
    ParamMapEntry(ext_name="grouping_filename", int_name="grouping"),
    ParamMapEntry(ext_name="user_name", int_name="user_name"),
    ParamMapEntry(ext_name="vanadium_normalisation", int_name="van_norm"),
    ParamMapEntry(ext_name="xye_filename", int_name="xye_filename"),
]
attr_mapping.extend(PARAM_MAPPING)
