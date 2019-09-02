# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from collections import namedtuple
from sans.common.enums import serializable_enum


# ----------------------------------------------------------------------------------------------------------------------
#  Named tuples for passing around data in a structured way, a bit like a plain old c-struct.
# ----------------------------------------------------------------------------------------------------------------------
# General
range_entry = namedtuple('range_entry', 'start, stop')
range_entry_with_detector = namedtuple('range_entry_with_detector', 'start, stop, detector_type')
single_entry_with_detector = namedtuple('range_entry_with_detector', 'entry, detector_type')

# Back
back_single_monitor_entry = namedtuple('back_single_monitor_entry', 'monitor, start, stop')

# Limits
mask_angle_entry = namedtuple('mask_angle_entry', 'min, max, use_mirror')
simple_range = namedtuple('simple_range', 'start, stop, step, step_type')
complex_range = namedtuple('complex_steps', 'start, step1, mid, step2, stop, step_type1, step_type2')
rebin_string_values = namedtuple('rebin_string_values', 'value')
event_binning_string_values = namedtuple('event_binning_string_values', 'value')
q_rebin_values = namedtuple('q_rebin_values', 'min, max, rebin_string')


# Mask
mask_line = namedtuple('mask_line', 'width, angle, x, y')
mask_block = namedtuple('mask_block', 'horizontal1, horizontal2, vertical1, vertical2, detector_type')
mask_block_cross = namedtuple('mask_block_cross', 'horizontal, vertical, detector_type')

# Set
position_entry = namedtuple('position_entry', 'pos1, pos2, detector_type')
set_scales_entry = namedtuple('set_scales_entry', 's, a, b, c, d')

# Fit
range_entry_fit = namedtuple('range_entry_fit', 'start, stop, fit_type')
fit_general = namedtuple('fit_general', 'start, stop, fit_type, data_type, polynomial_order')

# Mon
monitor_length = namedtuple('monitor_length', 'length, spectrum, interpolate')
monitor_spectrum = namedtuple('monitor_spectrum', 'spectrum, is_trans, interpolate')
monitor_file = namedtuple('monitor_file', 'file_path, detector_type')

# Det
det_fit_range = namedtuple('det_fit_range', 'start, stop, use_fit')


# ------------------------------------------------------------------
# --- State director keys ------------------------------------------
# ------------------------------------------------------------------


# --- DET
@serializable_enum("reduction_mode", "rescale", "shift", "rescale_fit", "shift_fit", "correction_x", "correction_y",
                   "correction_z", "correction_rotation", "correction_radius", "correction_translation",
                   "correction_x_tilt", "correction_y_tilt", "merge_range", "instrument")
class DetectorId(object):
    pass


# --- LIMITS
@serializable_enum("angle", "events_binning", "events_binning_range", "radius_cut", "wavelength_cut", "radius", "q",
                   "qxy", "wavelength")
class LimitsId(object):
    pass


# --- MASK
@serializable_enum("line", "time", "time_detector", "clear_detector_mask", "clear_time_mask", "single_spectrum_mask",
                   "spectrum_range_mask", "vertical_single_strip_mask", "vertical_range_strip_mask", "file",
                   "horizontal_single_strip_mask", "horizontal_range_strip_mask", "block", "block_cross")
class MaskId(object):
    pass


# --- SAMPLE
@serializable_enum("path", "offset")
class SampleId(object):
    pass


# --- SET
@serializable_enum("scales", "centre", "centre_HAB")
class SetId(object):
    pass


# --- TRANS
@serializable_enum("spec", "spec_shift", "radius", "roi", "mask", "sample_workspace", "can_workspace")
class TransId(object):
    pass


# --- TUBECALIBFILE
@serializable_enum("file")
class TubeCalibrationFileId(object):
    pass


# -- QRESOLUTION
@serializable_enum("on", "delta_r", "collimation_length", "a1", "a2", "h1", "w1", "h2", "w2", "moderator")
class QResolutionId(object):
    pass


# --- FIT
@serializable_enum("clear", "monitor_times", "general")
class FitId(object):
    pass


# --- GRAVITY
@serializable_enum("on_off", "extra_length")
class GravityId(object):
    pass


# --- MON
@serializable_enum("length", "direct", "flat", "hab", "spectrum", "spectrum_trans", "interpolate")
class MonId(object):
    pass


# --- PRINT
@serializable_enum("print_line")
class PrintId(object):
    pass


# -- BACK
@serializable_enum("all_monitors", "single_monitors", "monitor_off", "trans")
class BackId(object):
    pass


# -- OTHER - not settable in user file
@serializable_enum("reduction_dimensionality", "use_full_wavelength_range", "event_slices",
                   "use_compatibility_mode", "save_types", "save_as_zero_error_free", "user_specified_output_name",
                   "user_specified_output_name_suffix", "use_reduction_mode_as_suffix", "sample_width", "sample_height",
                   "sample_thickness", "sample_shape", "merge_mask", "merge_min", "merge_max", "wavelength_range",
                   "use_event_slice_optimisation")
class OtherId(object):
    pass
