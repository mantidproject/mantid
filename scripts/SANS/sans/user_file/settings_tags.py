# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from collections import namedtuple

from mantid.py3compat import Enum
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


class DetectorId(Enum):
    CORRECTION_X = "correction_x"
    CORRECTION_X_TILT = "correction_x_tilt"
    CORRECTION_Y = "correction_y"
    CORRECTION_Y_TILT = "correction_y_tilt"
    CORRECTION_Z = "correction_z"
    CORRECTION_RADIUS = "correction_radius"
    CORRECTION_ROTATION = "correction_rotation"
    CORRECTION_TRANSLATION = "correction_translation"
    MERGE_RANGE = "merge_range"
    INSTRUMENT = "instrument"
    REDUCTION_MODE = "reduction_mode"
    RESCALE = "rescale"
    RESCALE_FIT = "rescale_fit"
    SHIFT = "shift"
    SHIFT_FIT = "shift_fit"


class LimitsId(Enum):
    ANGLE = "angle"
    EVENTS_BINNING = "events_binning"
    EVENTS_BINNING_RANGE = "events_binning_range"
    RADIUS = "radius"
    RADIUS_CUT = "radius_cut"
    Q = "q"
    QXY = "qxy"
    WAVELENGTH = "wavelength"
    WAVELENGTH_CUT = "wavelength_cut"


class MaskId(Enum):
    BLOCK = "block"
    BLOCK_CROSS = "block_cross"
    CLEAR_DETECTOR_MASK = "clear_detector_mask"
    CLEAR_TIME_MASK = "clear_time_mask"
    FILE = "file"
    LINE = "line"
    HORIZONTAL_SINGLE_STRIP_MASK = "horizontal_single_strip_mask"
    HORIZONTAL_RANGE_STRIP_MASK = "horizontal_range_strip_mask"
    TIME = "time"
    TIME_DETECTOR = "time_detector"
    SINGLE_SPECTRUM_MASK = "single_spectrum_mask"
    SPECTRUM_RANGE_MASK = "spectrum_range_mask"
    VERTICAL_SINGLE_STRIP_MASK = "vertical_single_strip_mask"
    VERTICAL_RANGE_STRIP_MASK = "vertical_range_strip_mask"


class SampleId(Enum):
    PATH = "path"
    OFFSET = "offset"


class SetId(Enum):
    CENTRE = "centre"
    CENTRE_HAB = "centre_HAB"
    SCALES = "scales"


class TransId(Enum):
    CAN_WORKSPACE = "can_workspace"
    RADIUS = "radius"
    ROI = "roi"
    MASK = "mask"
    SAMPLE_WORKSPACE = "sample_workspace"
    SPEC = "spec"
    SPEC_4_SHIFT = "spec_4_shift"
    SPEC_5_SHIFT = "spec_5_shift"


class TubeCalibrationFileId(Enum):
    FILE = "file"


class QResolutionId(Enum):
    A1 = "a1"
    A2 = "a2"
    COLLIMATION_LENGTH = "collimation_length"
    DELTA_R = "delta_r"
    H1 = "h1"
    H2 = "h2"
    MODERATOR = "moderator"
    ON = "on"
    W1 = "w1"
    W2 = "w2"


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
