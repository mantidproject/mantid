from collections import namedtuple

# General
range_entry = namedtuple('range_entry', 'start, stop')
range_entry_with_detector = namedtuple('range_entry_with_detector', 'start, stop, detector_type')
single_entry_with_detector = namedtuple('range_entry_with_detector', 'entry, detector_type')

# Back
back_single_monitor_entry = namedtuple('back_single_monitor_entry', 'monitor, start, stop')

# Limits
mask_angle_entry = namedtuple('mask_angle_entry', 'min, max, is_no_mirror')
simple_range = namedtuple('simple_range', 'start, stop, step, step_type')
complex_range = namedtuple('complex_steps', 'start, step1, mid, step2, stop, step_type')
rebin_string_values = namedtuple('rebin_string_values', 'rebin_values')

# Mask
mask_line = namedtuple('mask_line', 'width, angle, x, y')
mask_block = namedtuple('mask_block', 'horizontal1, horizontal2, vertical1, vertical2, detector_type')
mask_block_cross = namedtuple('mask_block_cross', 'horizontal, vertical, detector_type')

# Set
position_entry = namedtuple('position_entry', 'pos1, pos2, detector_type')
set_scales_entry = namedtuple('set_scales_entry', 's, a, b, c, d')

# Fit
range_entry_fit = namedtuple('range_entry_fit', 'start, stop, fit_type')

# Mon
monitor_length = namedtuple('monitor_length', 'length, spectrum, interpolate')
monitor_spectrum = namedtuple('monitor_spectrum', 'spectrum, is_trans, interpolate')
monitor_file = namedtuple('monitor_file', 'file_path, detector_type')


# -----------------------------------------------------------------
# --- User File keywords ------------------------------------------
# -----------------------------------------------------------------

# --- DET
user_file_det_reduction_mode = "det_reduction_mode"
user_file_det_rescale = "det_rescale"
user_file_det_shift = "det_shift"

user_file_det_rescale_fit = "det_rescale_fit"
user_file_det_shift_fit = "det_shift_fit"

user_file_det_correction_x = "det_correction_x"
user_file_det_correction_y = "det_correction_y"
user_file_det_correction_z = "det_correction_z"
user_file_det_correction_rotation = "det_correction_rotation"
user_file_det_correction_radius = "det_correction_radius"
user_file_det_correction_translation = "det_correction_translation"
user_file_det_correction_x_tilt = "det_correction_x_tilt"
user_file_det_correction_y_tilt = "det_correction_y_tilt"

# --- LIMITS
user_file_limits_angle = "limits_angle"

user_file_limits_events_binning = "limits_events_binning"
user_file_limits_events_binning_range = "limits_events_binning_range "

user_file_limits_radius_cut = "limits_radius_cut"
user_file_limits_wavelength_cut = "limits_wavelength_cut"

user_file_limits_radius = "limits_radius"

user_file_limits_q = "limits_q"
user_file_limits_qxy = "limits_qxy"
user_file_limits_wavelength = "limits_wavelength"


# --- MASK
user_file_mask_line = "mask_line"

user_file_mask_time = "mask_time"
user_file_mask_time_detector = "mask_time_detector"

user_file_mask_clear_detector_mask = "mask_clear_detector_mask"
user_file_mask_clear_time_mask = "mask_clear_time_mask"

user_file_mask_single_spectrum_mask = "mask_single_spectrum_mask"
user_file_mask_spectrum_range_mask = "mask_spectrum_range_mask"

user_file_mask_vertical_single_strip_mask = "mask_vertical_single_strip_mask"
user_file_mask_vertical_range_strip_mask = "mask_vertical_range_strip_mask"

user_file_mask_horizontal_single_strip_mask = "mask_horizontal_single_strip_mask"
user_file_mask_horizontal_range_strip_mask = "mask_horizontal_range_strip_mask"

user_file_mask_block = "mask_block"
user_file_mask_block_cross = "block_cross"


# --- SAMPLE
user_file_sample_path = "sample_path"
user_file_sample_offset = "sample_offset"


# --- SET
user_file_set_scales = "set_scales"
user_file_set_centre = "set_centre"


# --- TRANS
user_file_trans_spec = "trans_spec"
user_file_trans_spec_shift = "trans_spec_shift"

user_file_trans_radius = "trans_radius"
user_file_trans_roi = "trans_roi"
user_file_trans_mask = "trans_mask"

user_file_trans_sample_workspace = "trans_sample_workspace"
user_file_trans_can_workspace = "trans_can_workspace"


# --- TUBECALIBFILE
user_file_tube_calibration_file = "tube_calibration_file"


# -- QRESOLUTION
user_file_q_resolution_on = "q_resolution_on"
user_file_q_resolution_delta_r = "q_resolution_delta_r"
user_file_q_resolution_collimation_length = "q_resolution_collimation_length"
user_file_q_resolution_a1 = "q_resolution_a1"
user_file_q_resolution_a2 = "q_resolution_a2"
user_file_q_resolution_h1 = "q_resolution_h1"
user_file_q_resolution_w1 = "q_resolution_w1"
user_file_q_resolution_h2 = "q_resolution_h2"
user_file_q_resolution_w2 = "q_resolution_w2"
user_file_q_resolution_moderator = "q_resolution_moderator"


# --- FIT
user_file_fit_clear = "fit_clear"

user_file_fit_range = "fit_range"
user_file_fit_monitor_times = "fit_monitor_times"

user_file_fit_can = "fit_can"
user_file_fit_sample = "fit_sample"
user_file_fit_can_poly = "fit_can_poly"
user_file_fit_sample_poly = "fit_sample_poly"
user_file_fit_lin = "LIN"
user_file_fit_log = "LOG"
user_file_fit_poly = "POLY"


# --- GRAVITY
user_file_gravity_on_off = "gravity_on_off"
user_file_gravity_extra_length = "gravity_extra_length"


# --- MASKFILE
user_file_mask_file = "mask_file"


# --- MON
user_file_mon_length = "mon_length"

user_file_mon_direct = "mon_direct"
user_file_mon_flat = "mon_flat"
user_file_mon_hab = "mon_hab"

user_file_mon_spectrum = "mon_spectrum"
user_file_mon_spectrum_trans = "mon_spectrum_trans"
user_file_mon_interpolate = "mon_interpolate"


# --- PRINT
user_file_print = "user_file_print"


# -- BACK
user_file_back_all_monitors = "back_all_monitors"
user_file_back_single_monitors = "back_single_monitor"
user_file_back_monitor_off = "back_monitor_off"
