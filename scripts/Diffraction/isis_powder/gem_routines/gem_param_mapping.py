# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.gem_routines.gem_enums import GEM_CHOPPER_MODES
from isis_powder.routines.common_enums import INPUT_BATCHING, WORKSPACE_UNITS

#                 Maps friendly user name (ext_name) -> script name (int_name)
attr_mapping = \
    [
     ParamMapEntry(ext_name="calibration_to_adjust",     int_name="cal_adjust", optional=True),
     ParamMapEntry(ext_name="calibration_directory",     int_name="calibration_dir"),
     ParamMapEntry(ext_name="calibration_mapping_file",  int_name="cal_mapping_path"),
     ParamMapEntry(ext_name="config_file",               int_name="config_file"),

     ParamMapEntry(ext_name="create_cal_rebin_1_params", int_name="cal_rebin_1"),
     ParamMapEntry(ext_name="create_cal_rebin_2_params", int_name="cal_rebin_2"),
     ParamMapEntry(ext_name="cross_corr_reference_spectra", int_name="reference_spectra"),
     ParamMapEntry(ext_name="cross_corr_ws_index_max", int_name="cross_corr_ws_max"),
     ParamMapEntry(ext_name="cross_corr_ws_index_min", int_name="cross_corr_ws_min"),
     ParamMapEntry(ext_name="cross_corr_x_min", int_name="cross_corr_x_min"),
     ParamMapEntry(ext_name="cross_corr_x_max", int_name="cross_corr_x_max"),

     ParamMapEntry(ext_name="get_det_offsets_d_ref", int_name="d_reference"),
     ParamMapEntry(ext_name="get_det_offsets_step", int_name="get_det_offsets_step"),
     ParamMapEntry(ext_name="get_det_offsets_x_min", int_name="get_det_offsets_x_min"),
     ParamMapEntry(ext_name="get_det_offsets_x_max", int_name="get_det_offsets_x_max"),

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
     ParamMapEntry(ext_name="save_all",                  int_name="save_all"),
     ParamMapEntry(ext_name="save_gda",                  int_name="save_gda"),
     ParamMapEntry(ext_name="save_maud_calib",           int_name="save_maud_calib"),
     ParamMapEntry(ext_name="save_maud",                 int_name="save_maud"),
     ParamMapEntry(ext_name="spline_coefficient",        int_name="spline_coeff"),
     ParamMapEntry(ext_name="subtract_empty_instrument", int_name="subtract_empty_inst", optional =True),
     ParamMapEntry(ext_name="suffix",                    int_name="suffix",         optional=True),
     ParamMapEntry(ext_name="texture_mode",              int_name="texture_mode",         optional=True),
     ParamMapEntry(ext_name="output_directory",          int_name="output_dir"),
     ParamMapEntry(ext_name="unit_to_keep",              int_name="unit_to_keep",
                   enum_class=WORKSPACE_UNITS,           optional=True),
     ParamMapEntry(ext_name="user_name",                 int_name="user_name"),
     ParamMapEntry(ext_name="vanadium_cropping_values",  int_name="vanadium_cropping_values"),
     ParamMapEntry(ext_name="vanadium_normalisation",    int_name="do_van_norm")
    ]
