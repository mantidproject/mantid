# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_powder.hrpd_routines.hrpd_enums import HRPD_MODES, HRPD_TOF_WINDOWS
from isis_powder.routines.common import PARAM_MAPPING as COMMON_PARAM_MAPPING
from isis_powder.routines.common_enums import EMPTY_CAN_SUBTRACTION_METHOD
from isis_powder.routines.param_map_entry import ParamMapEntry

attr_mapping = [
    ParamMapEntry(ext_name="calibration_directory", int_name="calibration_dir"),
    ParamMapEntry(ext_name="calibration_mapping_file", int_name="cal_mapping_path"),
    ParamMapEntry(ext_name="config_file", int_name="config_file_name"),
    ParamMapEntry(ext_name="do_absorb_corrections", int_name="do_absorb_corrections"),
    ParamMapEntry(
        ext_name="empty_can_subtraction_method",
        int_name="empty_can_subtraction_method",
        enum_class=EMPTY_CAN_SUBTRACTION_METHOD,
        optional=True,
    ),
    ParamMapEntry(ext_name="eff_integration_range", int_name="eff_integration_range", optional=True),
    ParamMapEntry(ext_name="file_ext", int_name="file_extension", optional=True),
    ParamMapEntry(ext_name="focused_bin_widths", int_name="focused_bin_widths"),
    ParamMapEntry(ext_name="focused_cropping_values", int_name="tof_cropping_values"),
    ParamMapEntry(ext_name="grouping_file_name", int_name="grouping_file_name"),
    ParamMapEntry(ext_name="output_directory", int_name="output_dir"),
    ParamMapEntry(ext_name="spline_coefficient", int_name="spline_coeff"),
    ParamMapEntry(ext_name="spline_coeff_per_detector", int_name="spline_coeff_per_detector", optional=True),
    ParamMapEntry(ext_name="first_cycle_run_no", int_name="run_in_range"),
    ParamMapEntry(ext_name="mode", int_name="mode", enum_class=HRPD_MODES),
    ParamMapEntry(ext_name="multiple_scattering", int_name="multiple_scattering"),
    ParamMapEntry(ext_name="per_detector_vanadium", int_name="per_detector_vanadium", optional=True),
    ParamMapEntry(ext_name="run_number", int_name="run_number"),
    ParamMapEntry(ext_name="sample_empty", int_name="sample_empty", optional=True),
    ParamMapEntry(ext_name="sample_empty_scale", int_name="sample_empty_scale"),
    ParamMapEntry(ext_name="subtract_empty_instrument", int_name="subtract_empty_inst", optional=True),
    ParamMapEntry(ext_name="do_solid_angle_corrections", int_name="do_solid_angle", optional=True),
    ParamMapEntry(ext_name="suffix", int_name="suffix", optional=True),
    ParamMapEntry(ext_name="user_name", int_name="user_name"),
    ParamMapEntry(ext_name="vanadium_normalisation", int_name="do_van_norm"),
    ParamMapEntry(ext_name="vanadium_tof_cropping", int_name="van_tof_cropping"),
    ParamMapEntry(ext_name="vanadium_peaks_masking_file", int_name="masking_file_name"),
    ParamMapEntry(ext_name="window", int_name="tof_window", enum_class=HRPD_TOF_WINDOWS),
    ParamMapEntry(ext_name="keep_raw_workspace", int_name="keep_raw_workspace", optional=True),
    ParamMapEntry(ext_name="mayers_mult_scat_events", int_name="mayers_mult_scat_events", optional=True),
    ParamMapEntry(ext_name="fit_prompt_pulse", int_name="fit_prompt_pulse", optional=True),
]
attr_mapping.extend(COMMON_PARAM_MAPPING)
