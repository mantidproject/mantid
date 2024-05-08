# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.routines.common import PARAM_MAPPING as COMMON_PARAM_MAPPING
from isis_powder.routines.common_enums import INPUT_BATCHING, EMPTY_CAN_SUBTRACTION_METHOD, VAN_NORMALISATION_METHOD
from isis_powder.polaris_routines.polaris_enums import POLARIS_CHOPPER_MODES

#                 Maps friendly user name (ext_name) -> script name (int_name)
attr_mapping = [
    ParamMapEntry(ext_name="calibration_directory", int_name="calibration_dir"),
    ParamMapEntry(ext_name="calibration_mapping_file", int_name="cal_mapping_path"),
    ParamMapEntry(ext_name="config_file", int_name="config_file"),
    ParamMapEntry(ext_name="debug", int_name="debug", optional=True),
    ParamMapEntry(ext_name="delta_r", int_name="delta_r", optional=True),
    ParamMapEntry(ext_name="delta_q", int_name="delta_q", optional=True),
    ParamMapEntry(ext_name="do_absorb_corrections", int_name="do_absorb_corrections"),
    ParamMapEntry(ext_name="do_van_normalisation", int_name="do_van_normalisation"),
    ParamMapEntry(
        ext_name="van_normalisation_method",
        int_name="van_normalisation_method",
        enum_class=VAN_NORMALISATION_METHOD,
        optional=True,
    ),
    ParamMapEntry(
        ext_name="empty_can_subtraction_method",
        int_name="empty_can_subtraction_method",
        enum_class=EMPTY_CAN_SUBTRACTION_METHOD,
        optional=True,
    ),
    ParamMapEntry(ext_name="file_ext", int_name="file_extension", optional=True),
    ParamMapEntry(ext_name="first_cycle_run_no", int_name="run_in_range"),
    ParamMapEntry(ext_name="focused_cropping_values", int_name="focused_cropping_values"),
    ParamMapEntry(ext_name="focused_bin_widths", int_name="focused_bin_widths"),
    ParamMapEntry(ext_name="freq_params", int_name="freq_params", optional=True),
    ParamMapEntry(ext_name="grouping_file_name", int_name="grouping_file_name"),
    ParamMapEntry(ext_name="input_mode", int_name="input_mode", enum_class=INPUT_BATCHING),
    ParamMapEntry(ext_name="lorch_filter", int_name="lorch_filter", optional=True),
    ParamMapEntry(ext_name="merge_banks", int_name="merge_banks", optional=True),
    ParamMapEntry(ext_name="mode", int_name="mode", enum_class=POLARIS_CHOPPER_MODES, optional=True),
    ParamMapEntry(ext_name="multiple_scattering", int_name="multiple_scattering", optional=True),
    ParamMapEntry(ext_name="paalman_pings_events_per_point", int_name="paalman_pings_events_per_point", optional=True),
    ParamMapEntry(ext_name="pdf_type", int_name="pdf_type"),
    ParamMapEntry(ext_name="per_detector_vanadium", int_name="per_detector_vanadium", optional=True),
    ParamMapEntry(ext_name="placzek_order", int_name="placzek_order", optional=True),
    ParamMapEntry(ext_name="q_lims", int_name="q_lims", optional=True),
    ParamMapEntry(ext_name="raw_data_cropping_values", int_name="raw_data_crop_values"),
    ParamMapEntry(ext_name="run_number", int_name="run_number"),
    ParamMapEntry(ext_name="sample_empty", int_name="sample_empty", optional=True),
    ParamMapEntry(ext_name="sample_empty_scale", int_name="sample_empty_scale"),
    ParamMapEntry(ext_name="sample_temp", int_name="sample_temp", optional=True),
    ParamMapEntry(ext_name="suffix", int_name="suffix", optional=True),
    ParamMapEntry(ext_name="spline_coefficient", int_name="spline_coeff"),
    ParamMapEntry(ext_name="spline_coeff_per_detector", int_name="spline_coeff_per_detector", optional=True),
    ParamMapEntry(ext_name="output_directory", int_name="output_dir"),
    ParamMapEntry(ext_name="user_name", int_name="user_name"),
    ParamMapEntry(ext_name="vanadium_cropping_values", int_name="van_crop_values"),
    ParamMapEntry(ext_name="vanadium_peaks_masking_file", int_name="masking_file_name"),
    ParamMapEntry(ext_name="keep_raw_workspace", int_name="keep_raw_workspace", optional=True),
    ParamMapEntry(ext_name="mayers_mult_scat_events", int_name="mayers_mult_scat_events", optional=True),
    ParamMapEntry(ext_name="pdf_output_name", int_name="pdf_output_name", optional=True),
]
attr_mapping.extend(COMMON_PARAM_MAPPING)
