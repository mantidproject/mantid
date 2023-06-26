# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.routines.common import PARAM_MAPPING
from isis_powder.routines.common_enums import EMPTY_CAN_SUBTRACTION_METHOD


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
    ParamMapEntry(ext_name="sample_empty_scale", int_name="sample_empty_scale", optional=True),
    ParamMapEntry(ext_name="keep_raw_workspace", int_name="keep_raw_workspace", optional=True),
    ParamMapEntry(ext_name="mayers_mult_scat_events", int_name="mayers_mult_scat_events", optional=True),
    ParamMapEntry(ext_name="absorb_corrections", int_name="absorb_corrections"),
    ParamMapEntry(
        ext_name="empty_can_subtraction_method",
        int_name="empty_can_subtraction_method",
        enum_class=EMPTY_CAN_SUBTRACTION_METHOD,
        optional=True,
    ),
    ParamMapEntry(ext_name="paalman_pings_events_per_point", int_name="paalman_pings_events_per_point", optional=True),
    ParamMapEntry(ext_name="simple_events_per_point", int_name="simple_events_per_point", optional=True),
    ParamMapEntry(ext_name="multiple_scattering", int_name="multiple_scattering", optional=True),
    ParamMapEntry(ext_name="neutron_paths_single", int_name="neutron_paths_single", optional=True),
    ParamMapEntry(ext_name="neutron_paths_multiple", int_name="neutron_paths_multiple", optional=True),
]
attr_mapping.extend(PARAM_MAPPING)
