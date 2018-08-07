from __future__ import (absolute_import, division, print_function)

from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.gem_routines.gem_enums import GEM_CHOPPER_MODES
from isis_powder.routines.common_enums import INPUT_BATCHING, WORKSPACE_UNITS

#                 Maps friendly user name (ext_name) -> script name (int_name)
attr_mapping = \
    [
     ParamMapEntry(ext_name="calibration_directory",     int_name="calibration_dir"),
     ParamMapEntry(ext_name="calibration_mapping_file",  int_name="cal_mapping_path"),
     ParamMapEntry(ext_name="config_file",               int_name="config_file"),
     ParamMapEntry(ext_name="do_absorb_corrections",     int_name="do_absorb_corrections"),
     ParamMapEntry(ext_name="file_ext",                  int_name="file_extension", optional=True),
     ParamMapEntry(ext_name="first_cycle_run_no",        int_name="run_in_range"),
     ParamMapEntry(ext_name="focused_cropping_values",   int_name="focused_cropping_values"),
     ParamMapEntry(ext_name="grouping_file_name",        int_name="grouping_file_name"),
     ParamMapEntry(ext_name="gsas_calib_filename",       int_name="gsas_calib_filename"),
     ParamMapEntry(ext_name="input_mode",                int_name="input_batching", enum_class=INPUT_BATCHING),
     ParamMapEntry(ext_name="maud_grouping_scheme",      int_name="maud_grouping_scheme"),
     ParamMapEntry(ext_name="mode",                      int_name="mode",           enum_class=GEM_CHOPPER_MODES),
     ParamMapEntry(ext_name="multiple_scattering",       int_name="multiple_scattering"),
     ParamMapEntry(ext_name="raw_tof_cropping_values",   int_name="raw_tof_cropping_values"),
     ParamMapEntry(ext_name="run_number",                int_name="run_number"),
     ParamMapEntry(ext_name="sample_empty",              int_name="sample_empty",   optional=True),
     ParamMapEntry(ext_name="sample_empty_scale",        int_name="sample_empty_scale"),
     ParamMapEntry(ext_name="save_angles",               int_name="save_angles"),
     ParamMapEntry(ext_name="save_maud_calib",           int_name="save_maud_calib"),
     ParamMapEntry(ext_name="save_maud",                 int_name="save_maud"),
     ParamMapEntry(ext_name="spline_coefficient",        int_name="spline_coeff"),
     ParamMapEntry(ext_name="suffix",                    int_name="suffix",         optional=True),
     ParamMapEntry(ext_name="texture_mode",              int_name="texture_mode",         optional=True),
     ParamMapEntry(ext_name="output_directory",          int_name="output_dir"),
     ParamMapEntry(ext_name="unit_to_keep",              int_name="unit_to_keep",
                   enum_class=WORKSPACE_UNITS,           optional=True),
     ParamMapEntry(ext_name="user_name",                 int_name="user_name"),
     ParamMapEntry(ext_name="vanadium_cropping_values",  int_name="vanadium_cropping_values"),
     ParamMapEntry(ext_name="vanadium_normalisation",    int_name="do_van_norm")
    ]
