# pylint: disable=too-many-lines, invalid-name, too-many-instance-attributes, too-many-branches, too-few-public-methods

import abc
import re


# -----------------------------------------------------------------
# --- Free Fuctions     ------------------------------------------
# -----------------------------------------------------------------
def convert_string_to_float(to_convert):
    return float(to_convert.strip())


def convert_string_to_integer(to_convert):
    return int(to_convert.strip())


def extract_range(to_extract, converter):
    # Remove leading and trailing whitespace
    to_extract = to_extract.strip()
    # Collapse multiple central whitespaces to a single one
    to_extract = ' '.join(to_extract.split())

    entries_string = to_extract.split()
    number_of_entries = len(entries_string)
    if number_of_entries != 2:
        raise RuntimeError("Expected a range defined by two numbers,"
                           " but instead received {0}".format(number_of_entries))

    return [converter(entries_string[0]),
            converter(entries_string[1])]


def extract_float_range(to_extract):
    return extract_range(to_extract, convert_string_to_float)


def extract_int_range(to_extract):
    return extract_range(to_extract, convert_string_to_integer)


def extract_list(to_extract, separator, converter):
    to_extract = to_extract.strip()
    to_extract = ' '.join(to_extract.split())
    string_list = [element.replace(" ", "") for element in to_extract.split(separator)]
    return [converter(element) for element in string_list]


def extract_float_list(to_extract, separator=","):
    return extract_list(to_extract, separator, convert_string_to_float)


def extract_string_list(to_extract, separator=","):
    return extract_list(to_extract, separator, lambda x: x)


def extract_float_range_midpoint_and_steps(to_extract):
    to_extract = ' '.join(to_extract.split())

    entries_string = to_extract.split()
    number_of_entries = len(entries_string)
    if number_of_entries != 5:
        raise RuntimeError("Expected a range defined by 5 numbers,"
                           " but instead received {0}".format(number_of_entries))

    return [convert_string_to_float(entries_string[0]),
            convert_string_to_float(entries_string[1]),
            convert_string_to_float(entries_string[2]),
            convert_string_to_float(entries_string[3]),
            convert_string_to_float(entries_string[4])]


def does_pattern_match(compiled_regex, line):
    return compiled_regex.match(line) is not None


def escape_special_characters_for_file_path(to_escape):
    escape = {"\a": "\\a", "\b": "\\b", r"\c": "\\c", "\f": "\\f",
              "\n": "\\n", "\r": "\\r", "\t": "\\t", "\v": "\\v"}
    keys = escape.keys()
    escaped = to_escape
    for key in keys:
        escaped = escaped.replace(key, escape[key])
    escaped = escaped.replace("\\", "/")
    return escaped


# -----------------------------------------------------------------
# --- Common Regex Strings-----------------------------------------
# -----------------------------------------------------------------
float_number = "[-+]?(\\d*[.])?\\d+"
integer_number = "[-+]?\\d+"
positive_float_number = "[+]?(\\d*[.])?\\d+"
start_string = "^\\s*"
end_string = "\\s*$"
space_string = "\\s+"
rebin_string = "(\\s*[-+]?\\d+(\\.\\d+)?)(\\s*,\\s*[-+]?\\d+(\\.\\d+)?)*"

# -----------------------------------------------------------------
# --- User File keywords ------------------------------------------
# -----------------------------------------------------------------

user_file_hab_suffix = "_hab"
user_file_lab_suffix = "_lab"

# --- DET
user_file_reduction_mode = "reduction_mode"
user_file_rescale = "rescale_hab"
user_file_shift = "shift_hab"

user_file_rescale_shift_fit_min = "merge_q_min"
user_file_rescale_shift_fit_max = "merge_q_max"

correction_x = "detector_x_encoder"
user_file_correction_x_hab = correction_x + user_file_hab_suffix
user_file_correction_x_lab = correction_x + user_file_lab_suffix

correction_y = "detector_y_encoder"
user_file_correction_y_hab = correction_y + user_file_hab_suffix
user_file_correction_y_lab = correction_y + user_file_lab_suffix

correction_z = "detector_z_encoder"
user_file_correction_z_hab = correction_z + user_file_hab_suffix
user_file_correction_z_lab = correction_z + user_file_lab_suffix

correction_rotation = "detector_rotation_encoder"
user_file_correction_rotation_hab = correction_rotation + user_file_hab_suffix
user_file_correction_rotation_lab = correction_rotation + user_file_lab_suffix

correction_radius = "detector_radius_encoder"
user_file_correction_radius_hab = correction_radius + user_file_hab_suffix
user_file_correction_radius_lab = correction_radius + user_file_lab_suffix

correction_translation = "detector_translation_encoder"
user_file_correction_translation_hab = correction_translation + user_file_hab_suffix
user_file_correction_translation_lab = correction_translation + user_file_lab_suffix

# --- LIMITS
user_file_limit_angle_min = "limit_angle_min"
user_file_limit_angle_max = "limit_angle_max"
user_file_no_mirror = "limit_angle_no_mirror"

user_file_events_binning = "events_binning"

user_file_radius_cut = "radius_cut"
user_file_wavelength_cut = "wavelength_cut"

user_file_radius_limit_min = "radius_limit_min"
user_file_radius_limit_max = "radius_limit_max"

user_file_q_min = "q_min"
user_file_q_step = "q_step"
user_file_q_step_type = "q_step_type"
user_file_q_max = "q_max"

user_file_q_mid = "q_mid"
user_file_q_step_min_mid = "q_step_min_mid"
user_file_q_step_mid_max = "q_step_mid_max"

user_file_qxy_min = "qxy_min"
user_file_qxy_step = "qxy_step"
user_file_qxy_step_type = "qxy_step_type"
user_file_qxy_max = "qxy_max"

user_file_qxy_mid = "qxy_mid"
user_file_qxy_step_min_mid = "qxy_step_min_mid"
user_file_qxy_step_mid_max = "qxy_step_mid_max"

user_file_wavelength_min = "wavelength_min"
user_file_wavelength_step = "wavelength_step"
user_file_wavelength_step_type = "wavelength_step_type"
user_file_wavelength_max = "wavelength_max"

user_file_wavelength_mid = "wavelength_mid"
user_file_wavelength_step_min_mid = "wavelength_step_min_mid"
user_file_wavelength_step_mid_max = "wavelength_step_mid_max"

# --- MASK
user_file_line_width = "line_width"
user_file_line_angle = "line_angle"
user_file_line_x = "line_x"
user_file_line_y = "line_y"

user_file_time_min = "mask_time_min"
user_file_time_max = "mask_time_max"

user_file_clear_detector_mask = "clear_detector_mask"
user_file_clear_time_mask = "clear_time_mask"

use_file_single_spectrum_mask = "single_spectrum_mask"
use_file_spectrum_range_mask = "spectrum_range_mask"

user_file_vertical_single_strip_mask_hab = "single_vertical_strip_mask_hab"
user_file_vertical_range_strip_mask_hab = "range_vertical_strip_mask_hab"
user_file_vertical_single_strip_mask_lab = "single_vertical_strip_mask_lab"
user_file_vertical_range_strip_mask_lab = "range_vertical_strip_mask_lab"

user_file_horizontal_single_strip_mask_hab = "single_horizontal_strip_mask_hab"
user_file_horizontal_range_strip_mask_hab = "range_horizontal_strip_mask_hab"
user_file_horizontal_single_strip_mask_lab = "single_horizontal_strip_mask_lab"
user_file_horizontal_range_strip_mask_lab = "range_horizontal_strip_mask_lab"

# --- SAMPLE
user_file_sample_path = "sample_path"
user_file_sample_offset = "sample_offset"

# --- SET
user_file_scales = "set_scales"
user_file_centre_lab = "centre_lab"
user_file_centre_hab = "centre_hab"

# --- TRANS
user_file_trans_spec = "trans_spec"
user_file_trans_spec_shift = "trans_spec_shift"

user_file_trans_radius = "trans_radius"
user_file_trans_roi = "trans_roi"
user_file_trans_mask = "trans_mask"

user_file_trans_sample_workspace = "trans_sample_workspace"
user_file_trans_can_workspace = "trans_can_workspace"

# --- TUBECALIBFILE
user_file_tube_calibration = "tube_calibration_file"

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
user_file_range_based_fit_lin = "range_based_fit_lin"
user_file_range_based_fit_log = "range_based_fit_log"
user_file_monitor_times = "monitor_times"

user_file_fit_can = "fit_can"
user_file_fit_sample = "fit_sample"
user_file_fit_can_poly = "fit_can_poly"
user_file_fit_sample_poly = "fit_sample_poly"
user_file_lin = "LIN"
user_file_log = "LOG"
user_file_poly = "POLY"

# --- GRAVITY
user_file_gravity_on = "gravity_on_off"
user_file_gravity_extra_length = "gravity_extra_length"

# --- MASKFILE
user_file_mask_file = "mask_file"

# --- MON
user_file_monitor_length = "monitor_length"
user_file_direct_lab = "direct_file_lab"
user_file_direct_hab = "direct_file_hab"
user_file_flat_lab = "flat_file_lab"
user_file_flat_hab = "flat_file_hab"
user_file_mon_hab = "hab_file"
user_file_mon_spectrum = "mon_spectrum"
user_file_mon_spectrum_trans = "mon_spectrum_trans"
user_file_mon_interpolate = "mon_interpolate"

# --- PRINT
user_file_print = "user_file_print"

# -- BACK
user_file_all_monitors = "back_all_monitors"
user_file_single_monitors = "back_single_monitor"
user_file_monitor_off = "back_off"


# ----------------------------------------------------------------
# --- Parsers ----------------------------------------------------
# ----------------------------------------------------------------
class UserFileComponentParser(object):
    separator_dash = "/"
    separator_space = "\\s"
    separator_equal = "="

    @abc.abstractmethod
    def parse_line(self, line):
        pass

    @staticmethod
    @abc.abstractmethod
    def get_type():
        pass

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        pass

    @staticmethod
    def get_settings(line, command_pattern):
        line = line.strip()
        line = line.upper()
        setting = re.sub(command_pattern, "", line)
        setting = setting.strip()
        return setting.upper()


class BackParser(UserFileComponentParser):
    """
    The BackParser handles the following structure
        Command | Qualifer    | Parameter
        BACK    / MON/TIMES     t1 t2
        BACK    / M m/TIMES      t1 t2
        BACK    / M m            t1 t2
        BACK    / M m/OFF
    """
    Type = "BACK"

    def __init__(self):
        super(BackParser, self).__init__()

        # General
        self._times = "\\s*/\\s*TIMES"

        # All Monitors
        self._all_mons = "\\s*MON\\s*/\\s*TIMES\\s*"
        self._all_mons_pattern = re.compile(start_string + self._all_mons + space_string + float_number +
                                            space_string + float_number + end_string)

        # Single Monitor
        self._mon_id = "M"
        self._single_monitor = "\\s*" + self._mon_id + integer_number + "\\s*"
        self._single_monitor_pattern = re.compile(start_string + self._single_monitor +
                                                  "(\\s*" + self._times + "\\s*)?" + space_string + float_number +
                                                  space_string + float_number + end_string)

        # Off
        self._off_pattern = re.compile(start_string + self._single_monitor + "\\s*/\\s*OFF\\s*" + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, BackParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_all_mon(setting):
            output = self._extract_all_mon(setting)
        elif self._is_single_mon(setting):
            output = self._extract_single_mon(setting)
        elif self._is_off(setting):
            output = self._extract_off(setting)
        else:
            raise RuntimeError("BackParser: Unknown command for BACK: {0}".format(line))
        return output

    def _is_all_mon(self, line):
        return does_pattern_match(self._all_mons_pattern, line)

    def _is_single_mon(self, line):
        return does_pattern_match(self._single_monitor_pattern, line)

    def _is_off(self, line):
        return does_pattern_match(self._off_pattern, line)

    def _extract_all_mon(self, line):
        all_mons_string = re.sub(self._all_mons, "", line)
        time_range = extract_float_range(all_mons_string)
        return {user_file_all_monitors: time_range}

    def _extract_single_mon(self, line):
        monitor_number = self._get_monitor_number(line)
        single_string = re.sub(self._times, "", line)
        all_mons_string = re.sub(self._single_monitor, "", single_string)
        time_range = extract_float_range(all_mons_string)
        return {user_file_single_monitors: [monitor_number, time_range[0], time_range[1]]}

    def _extract_off(self, line):
        monitor_number = self._get_monitor_number(line)
        return {user_file_monitor_off: monitor_number}

    def _get_monitor_number(self, line):
        monitor_selection = re.search(self._single_monitor, line).group(0)
        monitor_selection = monitor_selection.strip()
        monitor_number_string = re.sub(self._mon_id, "", monitor_selection)
        return convert_string_to_float(monitor_number_string)

    @staticmethod
    def get_type():
        return BackParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + BackParser.get_type() + "\\s*/\\s*"


class DetParser(UserFileComponentParser):
    """
    The DetParser handles the following structure
        1) Corrections:
            DET/CORR/FRONT/qualifier [parameter]
            DET/CORR/REAR/qualifier [parameter]
              qualifiers are:
              X , Y, Z, ROT, RADIUS, SIDE

        2) Reduction Mode
            DET/FRONT
            DET/REAR
            DET/BOTH
            DET/MERGED
            DET/MERGE
            DET/MAIN
            DET/HAB

        3) Settings for merged operation
            DET/RESCALE rescale
            DET/SHIFT shift
            DET/RESCALE/FIT [Q1 Q2]
            DET/SHIFT/FIT [Q1 Q2]
    """
    Type = "DET"

    def __init__(self):
        super(DetParser, self).__init__()
        # Reduction mode
        self._HAB = ["FRONT", "HAB"]
        self._LAB = ["REAR", "MAIN"]
        self._BOTH = ["BOTH"]
        self._MERGE = ["MERGE", "MERGED"]
        self._reduction_mode = []
        self._reduction_mode.extend(self._BOTH)
        self._reduction_mode.extend(self._LAB)
        self._reduction_mode.extend(self._HAB)
        self._reduction_mode.extend(self._MERGE)

        # Corrections
        self._x = "\\s*X\\s*"
        self._x_pattern = re.compile(start_string + self._x + space_string + float_number + end_string)
        self._y = "\\s*Y\\s*"
        self._y_pattern = re.compile(start_string + self._y + space_string + float_number + end_string)
        self._z = "\\s*Z\\s*"
        self._z_pattern = re.compile(start_string + self._z + space_string + float_number + end_string)
        self._rotation = "\\s*ROT\\s*"
        self._rotation_pattern = re.compile(start_string + self._rotation + space_string + float_number + end_string)
        self._translation = "\\s*SIDE\\s*"
        self._translation_pattern = re.compile(start_string + self._translation + space_string +
                                               float_number + end_string)
        self._radius = "\\s*RADIUS\\s*"
        self._radius_pattern = re.compile(start_string + self._radius + space_string + float_number + end_string)
        self._correction_lab = "\\s*CORR\\s*/\\s*REAR\\s*/\\s*"
        self._correction_hab = "\\s*CORR\\s*/\\s*FRONT\\s*/\\s*"
        self._correction_LAB_pattern = re.compile(start_string + self._correction_lab)
        self._correction_HAB_pattern = re.compile(start_string + self._correction_hab)

        # Merge options
        self._rescale = "\\s*RESCALE\\s*"
        self._rescale_pattern = re.compile(start_string + self._rescale + space_string + float_number + end_string)
        self._shift = "\\s*SHIFT\\s*"
        self._shift_pattern = re.compile(start_string + self._shift + space_string + float_number + end_string)
        self._rescale_fit = "\\s*RESCALE\\s*/\\s*FIT\\s*"
        self._rescale_fit_pattern = re.compile(start_string + self._rescale_fit + space_string +
                                               float_number + space_string +
                                               float_number + end_string)
        self._shift_fit = "\\s*SHIFT\\s*/\\s*FIT\\s*"
        self._shift_fit_pattern = re.compile(start_string + self._shift_fit + space_string +
                                             float_number + space_string +
                                             float_number + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, DetParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_reduction_mode_setting(setting):
            output = self._extract_reduction_mode(setting)
        elif self._is_correction_setting(setting):
            output = self._extract_correction(setting)
        elif self._is_merge_option_setting(setting):
            output = self._extract_merge_option(setting)
        else:
            raise RuntimeError("DetParser: Unknown command for DET: {0}".format(line))
        return output

    def _is_reduction_mode_setting(self, line):
        front_element = line.split(UserFileComponentParser.separator_dash, 1)[0]
        return front_element in self._reduction_mode

    def _is_correction_setting(self, line):
        return does_pattern_match(self._correction_HAB_pattern, line) or \
               does_pattern_match(self._correction_LAB_pattern, line)

    def _is_merge_option_setting(self, line):
        return does_pattern_match(self._rescale_pattern, line) or \
               does_pattern_match(self._shift_pattern, line) or \
               does_pattern_match(self._rescale_fit_pattern, line) or \
               does_pattern_match(self._shift_fit_pattern, line)

    def _extract_reduction_mode(self, line):
        line_capital = line.upper()
        if line_capital in self._HAB:
            return {user_file_reduction_mode: "HAB"}
        elif line_capital in self._LAB:
            return {user_file_reduction_mode: "LAB"}
        elif line_capital in self._BOTH:
            return {user_file_reduction_mode: "BOTH"}
        elif line_capital in self._MERGE:
            return {user_file_reduction_mode: "MERGE"}
        else:
            raise RuntimeError("DetParser:  Could not extract line: {0}".format(line))

    def _extract_correction(self, line):
        if self._correction_HAB_pattern.match(line) is not None:
            qualifier = re.sub(self._correction_hab, "", line)
            qualifier = qualifier.strip()
            return self._extract_detector_setting(qualifier, user_file_hab_suffix)
        elif self._correction_LAB_pattern.match(line) is not None:
            qualifier = re.sub(self._correction_lab, "", line)
            qualifier = qualifier.strip()
            return self._extract_detector_setting(qualifier, user_file_lab_suffix)
        else:
            raise RuntimeError("DetParser: Could not extract line: {0}".format(line))

    def _extract_detector_setting(self, qualifier, detector_suffix):
        if self._x_pattern.match(qualifier):
            value_string = re.sub(self._x, "", qualifier)
            key = correction_x
        elif self._y_pattern.match(qualifier):
            value_string = re.sub(self._y, "", qualifier)
            key = correction_y
        elif self._z_pattern.match(qualifier):
            value_string = re.sub(self._z, "", qualifier)
            key = correction_z
        elif self._rotation_pattern.match(qualifier):
            value_string = re.sub(self._rotation, "", qualifier)
            key = correction_rotation
        elif self._translation_pattern.match(qualifier):
            value_string = re.sub(self._translation, "", qualifier)
            key = correction_translation
        elif self._radius_pattern.match(qualifier):
            value_string = re.sub(self._radius, "", qualifier)
            key = correction_radius
        else:
            raise RuntimeError("DetParser: Unknown qualifier encountered: {0}".format(qualifier))

        # Qualify the key with the selected detector
        key = key + detector_suffix
        value_string = value_string.strip()
        value = convert_string_to_float(value_string)
        return {key: value}

    def _extract_merge_option(self, line):
        if self._rescale_pattern.match(line) is not None:
            rescale_string = re.sub(self._rescale, "", line)
            rescale = convert_string_to_float(rescale_string)
            return {user_file_rescale: rescale}
        elif self._shift_pattern.match(line) is not None:
            shift_string = re.sub(self._shift, "", line)
            shift = convert_string_to_float(shift_string)
            return {user_file_shift: shift}
        elif self._rescale_fit_pattern.match(line) is not None:
            rescale_fit_string = re.sub(self._rescale_fit, "", line)
            rescale_fit = extract_float_range(rescale_fit_string)
            return {user_file_rescale_shift_fit_min: rescale_fit[0],
                    user_file_rescale_shift_fit_max: rescale_fit[1]}
        elif self._shift_fit_pattern.match(line) is not None:
            shift_fit_string = re.sub(self._shift_fit, "", line)
            shift_fit = extract_float_range(shift_fit_string)
            return {user_file_rescale_shift_fit_min: shift_fit[0],
                    user_file_rescale_shift_fit_max: shift_fit[1]}
        else:
            raise RuntimeError("DetParser: Could not extract line: {0}".format(line))

    @staticmethod
    def get_type():
        return DetParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + DetParser.get_type() + "\\s*/\\s*"


class LimitParser(UserFileComponentParser):
    """
    The LimitParser handles the following structure for
        L/PHI[/NOMIRROR] d1 d2

        L/Q/ q1 q2 [dq[/LIN]]  or  L/Q q1 q2 [dq[/LOG]]
        L/Q q1,dq1,q3,dq2,q2 [/LIN]]  or  L/Q q1,dq1,q3,dq2,q2 [/LOG]]

        L/Q/RCut c
        L/Q/WCut c

        L/QXY qxy1 qxy2 [dqxy[/LIN]]  or  L/QXY qxy1 qxy2 [dqxy[/LOG]]
        L/QXY qxy1,dqxy1,qxy3,dqxy2,qxy2 [/LIN]]  or  L/QXY qxy1,dqxy1,qxy3,dqxy2,qxy2 [/LOG]]

        L/R r1 r2

        L/WAV l1 l2 [dl[/LIN]  or  L/WAV l1 l2 [dl[/LOG]
        L/WAV l1,dl1,l3,dl2,l2 [/LIN]  or  L/WAV l1,dl1,l3,dl2,l2 [/LOG]

        L/EVENTSTIME rebin_str
    """
    Type = "L"

    def __init__(self):
        super(LimitParser, self).__init__()

        # ranges
        self._lin = "\\s*/\\s*LIN\\s*"
        self._log = "\\s*/\\s*LOG\\s*"
        self._lin_or_log = self._lin + "|" + self._log
        self._simple_step = "(\\s+" + float_number + "\\s*(" + self._lin_or_log + ")?)?"
        self._range = float_number + "\\s+" + float_number
        self._simple_range = "\\s*" + self._range + self._simple_step

        self._complex_range = "\\s*" + float_number + space_string + float_number + space_string + float_number +\
                              space_string + float_number + space_string + float_number +\
                              "(\\s*" + self._lin_or_log + ")?"

        # Angle limits
        self._phi_no_mirror = "\\s*/\\s*NOMIRROR\\s*"
        self._phi = "\\s*PHI\\s*(" + self._phi_no_mirror + ")?\\s*"
        self._phi_pattern = re.compile(start_string + self._phi + space_string +
                                       float_number + space_string +
                                       float_number + end_string)

        # Event time limits
        self._events_time = "\\s*EVENTSTIME\\s*"
        self._events_time_pattern = re.compile(start_string + self._events_time +
                                               space_string + rebin_string + end_string)

        # Q Limits
        self._q = "\\s*Q\\s*"
        self._q_simple_pattern = re.compile(start_string + self._q + space_string +
                                            self._simple_range + end_string)
        self._q_complex_pattern = re.compile(start_string + self._q + space_string + self._complex_range + end_string)

        # Qxy limits
        self._qxy = "\\s*QXY\\s*"
        self._qxy_simple_pattern = re.compile(start_string + self._qxy + space_string + self._simple_range + end_string)
        self._qxy_complex_pattern = re.compile(start_string + self._qxy + space_string +
                                               self._complex_range + end_string)

        # Wavelength limits
        self._wavelength = "\\s*WAV\\s*"
        self._wavelength_simple_pattern = re.compile(start_string + self._wavelength + space_string +
                                                     self._simple_range + end_string)
        self._wavelength_complex_pattern = re.compile(start_string + self._wavelength + space_string +
                                                      self._complex_range + end_string)

        # Cut limits
        self._radius_cut = "\\s*Q\\s*/\\s*RCUT\\s*"
        self._radius_cut_pattern = re.compile(start_string + self._radius_cut + space_string +
                                              float_number + end_string)
        self._wavelength_cut = "\\s*Q\\s*/\\s*WCUT\\s*"
        self._wavelength_cut_pattern = re.compile(start_string + self._wavelength_cut +
                                                  space_string + float_number + end_string)

        # Radius limits
        self._radius = "\\s*R\\s*"
        self._radius_pattern = re.compile(start_string + self._radius +
                                          space_string + float_number +
                                          space_string + float_number + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, LimitParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_angle_limit(setting):
            output = self._extract_angle_limit(setting)
        elif self._is_event_binning(setting):
            output = self._extract_event_binning(setting)
        elif self._is_cut_limit(setting):
            output = self._extract_cut_limit(setting)
        elif self._is_radius_limit(setting):
            output = self._extract_radius_limit(setting)
        elif self._is_q_limit(setting):
            output = self._extract_q_limit(setting)
        elif self._is_wavelength_limit(setting):
            output = self._extract_wavelength_limit(setting)
        elif self._is_qxy_limit(setting):
            output = self._extract_qxy_limit(setting)
        else:
            raise RuntimeError("LimitParser: Unknown command for L: {0}".format(line))
        return output

    def _is_angle_limit(self, line):
        return does_pattern_match(self._phi_pattern, line)

    def _is_event_binning(self, line):
        return does_pattern_match(self._events_time_pattern, line)

    def _is_cut_limit(self, line):
        return does_pattern_match(self._radius_cut_pattern, line) or \
               does_pattern_match(self._wavelength_cut_pattern, line)

    def _is_radius_limit(self, line):
        return does_pattern_match(self._radius_pattern, line)

    def _is_q_limit(self, line):
        return does_pattern_match(self._q_simple_pattern, line) or does_pattern_match(self._q_complex_pattern, line)

    def _is_qxy_limit(self, line):
        return does_pattern_match(self._qxy_simple_pattern, line) or does_pattern_match(self._qxy_complex_pattern, line)

    def _is_wavelength_limit(self, line):
        return does_pattern_match(self._wavelength_simple_pattern, line) or\
               does_pattern_match(self._wavelength_complex_pattern, line)

    def _extract_angle_limit(self, line):
        is_no_mirror =  re.search(self._phi_no_mirror, line) is not None
        angles_string = re.sub(self._phi, "", line)
        angles = extract_float_range(angles_string)
        return {user_file_limit_angle_min: angles[0],
                user_file_limit_angle_max: angles[1],
                user_file_no_mirror: is_no_mirror}

    def _extract_event_binning(self, line):
        rebin_values_string = re.sub(self._events_time, "", line)
        rebin_values = extract_float_list(rebin_values_string)
        return {user_file_events_binning: rebin_values}

    def _extract_cut_limit(self, line):
        if self._radius_cut_pattern.match(line) is not None:
            key = user_file_radius_cut
            limit_value = re.sub(self._radius_cut, "", line)
        else:
            key = user_file_wavelength_cut
            limit_value = re.sub(self._wavelength_cut, "", line)
        return {key: convert_string_to_float(limit_value)}

    def _extract_radius_limit(self, line):
        radius_range_string = re.sub(self._radius, "", line)
        radius_range = extract_float_range(radius_range_string)
        return {user_file_radius_limit_min: radius_range[0],
                user_file_radius_limit_max: radius_range[1]}

    def _extract_q_limit(self, line):
        q_range = re.sub(self._q, "", line)
        if does_pattern_match(self._q_simple_pattern, line):
            tags = [user_file_q_min, user_file_q_max,
                    user_file_q_step, user_file_q_step_type]
            output = self._extract_simple_pattern(q_range, tags)
        else:
            tags = [user_file_q_min, user_file_q_step_min_mid, user_file_q_mid,
                    user_file_q_step_mid_max, user_file_q_max, user_file_q_step_type]
            output = self._extract_complex_pattern(q_range, tags)
        return output

    def _extract_qxy_limit(self, line):
        qxy_range = re.sub(self._qxy, "", line)
        if does_pattern_match(self._qxy_simple_pattern, line):
            tags = [user_file_qxy_min, user_file_qxy_max,
                    user_file_qxy_step, user_file_qxy_step_type]
            output = self._extract_simple_pattern(qxy_range, tags)
        else:
            tags = [user_file_qxy_min, user_file_qxy_step_min_mid, user_file_qxy_mid,
                    user_file_qxy_step_mid_max, user_file_qxy_max, user_file_qxy_step_type]
            output = self._extract_complex_pattern(qxy_range, tags)
        return output

    def _extract_wavelength_limit(self, line):
        wavelength_range = re.sub(self._wavelength, "", line)
        if does_pattern_match(self._wavelength_simple_pattern, line):
            tags = [user_file_wavelength_min, user_file_wavelength_max,
                    user_file_wavelength_step, user_file_wavelength_step_type]
            output = self._extract_simple_pattern(wavelength_range, tags)
        else:
            tags = [user_file_wavelength_min, user_file_wavelength_step_min_mid, user_file_wavelength_mid,
                    user_file_wavelength_step_mid_max, user_file_wavelength_max, user_file_wavelength_step_type]
            output = self._extract_complex_pattern(wavelength_range, tags)
        return output

    def _extract_simple_pattern(self, simple_range, tags):
        if re.sub(self._range, "", simple_range, 1) == "":
            float_range = extract_float_range(simple_range)
            output = {tags[0]: float_range[0],
                      tags[1]: float_range[1]}
        else:
            # Extract the step information
            range_removed = re.sub(self._range, "", simple_range, 1)

            # Get the step type
            step_type = self._get_step_type(range_removed)

            # Get the step
            step_string = re.sub(self._lin_or_log, "", range_removed)
            step = convert_string_to_float(step_string)

            # Get the range
            pure_range = re.sub(range_removed, "", simple_range)
            float_range = extract_float_range(pure_range)

            output = {tags[0]: float_range[0],
                      tags[1]: float_range[1],
                      tags[2]: step,
                      tags[3]: step_type}
        return output

    def _extract_complex_pattern(self, complex_range, tags):
        # Get the step type
        step_type = self._get_step_type(complex_range)

        # Remove the step type
        range_with_steps_string = re.sub(self._lin_or_log, "", complex_range)
        range_with_steps = extract_float_range_midpoint_and_steps(range_with_steps_string)

        return {tags[0]: range_with_steps[0],
                tags[1]: range_with_steps[1],
                tags[2]: range_with_steps[2],
                tags[3]: range_with_steps[3],
                tags[4]: range_with_steps[4],
                tags[5]: step_type}

    def _get_step_type(self, range_string):
        return "LOG" if re.search(self._log, range_string) is not None else "LIN"

    @staticmethod
    def get_type():
        return LimitParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + LimitParser.get_type() + "\\s*/\\s*"


class MaskParser(UserFileComponentParser):
    """
    The MaskParser handles the following structure for
        MASK/CLEAR[/TIME]

        MASK[/REAR/FRONT/HAB] Hn[>Hm]  or  MASK Vn[>Vm]  - to mask single wires or 'strips'
        MASK[/REAR/FRONT/HAB] Hn>Hm+Vn>Vm                - to mask a rectangular 'box'
        MASK[/REAR/FRONT/HAB] Hn+Vm - to mask the intersection of Hn and Vm

        MASK Ssp1[>Ssp2]

        MASK/TIME t1 t2 or  MASK/T t1 t2

        MASK/LINE width angle [x y]
    """
    Type = "MASK"

    def __init__(self):
        super(MaskParser, self).__init__()

        # Blocks

        # ranges
        self._two_floats = "\\s*" + float_number + space_string + float_number + "\\s*"
        self._optional_two_floats = "(\\s+" + self._two_floats + "\\s*)?\\s*"
        self._range = "\\s*>\\s*"

        # Line Mask
        self._line = "\\s*LINE\\s*"
        self._line_pattern = re.compile(start_string + self._line + space_string + self._two_floats +
                                        self._optional_two_floats + end_string)

        # Time Mask
        self._time = "\\s*/\\s*TIME\\s*"
        self._time_or_t = "\\s*(TIME|T)\\s*"
        self._time_pattern = re.compile(start_string + self._time_or_t + space_string + self._two_floats + end_string)

        # Clear Mask
        self._clear = "\\s*CLEAR\\s*"
        self._clear_pattern = re.compile(start_string + self._clear + "\\s*(" + self._time + ")?" + end_string)

        # Spectrum Mask
        self._spectrum = "\\s*S\\s*"
        self._additional_spectrum = "(\\s*>" + self._spectrum + integer_number+")"
        self._spectrum_range_pattern = re.compile(start_string + self._spectrum + integer_number +
                                                  self._additional_spectrum + end_string)
        self._spectrum_single_pattern = re.compile(start_string + self._spectrum + integer_number + end_string)

        # Strip Masks
        self._hab = "\\s*\\HAB|FRONT\\s*"
        self._lab = "\\s*LAB|REAR|MAIN\\s*"
        self._detector = "\\s*(" + self._hab + "|" + self._lab + ")?\\s*"

        # Vertical strip Mask
        self._v = "\\s*V\\s*"
        self._additional_v = "(\\s*>" + self._v + integer_number + ")"
        self._single_vertical_strip_pattern = re.compile(start_string + self._detector + self._v +
                                                         integer_number + end_string)
        self._range_vertical_strip_pattern = re.compile(start_string + self._detector + self._v +
                                                        integer_number + self._additional_v + end_string)

        # Horizontal strip Mask
        self._h = "\\s*H\\s*"
        self._additional_h = "(\\s*>" + self._h + integer_number + ")"
        self._single_horizontal_strip_pattern = re.compile(start_string + self._detector + self._h +
                                                           integer_number + end_string)
        self._range_horizontal_strip_pattern = re.compile(start_string + self._detector + self._h +
                                                          integer_number + self._additional_h + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, MaskParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_line_mask(setting):
            output = self._extract_line_mask(setting)
        elif self._is_time_mask(setting):
            output = self._extract_time_mask(setting)
        elif self._is_clear_mask(setting):
            output = self._extract_clear_mask(setting)
        elif self._is_single_spectrum_mask(setting):
            output = self._extract_single_spectrum_mask(setting)
        elif self._is_spectrum_range_mask(setting):
            output = self._extract_spectrum_range_mask(setting)
        elif self._is_vertical_single_strip_mask(setting):
            output = self._extract_vertical_single_strip_mask(setting)
        elif self._is_vertical_range_strip_mask(setting):
            output = self._extract_vertical_range_strip_mask(setting)
        elif self._is_horizontal_single_strip_mask(setting):
            output = self._extract_horizontal_single_strip_mask(setting)
        elif self._is_horizontal_range_strip_mask(setting):
            output = self._extract_horizontal_range_strip_mask(setting)
        else:
            raise RuntimeError("MaskParser: Unknown command for MASK: {0}".format(line))
        return output

    def _is_line_mask(self, line):
        return does_pattern_match(self._line_pattern, line)

    def _is_time_mask(self, line):
        return does_pattern_match(self._time_pattern, line)

    def _is_clear_mask(self, line):
        return does_pattern_match(self._clear_pattern, line)

    def _is_single_spectrum_mask(self, line):
        return does_pattern_match(self._spectrum_single_pattern, line)

    def _is_spectrum_range_mask(self, line):
        return does_pattern_match(self._spectrum_range_pattern, line)

    def _is_vertical_single_strip_mask(self, line):
        return does_pattern_match(self._single_vertical_strip_pattern, line)

    def _is_vertical_range_strip_mask(self, line):
        return does_pattern_match(self._range_vertical_strip_pattern, line)

    def _is_horizontal_single_strip_mask(self, line):
        return does_pattern_match(self._single_horizontal_strip_pattern, line)

    def _is_horizontal_range_strip_mask(self, line):
        return does_pattern_match(self._range_horizontal_strip_pattern, line)

    def _extract_line_mask(self, line):
        line_string = re.sub(self._line, "", line)
        line_values = extract_float_list(line_string, " ")
        length_values = len(line_values)
        if length_values == 2:
            output = {user_file_line_width: line_values[0],
                      user_file_line_angle: line_values[1]}
        elif length_values == 4:
            output = {user_file_line_width: line_values[0],
                      user_file_line_angle: line_values[1],
                      user_file_line_x: line_values[2],
                      user_file_line_y: line_values[3]}
        else:
            raise ValueError("MaskParser: Line mask accepts wither 2 or 4 parameters,"
                             " but {0} parameters were passed in.".format(length_values))
        return output

    def _extract_time_mask(self, line):
        min_and_max_time_range = re.sub(self._time_or_t, "", line)
        min_and_max_time = extract_float_range(min_and_max_time_range)
        return {user_file_time_min: min_and_max_time[0],
                user_file_time_max: min_and_max_time[1]}

    def _extract_clear_mask(self, line):
        clear_removed = re.sub(self._clear, "", line)
        return {user_file_clear_detector_mask: True} if clear_removed == "" else {user_file_clear_time_mask: True}

    def _extract_single_spectrum_mask(self, line):
        single_spectrum_string = re.sub(self._spectrum, "", line)
        single_spectrum = convert_string_to_integer(single_spectrum_string)
        return {use_file_single_spectrum_mask: single_spectrum}

    def _extract_spectrum_range_mask(self, line):
        spectrum_range_string = re.sub(self._spectrum, "", line)
        spectrum_range_string = re.sub(self._range, " ", spectrum_range_string)
        spectrum_range = extract_int_range(spectrum_range_string)
        return {use_file_spectrum_range_mask: [spectrum_range[0], spectrum_range[1]]}

    def _extract_vertical_single_strip_mask(self, line):
        key = user_file_vertical_single_strip_mask_hab if re.search(self._hab, line) is not None \
            else user_file_vertical_single_strip_mask_lab

        single_vertical_strip_string = re.sub(self._detector, "", line)
        single_vertical_strip_string = re.sub(self._v, "", single_vertical_strip_string)
        single_vertical_strip = convert_string_to_integer(single_vertical_strip_string)
        return {key: single_vertical_strip}

    def _extract_vertical_range_strip_mask(self, line):
        key = user_file_vertical_range_strip_mask_hab if re.search(self._hab, line) is not None \
            else user_file_vertical_range_strip_mask_lab

        range_vertical_strip_string = re.sub(self._detector, "", line)
        range_vertical_strip_string = re.sub(self._v, "", range_vertical_strip_string)
        range_vertical_strip_string = re.sub(self._range, " ", range_vertical_strip_string)
        range_vertical_strip = extract_int_range(range_vertical_strip_string)
        return {key: [range_vertical_strip[0], range_vertical_strip[1]]}

    def _extract_horizontal_single_strip_mask(self, line):
        key = user_file_horizontal_single_strip_mask_hab if re.search(self._hab, line) is not None \
            else user_file_horizontal_single_strip_mask_lab

        single_horizontal_strip_string = re.sub(self._detector, "", line)
        single_horizontal_strip_string = re.sub(self._h, "", single_horizontal_strip_string)
        single_horizontal_strip = convert_string_to_integer(single_horizontal_strip_string)
        return {key: single_horizontal_strip}

    def _extract_horizontal_range_strip_mask(self, line):
        key = user_file_horizontal_range_strip_mask_hab if re.search(self._hab, line) is not None \
            else user_file_horizontal_range_strip_mask_lab

        range_horizontal_strip_string = re.sub(self._detector, "", line)
        range_horizontal_strip_string = re.sub(self._h, "", range_horizontal_strip_string)
        range_horizontal_strip_string = re.sub(self._range, " ", range_horizontal_strip_string)
        range_horizontal_strip = extract_int_range(range_horizontal_strip_string)
        return {key: [range_horizontal_strip[0], range_horizontal_strip[1]]}

    @staticmethod
    def get_type():
        return MaskParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + MaskParser.get_type() + "(\\s*/\\s*|\\s+)"


class SampleParser(UserFileComponentParser):
    """
    The SampleParser handles the following structure for
        SAMPLE/OFFSET z1
        SAMPLE/PATH/ON
        SAMPLE/PATH/OFF
    """
    Type = "SAMPLE"

    def __init__(self):
        super(SampleParser, self).__init__()

        # Offset
        self._offset = "\\s*OFFSET\\s*"
        self._offset_pattern = re.compile(start_string + self._offset + space_string + float_number + end_string)

        # Path
        self._on = "\\s*ON\\s*"
        self._off = "\\s*OFF\\s*"
        self._path = "\\s*PATH\\s*/\\s*"
        self._path_pattern = re.compile(start_string + self._path + "(" + self._on + "|" + self._off + ")" + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, SampleParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_sample_path(setting):
            output = self._extract_sample_path(setting)
        elif self._is_sample_offset(setting):
            output = self._extract_sample_offset(setting)
        else:
            raise RuntimeError("SampleParser: Unknown command for SAMPLE: {0}".format(line))
        return output

    def _is_sample_path(self, line):
        return does_pattern_match(self._path_pattern, line)

    def _is_sample_offset(self, line):
        return does_pattern_match(self._offset_pattern, line)

    def _extract_sample_path(self, line):
        value = False if re.search(self._off, line) is not None else True
        return {user_file_sample_path: value}

    def _extract_sample_offset(self, line):
        offset_string = re.sub(self._offset, "", line)
        offset = convert_string_to_float(offset_string)
        return {user_file_sample_offset: offset}

    @staticmethod
    def get_type():
        return SampleParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + SampleParser.get_type() + "(\\s*/)\\s*"


class SetParser(UserFileComponentParser):
    """
    The SetParser handles the following structure for
        SET CENTRE[/MAIN] x y
        SET CENTRE/HAB x y
        SET SCALES s a b c d
    """
    Type = "SET"

    def __init__(self):
        super(SetParser, self).__init__()

        # Scales
        self._scales = "\\s*SCALES\\s*"
        self._scales_pattern = re.compile(start_string + self._scales + space_string + float_number + space_string +
                                          float_number + space_string + float_number + space_string + float_number +
                                          space_string + float_number + end_string)

        # Centre
        self._centre = "\\s*CENTRE\\s*"
        self._hab = "\\s*(HAB|FRONT)\\s*"
        self._lab = "\\s*(LAB|REAR|MAIN)\\s*"
        self._hab_or_lab = "\\s*((/" + self._hab + "|/" + self._lab + "))\\s*"
        self._centre_pattern = re.compile(start_string + self._centre + "\\s*(" + self._hab_or_lab + space_string +
                                          ")?\\s*" + float_number + space_string + float_number + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, SetParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_scales(setting):
            output = self._extract_scales(setting)
        elif self._is_centre(setting):
            output = self._extract_centre(setting)
        else:
            raise RuntimeError("SetParser: Unknown command for SET: {0}".format(line))
        return output

    def _is_scales(self, line):
        return does_pattern_match(self._scales_pattern, line)

    def _is_centre(self, line):
        return does_pattern_match(self._centre_pattern, line)

    def _extract_scales(self, line):
        scales_string = re.sub(self._scales, "", line)
        scales = extract_float_list(scales_string, separator=" ")
        if len(scales) != 5:
            raise ValueError("SetParser: Expected 5 entries for the SCALES setting, but got {0}.".format(len(scales)))
        return {user_file_scales: scales}

    def _extract_centre(self, line):
        key = user_file_centre_hab if re.search(self._hab, line) is not None else user_file_centre_lab
        centre_string = re.sub(self._centre, "", line)
        centre_string = re.sub(self._hab_or_lab, "", centre_string)
        centre = extract_float_range(centre_string)
        return {key: centre}

    @staticmethod
    def get_type():
        return SetParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + SetParser.get_type() + "\\s+"


class TransParser(UserFileComponentParser):
    """
    The TransParser handles the following structure for
        TRANS/TRANSPEC=n
        TRANS/SAMPLEWS=ws1
        TRANS/CANWS=ws2
        TRANS/TRANSPEC=4/SHIFT=z
        TRANS/RADIUS=r
        TRANS/ROI=roi_mask.xml
        TRANS/MASK=mask.xml
    """
    Type = "TRANS"

    def __init__(self):
        super(TransParser, self).__init__()
        # General
        self._single_file = "[\\w]+(\\.XML)"
        self._multiple_files = self._single_file + "(,\\s*" + self._single_file + ")*\\s*"
        self._workspace = "[\\w]+"

        # Trans Spec
        self._trans_spec = "\\s*TRANSSPEC\\s*=\\s*"
        self._trans_spec_pattern = re.compile(start_string + self._trans_spec + integer_number +
                                              end_string)

        # Trans Spec Shift
        self._shift = "\\s*/\\s*SHIFT\\s*=\\s*"
        self._trans_spec_4 = self._trans_spec + "4"
        self._trans_spec_shift_pattern = re.compile(start_string + self._trans_spec_4 + self._shift + float_number +
                                                    end_string)

        # Radius
        self._radius = "\\s*RADIUS\\s*=\\s*"
        self._radius_pattern = re.compile(start_string + self._radius + float_number)

        # ROI
        self._roi = "\\s*ROI\\s*=\\s*"
        self._roi_pattern = re.compile(start_string + self._roi + self._multiple_files + end_string)

        # Mask
        self._mask = "\\s*MASK\\s*=\\s*"
        self._mask_pattern = re.compile(start_string + self._mask + self._multiple_files + end_string)

        # CanWS
        self._can_workspace = "\\s*CANWS\\s*=\\s*"
        self._can_workspace_pattern = re.compile(start_string + self._can_workspace + self._workspace +
                                                 end_string)
        # SampleWS
        self._sample_workspace = "\\s*SAMPLEWS\\s*=\\s*"
        self._sample_workspace_pattern = re.compile(start_string + self._sample_workspace + self._workspace +
                                                    end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, TransParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_trans_spec(setting):
            output = self._extract_trans_spec(setting)
        elif self._is_trans_spec_shift(setting):
            output = self._extract_trans_spec_shift(setting)
        elif self._is_radius(setting):
            output = self._extract_radius(setting)
        elif self._is_roi(setting):
            # Note that we need the original line in order to extract the the case sensitive meaning
            output = self._extract_roi(setting, line)
        elif self._is_mask(setting):
            # Note that we need the original line in order to extract the the case sensitive meaning
            output = self._extract_mask(setting, line)
        elif self._is_sample_workspace(setting):
            # Note that we need the original line in order to extract the the case sensitive meaning
            output = self._extract_sample_workspace(setting, line)
        elif self._is_can_workspace(setting):
            # Note that we need the original line in order to extract the the case sensitive meaning
            output = self._extract_can_workspace(setting, line)
        else:
            raise RuntimeError("TransParser: Unknown command for TRANS: {0}".format(line))
        return output

    def _is_trans_spec(self, line):
        return does_pattern_match(self._trans_spec_pattern, line)

    def _is_trans_spec_shift(self, line):
        return does_pattern_match(self._trans_spec_shift_pattern, line)

    def _is_radius(self, line):
        return does_pattern_match(self._radius_pattern, line)

    def _is_roi(self, line):
        return does_pattern_match(self._roi_pattern, line)

    def _is_mask(self, line):
        return does_pattern_match(self._mask_pattern, line)

    def _is_sample_workspace(self, line):
        return does_pattern_match(self._sample_workspace_pattern, line)

    def _is_can_workspace(self, line):
        return does_pattern_match(self._can_workspace_pattern, line)

    def _extract_trans_spec(self, line):
        trans_spec_string = re.sub(self._trans_spec, "", line)
        trans_spec = convert_string_to_integer(trans_spec_string)
        return {user_file_trans_spec: trans_spec}

    def _extract_trans_spec_shift(self, line):
        trans_spec_shift_string = re.sub(self._trans_spec_4, "", line)
        trans_spec_shift_string = re.sub(self._shift, "", trans_spec_shift_string)
        trans_spec_shift = convert_string_to_float(trans_spec_shift_string)
        return {user_file_trans_spec_shift: trans_spec_shift}

    def _extract_radius(self, line):
        radius_string = re.sub(self._radius, "", line)
        radius = convert_string_to_float(radius_string)
        return {user_file_trans_radius: radius}

    def _extract_roi(self, line, original_line):
        file_names = TransParser.extract_file_names(line, original_line, self._roi)
        return {user_file_trans_roi: file_names}

    def _extract_mask(self, line, original_line):
        file_names = TransParser.extract_file_names(line, original_line, self._mask)
        return {user_file_trans_mask: file_names}

    def _extract_sample_workspace(self, line, original_line):
        sample_workspace = TransParser.extract_workspace(line, original_line, self._sample_workspace)
        return {user_file_trans_sample_workspace:  sample_workspace}

    def _extract_can_workspace(self, line, original_line):
        can_workspace = TransParser.extract_workspace(line, original_line, self._can_workspace)
        return {user_file_trans_can_workspace:  can_workspace}

    @staticmethod
    def extract_workspace(line, original_line, to_remove):
        element = re.sub(to_remove, "", line)
        element = element.strip()
        return re.search(element, original_line, re.IGNORECASE).group(0)

    @staticmethod
    def extract_file_names(line, original_line, to_remove):
        elements_string = re.sub(to_remove, "", line)
        elements = extract_string_list(elements_string)
        return [re.search(element, original_line, re.IGNORECASE).group(0) for element in elements]

    @staticmethod
    def get_type():
        return TransParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + TransParser.get_type() + "\\s*/\\s*"


class TubeCalibFileParser(UserFileComponentParser):
    """
    The TransParser handles the following structure for
        TUBECALIBFILE=calib_file.nxs
    """
    Type = "TUBECALIBFILE"

    def __init__(self):
        super(TubeCalibFileParser, self).__init__()

        self._tube_calib_file = "\\s*[\\w]+(\\.NXS)\\s*"
        self._tube_calib_file_pattern = re.compile(start_string + self._tube_calib_file + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, TubeCalibFileParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_tube_calib_file(setting):
            output = self._extract_tube_calib_file(setting, line)
        else:
            raise RuntimeError("TubeCalibFileParser: Unknown command for TUBECALIBFILE: {0}".format(line))
        return output

    def _is_tube_calib_file(self, line):
        return does_pattern_match(self._tube_calib_file_pattern, line)

    @staticmethod
    def _extract_tube_calib_file(line, original_line):
        file_name_capital = line.strip()
        file_name = re.search(file_name_capital, original_line, re.IGNORECASE).group(0)
        return {user_file_tube_calibration: file_name}

    @staticmethod
    def get_type():
        return TubeCalibFileParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + TubeCalibFileParser.get_type() + "\\s*=\\s*"


class QResolutionParser(UserFileComponentParser):
    """
    The TransParser handles the following structure for
        QRESOL/ON
        QRESOL/OFF
        QRESOL/DELTAR=dr
        QRESOL/LCOLLIM="lcollim"
        QRESOL/MODERATOR=moderator_rkh_file.txt
        QRESOL/A1="a1"
        QRESOL/A2="a2"
        QRESOL/H1="h1"
        QRESOL/H2="h2"
        QRESOL/W1="w1"
        QRESOL/W2="w2"
    """
    Type = "QRESOLUTION"

    def __init__(self):
        super(QResolutionParser, self).__init__()

        # On Off
        self._on = "\\s*ON\\s*"
        self._off = "\\s*OFF\\s*"
        self._on_or_off = "\\s*(" + self._on + "|" + self._off + ")\\s*"
        self._on_or_off_pattern = re.compile(start_string + self._on_or_off + end_string)

        # Delta R
        self._delta_r = "\\s*DELTAR\\s*=\\s*"
        self._delta_r_pattern = re.compile(start_string + self._delta_r + float_number + end_string)

        # Collimation Length
        self._collimation_length = "\\s*LCOLLIM\\s*=\\s*"
        self._collimation_length_pattern = re.compile(start_string + self._collimation_length +
                                                      float_number + end_string)

        # A1
        self._a1 = "\\s*A1\\s*=\\s*"
        self._a1_pattern = re.compile(start_string + self._a1 + float_number + end_string)

        # A2
        self._a2 = "\\s*A2\\s*=\\s*"
        self._a2_pattern = re.compile(start_string + self._a2 + float_number + end_string)

        # H1
        self._h1 = "\\s*H1\\s*=\\s*"
        self._h1_pattern = re.compile(start_string + self._h1 + float_number + end_string)

        # H2
        self._h2 = "\\s*H2\\s*=\\s*"
        self._h2_pattern = re.compile(start_string + self._h2 + float_number + end_string)

        # W1
        self._w1 = "\\s*W1\\s*=\\s*"
        self._w1_pattern = re.compile(start_string + self._w1 + float_number + end_string)

        # W2
        self._w2 = "\\s*W2\\s*=\\s*"
        self._w2_pattern = re.compile(start_string + self._w2 + float_number + end_string)

        # Moderator
        self._moderator = "\\s*MODERATOR\\s*=\\s*"
        self._file = "[\\w]+(\\.TXT)"
        self._moderator_pattern = re.compile(start_string + self._moderator + self._file)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, QResolutionParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_on_off(setting):
            output = self._extract_on_off(setting)
        elif self._is_delta_r(setting):
            output = self._extract_delta_r(setting)
        elif self._is_a1(setting):
            output = self._extract_a1(setting)
        elif self._is_a2(setting):
            output = self._extract_a2(setting)
        elif self._is_h1(setting):
            output = self._extract_h1(setting)
        elif self._is_w1(setting):
            output = self._extract_w1(setting)
        elif self._is_h2(setting):
            output = self._extract_h2(setting)
        elif self._is_w2(setting):
            output = self._extract_w2(setting)
        elif self._is_collimation_length(setting):
            output = self._extract_collimation_length(setting)
        elif self._is_moderator(setting):
            output = self._extract_moderator(setting, line)
        else:
            raise RuntimeError("QResolutionParser: Unknown command for QRESOLUTION: {0}".format(line))
        return output

    def _is_on_off(self, line):
        return does_pattern_match(self._on_or_off_pattern, line)

    def _is_delta_r(self, line):
        return does_pattern_match(self._delta_r_pattern, line)

    def _is_a1(self, line):
        return does_pattern_match(self._a1_pattern, line)

    def _is_a2(self, line):
        return does_pattern_match(self._a2_pattern, line)

    def _is_h1(self, line):
        return does_pattern_match(self._h1_pattern, line)

    def _is_w1(self, line):
        return does_pattern_match(self._w1_pattern, line)

    def _is_h2(self, line):
        return does_pattern_match(self._h2_pattern, line)

    def _is_w2(self, line):
        return does_pattern_match(self._w2_pattern, line)

    def _is_collimation_length(self, line):
        return does_pattern_match(self._collimation_length_pattern, line)

    def _is_moderator(self, line):
        return does_pattern_match(self._moderator_pattern, line)

    def _extract_on_off(self, line):
        value = False if re.search(self._off, line) is not None else True
        return {user_file_q_resolution_on: value}

    def _extract_delta_r(self, line):
        return {user_file_q_resolution_delta_r: QResolutionParser.extract_float(line, self._delta_r)}

    def _extract_collimation_length(self, line):
        return {user_file_q_resolution_collimation_length: QResolutionParser.extract_float(line,
                                                                                           self._collimation_length)}

    def _extract_a1(self, line):
        return {user_file_q_resolution_a1: QResolutionParser.extract_float(line, self._a1)}

    def _extract_a2(self, line):
        return {user_file_q_resolution_a2: QResolutionParser.extract_float(line, self._a2)}

    def _extract_h1(self, line):
        return {user_file_q_resolution_h1: QResolutionParser.extract_float(line, self._h1)}

    def _extract_w1(self, line):
        return {user_file_q_resolution_w1: QResolutionParser.extract_float(line, self._w1)}

    def _extract_h2(self, line):
        return {user_file_q_resolution_h2: QResolutionParser.extract_float(line, self._h2)}

    def _extract_w2(self, line):
        return {user_file_q_resolution_w2: QResolutionParser.extract_float(line, self._w2)}

    def _extract_moderator(self, line, original_line):
        moderator_capital = re.sub(self._moderator, "", line)
        moderator = re.search(moderator_capital, original_line, re.IGNORECASE).group(0)
        return {user_file_q_resolution_moderator: moderator}

    @staticmethod
    def extract_float(line, to_remove):
        value_string = re.sub(to_remove, "", line)
        return convert_string_to_float(value_string)

    @staticmethod
    def get_type():
        return QResolutionParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + QResolutionParser.get_type() + "\\s*/\\s*"


class FitParser(UserFileComponentParser):
    """
    The FitParser handles the following structure for
        FIT/TRANS/CLEAR  or  FIT/TRANS/OFF
        FIT/TRANS/LIN [w1 w2]  or  FIT/TRANS/LINEAR [w1 w2]  or  FIT/TRANS/STRAIGHT [w1 w2]
        FIT/TRANS/LOG [w1 w2]  or  FIT/TRANS/YLOG [w1 w2]
        FIT/MONITOR time1 time2
        FIT/TRANS/[CAN/|SAMPLE/][LIN|LOG|POLYNOMIAL[2|3|4|5]]
    """
    Type = "FIT"
    sample = "SAMPLE"
    can = "CAN"
    both = "BOTH"

    def __init__(self):
        super(FitParser, self).__init__()

        # General
        self._trans_prefix = "\\s*TRANS\\s*/\\s*"

        # Clear
        trans_off_or_clear = self._trans_prefix + "(OFF|CLEAR)\\s*"
        self._trans_clear_pattern = re.compile(start_string + trans_off_or_clear + end_string)

        # Range based fits
        self._lin = "\\s*(LINEAR|LIN|STRAIGHT)\\s*"
        self._log = "\\s*(YLOG|LOG)\\s*"
        self._lin_or_log = "\\s*(" + self._trans_prefix + self._lin + "|" + self._trans_prefix + self._log + ")\\s*"
        self._range_based_fits_pattern = re.compile(start_string + self._lin_or_log + space_string +
                                                    float_number + space_string + float_number + end_string)

        # General fits
        self._sample = "\\s*SAMPLE\\s*/\\s*"
        self._can = "\\s*CAN\\s*/\\s*"
        self._can_or_sample = "\\s*(" + self._can + "|" + self._sample + ")"
        self._optional_can_or_sample = "\\s*(" + self._can_or_sample + ")?"

        self._polynomial = "\\s*POLYNOMIAL\\s*"
        self._polynomial_with_optional_order = self._polynomial + "(2|3|4|5)?\\s*"
        self._lin_or_log_or_poly = "\\s*(" + self._lin + "|" + self._log + "|" +\
                                   self._polynomial_with_optional_order + ")\\s*"

        self._general_fit_pattern = re.compile(start_string + self._trans_prefix + self._optional_can_or_sample +
                                               self._lin_or_log_or_poly + end_string)

        # Monitor times
        self._monitor = "\\s*MONITOR\\s*"
        self._monitor_pattern = re.compile(start_string + self._monitor + space_string + float_number + space_string +
                                           float_number + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, FitParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_clear(setting):
            output = FitParser.extract_clear()
        elif self._is_range_based_fit(setting):
            output = self._extract_range_based_fit(setting)
        elif self._is_monitor(setting):
            output = self._extract_monitor(setting)
        elif self._is_general_fit(setting):
            output = self._extract_general_fit(setting)
        else:
            raise RuntimeError("FitParser: Unknown command for FIT: {0}".format(line))
        return output

    def _is_clear(self, line):
        return does_pattern_match(self._trans_clear_pattern, line)

    def _is_range_based_fit(self, line):
        return does_pattern_match(self._range_based_fits_pattern, line)

    def _is_monitor(self, line):
        return does_pattern_match(self._monitor_pattern, line)

    def _is_general_fit(self, line):
        return does_pattern_match(self._general_fit_pattern, line)

    def _extract_range_based_fit(self, line):
        key = user_file_range_based_fit_log if re.search(self._log, line) is not None else user_file_range_based_fit_lin
        value_string = re.sub(self._lin_or_log, "", line)
        values = extract_float_range(value_string)
        return {key: values}

    def _extract_monitor(self, line):
        values_string = re.sub(self._monitor, "", line)
        values = extract_float_range(values_string)
        return {user_file_monitor_times: values}

    def _extract_general_fit(self, line):
        fit_type = self._get_fit_type(line)
        ws_type = self._get_workspace_type(line)

        output = dict()

        poly_order = self._get_polynomial_order(fit_type, line)

        if ws_type == FitParser.both:
            if fit_type == user_file_lin:
                output.update({user_file_fit_can: user_file_lin})
                output.update({user_file_fit_sample: user_file_lin})
            elif fit_type == user_file_log:
                output.update({user_file_fit_can: user_file_log})
                output.update({user_file_fit_sample: user_file_log})
            else:
                output.update({user_file_fit_can_poly: poly_order})
                output.update({user_file_fit_sample_poly: poly_order})
        elif ws_type == FitParser.sample:
            if fit_type == user_file_lin:
                output.update({user_file_fit_sample: user_file_lin})
            elif fit_type == user_file_log:
                output.update({user_file_fit_sample: user_file_log})
            else:
                output.update({user_file_fit_sample_poly: poly_order})
        elif ws_type == FitParser.can:
            if fit_type == user_file_lin:
                output.update({user_file_fit_can: user_file_lin})
            elif fit_type == user_file_log:
                output.update({user_file_fit_can: user_file_log})
            else:
                output.update({user_file_fit_can_poly: poly_order})
        else:
            raise RuntimeError("FitParser: Encountered an unknown workspace selection: {0}".format(line))
        return output

    def _get_polynomial_order(self, fit_type, line):
        if fit_type != user_file_poly:
            return 0
        # Remove trans
        poly_string = re.sub(self._trans_prefix, "", line)
        poly_string = re.sub(self._can_or_sample, "", poly_string)
        poly_string = re.sub(self._polynomial, "", poly_string)
        return 2 if poly_string == "" else convert_string_to_integer(poly_string)

    def _get_fit_type(self, line):
        if re.search(self._log, line) is not None:
            fit_type = user_file_log
        elif re.search(self._lin, line) is not None:
            fit_type = user_file_lin
        elif re.search(self._polynomial, line) is not None:
            fit_type = user_file_poly
        else:
            raise RuntimeError("FitParser: Encountered unknown fit function: {0}".format(line))
        return fit_type

    def _get_workspace_type(self, line):
        if re.search(self._sample, line) is not None:
            ws_type = FitParser.sample
        elif re.search(self._can, line) is not None:
            ws_type = FitParser.can
        else:
            ws_type = FitParser.both
        return ws_type

    @staticmethod
    def extract_clear():
        return {user_file_fit_clear: True}

    @staticmethod
    def get_type():
        return FitParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + FitParser.get_type() + "\\s*/\\s*"


class GravityParser(UserFileComponentParser):
    """
    The GravityParser handles the following structure for
        GRAVITY ON
        GRAVITY OFF
        GRAVITY/LEXTRA=l1
    """
    Type = "GRAVITY"

    def __init__(self):
        super(GravityParser, self).__init__()

        # On Off
        self._on = "ON"
        self._on_off = "\\s*(OFF|" + self._on + ")"
        self._on_off_pattern = re.compile(start_string + self._on_off + end_string)

        # Extra length
        self._extra_length = "\\s*LEXTRA\\s*=\\s*"
        self._extra_length_pattern = re.compile(start_string + self._extra_length + float_number + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, GravityParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_on_off(setting):
            output = self._extract_on_off(setting)
        elif self._is_extra_length(setting):
            output = self._extract_extra_length(setting)
        else:
            raise RuntimeError("GravityParser: Unknown command for GRAVITY: {0}".format(line))
        return output

    def _is_on_off(self, line):
        return does_pattern_match(self._on_off_pattern, line)

    def _is_extra_length(self, line):
        return does_pattern_match(self._extra_length_pattern, line)

    def _extract_on_off(self, line):
        value = re.sub(self._on, "", line).strip() == ""
        return {user_file_gravity_on: value}

    def _extract_extra_length(self, line):
        extra_length_string = re.sub(self._extra_length, "", line)
        extra_length = convert_string_to_float(extra_length_string)
        return {user_file_gravity_extra_length: extra_length}

    @staticmethod
    def get_type():
        return GravityParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + GravityParser.get_type() + "(\\s*/\\s*|\\s+)"


class MaskFileParser(UserFileComponentParser):
    """
    The MaskFileParser handles the following structure for
        MASKFILE=mask1.xml,mask2.xml,...
    """
    Type = "MASKFILE"

    def __init__(self):
        super(MaskFileParser, self).__init__()

        # MaskFile
        self._single_file = "[\\w]+(\\.XML)"
        self._multiple_files = self._single_file + "(,\\s*" + self._single_file + ")*\\s*"
        self._mask_file_pattern = re.compile(start_string + "\\s*" + self._multiple_files + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, MaskFileParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_mask_file(setting):
            output = MaskFileParser.extract_mask_file(setting, line)
        else:
            raise RuntimeError("MaskFileParser: Unknown command for MASKFILE: {0}".format(line))
        return output

    def _is_mask_file(self, line):
        return does_pattern_match(self._mask_file_pattern, line)

    @staticmethod
    def extract_mask_file(line, original_line):
        elements_capital = extract_string_list(line)
        elements = [re.search(element, original_line, re.IGNORECASE).group(0) for element in elements_capital]
        return {user_file_mask_file: elements}

    @staticmethod
    def get_type():
        return MaskFileParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + MaskFileParser.get_type() + "(\\s*=\\s*)"


class MonParser(UserFileComponentParser):
    """
    The MonParser handles the following structure for
        MON/DIRECT[/FRONT]=file  or  MON/DIRECT[/REAR]=file
        MON/FLAT[/FRONT]=file  or  MON/FLAT[/REAR]=file
        MON/HAB=file

        MON/LENGTH=z sp [/INTERPOLATE]  or  MON/LENGTH=z sp [/INTERPOLATE]
        MON[/TRANS]/SPECTRUM=sp [/INTERPOLATE]  or  MON[/TRANS]/SPECTRUM=sp [/INTERPOLATE]
    """
    Type = "MON"

    def __init__(self):
        super(MonParser, self).__init__()

        # General
        self._hab = "\\s*HAB|FRONT\\s*"
        self._lab = "\\s*LAB|REAR|MAIN\\s*"
        self._detector = "\\s*/\\s*(" + self._hab + "|" + self._lab + ")\\s*"
        self._optional_detector = "\\s*(" + self._detector + ")?\\s*"
        self._equal = "\\s*=\\s*"

        self._file_path = "\\s*[^\\s]*\\.[\\w]+\\s*"

        # Length
        self._length = "\\s*LENGTH\\s*=\\s*"
        self._interpolate = "\\s*/\\s*INTERPOLATE\\s*"
        self._length_pattern = re.compile(start_string + self._length + float_number + space_string + integer_number +
                                          "(\\s*" + self._interpolate + "\\s*)?" + end_string)

        # Direct
        self._direct = "\\s*DIRECT\\s*"
        self._direct_pattern = re.compile(start_string + self._direct + self._optional_detector +
                                          self._equal + self._file_path + end_string)

        # Flat
        self._flat = "\\s*FLAT\\s*"
        self._flat_pattern = re.compile(start_string + self._flat + self._optional_detector +
                                        self._equal + self._file_path + end_string)

        # Flat
        self._hab_file = "\\s*HAB\\s*"
        self._hab_pattern = re.compile(start_string + self._hab_file + self._optional_detector +
                                       self._equal + self._file_path + end_string)

        # Spectrum
        self._spectrum = "\\s*SPECTRUM\\s*"
        self._trans = "\\s*TRANS\\s*"
        self._spectrum_pattern = re.compile(start_string + "(\\s*" + self._trans + "\\s*/\\s*)?" + self._spectrum +
                                            self._equal + integer_number + "(\\s*" + self._interpolate + "\\s*)?" +
                                            end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        line = escape_special_characters_for_file_path(line)
        setting = UserFileComponentParser.get_settings(line, MonParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_length(setting):
            output = self._extract_length(setting)
        elif self._is_direct(setting):
            output = self._extract_direct(setting, line)
        elif self._is_flat(setting):
            output = self._extract_flat(setting, line)
        elif self._is_hab(setting):
            output = self._extract_hab(setting, line)
        elif self._is_spectrum(setting):
            output = self._extract_spectrum(setting)
        else:
            raise RuntimeError("MonParser: Unknown command for MON: {0}".format(line))
        return output

    def _is_length(self, line):
        return does_pattern_match(self._length_pattern, line)

    def _is_direct(self, line):
        return does_pattern_match(self._direct_pattern, line)

    def _is_flat(self, line):
        return does_pattern_match(self._flat_pattern, line)

    def _is_hab(self, line):
        return does_pattern_match(self._hab_pattern, line)

    def _is_spectrum(self, line):
        return does_pattern_match(self._spectrum_pattern, line)

    def _extract_length(self, line):
        output = dict()
        if re.search(self._interpolate, line) is not None:
            output.update({user_file_mon_interpolate: True})
            line = re.sub(self._interpolate, "", line)
        length_string = re.sub(self._length, "", line)
        length_entries = extract_float_list(length_string, separator=" ")
        if len(length_entries) != 2:
            raise RuntimeError("MonParser: Length setting needs 2 numeric parameters, "
                               "but received {0}.".format(len(length_entries)))
        output.update({user_file_monitor_length: [length_entries[0], int(length_entries[1])]})
        return output

    def _extract_direct(self, line, original_line):
        key = user_file_direct_hab if re.search(self._hab, line, re.IGNORECASE) is not None\
            else user_file_direct_lab
        file_path = self._extract_file_path(line, original_line, self._direct)
        return {key: file_path}

    def _extract_flat(self, line, original_line):
        key = user_file_flat_hab if re.search(self._hab, line, re.IGNORECASE) is not None\
            else user_file_flat_lab
        file_path = self._extract_file_path(line, original_line, self._flat)
        return {key: file_path}

    def _extract_hab(self, line, original_line):
        file_path = self._extract_file_path(line, original_line, self._hab_file)
        return {user_file_mon_hab: file_path}

    def _extract_file_path(self, line, original_line, to_remove):
        direct = re.sub(self._detector, "", line)
        direct = re.sub(to_remove, "", direct)
        direct = re.sub(self._equal, "", direct)
        direct = direct.strip()
        return re.search(direct, original_line, re.IGNORECASE).group(0)

    def _extract_spectrum(self, line):
        output = {}
        if re.search(self._interpolate, line) is not None:
            output.update({user_file_mon_interpolate: True})
            line = re.sub(self._interpolate, "", line)

        if re.search(self._trans, line) is not None:
            key = user_file_mon_spectrum_trans
            line = re.sub(self._trans, "", line)
            line = re.sub("/", "", line)
        else:
            key = user_file_mon_spectrum

        line = re.sub(self._spectrum, "", line)
        line = re.sub(self._equal, "", line)
        output.update({key: convert_string_to_integer(line)})
        return output

    @staticmethod
    def get_type():
        return MonParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + MonParser.get_type() + "(\\s*/\\s*)"


class PrintParser(UserFileComponentParser):
    """
    The PrintParser handles the following structure for
        PRINT string
    """
    Type = "PRINT"

    def __init__(self):
        super(PrintParser, self).__init__()

        # Print
        self._print = "\\s*PRINT\\s+"
        self._print_pattern = re.compile(start_string + self._print + "\\s*.*\\s*" + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, PrintParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        original_setting = re.search(setting.strip(), line, re.IGNORECASE).group(0)
        return {user_file_print: original_setting}

    @staticmethod
    def get_type():
        return PrintParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + PrintParser.get_type() + "(\\s+)"


class SANS2DParser(UserFileComponentParser):
    """
    The BackParser is a hollow parser to ensure backwards compatibility
    """
    Type = "SANS2D"

    def __init__(self):
        super(SANS2DParser, self).__init__()

    def parse_line(self, line):
        return None

    @staticmethod
    def get_type():
        return SANS2DParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + SANS2DParser.get_type() + "(\\s*)"


class LOQParser(UserFileComponentParser):
    """
    The BackParser is a hollow parser to ensure backwards compatibility
    """
    Type = "LOQ"

    def __init__(self):
        super(LOQParser, self).__init__()

    def parse_line(self, line):
        return None

    @staticmethod
    def get_type():
        return LOQParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + LOQParser.get_type() + "(\\s*)"


class UserFileParser(object):
    def __init__(self):
        super(UserFileParser, self).__init__()
        self._parsers = {BackParser.get_type(): BackParser(),
                         DetParser.get_type(): DetParser(),
                         LimitParser.get_type(): LimitParser(),
                         MaskParser.get_type(): MaskParser(),
                         SampleParser.get_type(): SampleParser(),
                         SetParser.get_type(): SetParser(),
                         TransParser.get_type(): TransParser(),
                         TubeCalibFileParser.get_type(): TubeCalibFileParser(),
                         QResolutionParser.get_type(): QResolutionParser(),
                         FitParser.get_type(): FitParser(),
                         GravityParser.get_type(): GravityParser(),
                         MaskFileParser.get_type(): MaskFileParser(),
                         MonParser.get_type(): MonParser(),
                         PrintParser.get_type(): PrintParser(),
                         SANS2DParser.get_type(): SANS2DParser(),
                         LOQParser.get_type(): LOQParser()}

    def _get_correct_parser(self, line):
        line = line.strip()
        line = line.upper()
        for key in self._parsers:
            parser = self._parsers[key]
            if re.match(parser.get_type_pattern(), line, re.IGNORECASE) is not None:
                return parser
            else:
                continue
        # We have encountered an unknown file specifier.
        raise ValueError("UserFileParser: Unknown user "
                         "file command: {0}".format(line))

    def parse_line(self, line):
        # Clean the line of trailing white space
        line = line.strip()

        # Get the appropriate parser
        parser = self._get_correct_parser(line)

        # Parse the line and return the result
        return parser.parse_line(line)
