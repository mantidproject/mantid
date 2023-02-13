# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple

from enum import Enum

# ----------------------------------------------------------------------------------------------------------------------
#  Named tuples for passing around data in a structured way, a bit like a plain old c-struct.
# ----------------------------------------------------------------------------------------------------------------------
# General
range_entry = namedtuple("range_entry", "start, stop")
range_entry_with_detector = namedtuple("range_entry_with_detector", "start, stop, detector_type")
single_entry_with_detector = namedtuple("range_entry_with_detector", "entry, detector_type")

# Back
back_single_monitor_entry = namedtuple("back_single_monitor_entry", "monitor, start, stop")

# Limits
mask_angle_entry = namedtuple("mask_angle_entry", "min, max, use_mirror")
simple_range = namedtuple("simple_range", "start, stop, step, step_type")
q_xy_range = namedtuple("q_xy_range", "start, stop, step")
complex_range = namedtuple("complex_steps", "start, step1, mid, step2, stop, step_type1, step_type2")
rebin_string_values = namedtuple("rebin_string_values", "value")
event_binning_string_values = namedtuple("event_binning_string_values", "value")
q_rebin_values = namedtuple("q_rebin_values", "min, max, rebin_string")


# Mask
mask_line = namedtuple("mask_line", "width, angle, x, y")
mask_block = namedtuple("mask_block", "horizontal1, horizontal2, vertical1, vertical2, detector_type")
mask_block_cross = namedtuple("mask_block_cross", "horizontal, vertical, detector_type")

# Set
position_entry = namedtuple("position_entry", "pos1, pos2, detector_type")
set_scales_entry = namedtuple("set_scales_entry", "s, a, b, c, d")

# Fit
range_entry_fit = namedtuple("range_entry_fit", "start, stop, fit_type")
fit_general = namedtuple("fit_general", "start, stop, fit_type, data_type, polynomial_order")

# Mon
monitor_length = namedtuple("monitor_length", "length, spectrum, interpolate")
monitor_spectrum = namedtuple("monitor_spectrum", "spectrum, is_trans, interpolate")
monitor_file = namedtuple("monitor_file", "file_path, detector_type")

# Det
det_fit_range = namedtuple("det_fit_range", "start, stop, use_fit")


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


class FitId(Enum):
    CLEAR = "clear"
    GENERAL = "general"
    MONITOR_TIMES = "monitor_times"


class GravityId(Enum):
    EXTRA_LENGTH = "extra_length"
    ON_OFF = "on_off"


class MonId(Enum):
    DIRECT = "direct"
    FLAT = "flat"
    HAB = "hab"
    INTERPOLATE = "interpolate"
    LENGTH = "length"
    SPECTRUM = "spectrum"
    SPECTRUM_TRANS = "spectrum_trans"


class PrintId(Enum):
    PRINT_LINE = "print_line"


class BackId(Enum):
    ALL_MONITORS = "all_monitors"
    MONITOR_OFF = "monitor_off"
    SINGLE_MONITORS = "single_monitors"
    TRANS = "trans"


class OtherId(Enum):
    EVENT_SLICES = "event_slices"

    MERGE_MASK = "merge_mask"
    MERGE_MIN = "merge_min"
    MERGE_MAX = "merge_max"

    REDUCTION_DIMENSIONALITY = "reduction_dimensionality"

    SAVE_AS_ZERO_ERROR_FREE = "save_as_zero_error_free"
    SAVE_TYPES = "save_types"

    SAMPLE_HEIGHT = "sample_height"
    SAMPLE_WIDTH = "sample_width"
    SAMPLE_THICKNESS = "sample_thickness"
    SAMPLE_SHAPE = "sample_shape"

    USE_COMPATIBILITY_MODE = "use_compatibility_mode"
    USE_EVENT_SLICE_OPTIMISATION = "use_event_slice_optimisation"
    USE_FULL_WAVELENGTH_RANGE = "use_full_wavelength_range"
    USE_REDUCTION_MODE_AS_SUFFIX = "use_reduction_mode_as_suffix"

    USER_SPECIFIED_OUTPUT_NAME = "user_specified_output_name"
    USER_SPECIFIED_OUTPUT_NAME_SUFFIX = "user_specified_output_name_suffix"

    WAVELENGTH_RANGE = "wavelength_range"
