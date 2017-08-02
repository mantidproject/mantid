from __future__ import (absolute_import, division, print_function)

from isis_powder.routines.param_map_entry import ParamMapEntry

attr_mapping = \
    [
        ParamMapEntry(ext_name="calibration_directory",     int_name="calibration_dir"),
        ParamMapEntry(ext_name="calibration_mapping_file",  int_name="cal_mapping_path"),
        ParamMapEntry(ext_name="do_absorb_corrections",     int_name="do_absorb_corrections"),
        ParamMapEntry(ext_name="file_ext",                  int_name="file_extension", optional=True),
        ParamMapEntry(ext_name="output_directory",          int_name="output_dir"),
        ParamMapEntry(ext_name="run_in_cycle",              int_name="run_in_range"),
        ParamMapEntry(ext_name="run_number",                int_name="run_number"),
        ParamMapEntry(ext_name="user_name",                 int_name="user_name"),
        ParamMapEntry(ext_name="vanadium_normalisation",    int_name="do_van_norm")
    ]
