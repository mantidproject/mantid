# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines, invalid-name, too-many-instance-attributes, too-many-branches, too-few-public-methods

import abc
import re
from math import copysign


from SANS.sans.common.enums import ReductionMode, DetectorType, RangeStepType, FitType, DataType, SANSInstrument
from sans.user_file.settings_tags import (
    DetectorId,
    BackId,
    range_entry,
    back_single_monitor_entry,
    single_entry_with_detector,
    mask_angle_entry,
    LimitsId,
    simple_range,
    complex_range,
    MaskId,
    mask_block,
    mask_block_cross,
    mask_line,
    range_entry_with_detector,
    SampleId,
    SetId,
    set_scales_entry,
    position_entry,
    TransId,
    TubeCalibrationFileId,
    QResolutionId,
    FitId,
    fit_general,
    MonId,
    monitor_length,
    monitor_file,
    GravityId,
    OtherId,
    monitor_spectrum,
    PrintId,
    det_fit_range,
    q_rebin_values,
)


# -----------------------------------------------------------------
# --- Free Functions     ------------------------------------------
# -----------------------------------------------------------------
def convert_string_to_float(to_convert):
    return float(to_convert.strip())


def convert_string_to_integer(to_convert):
    return int(to_convert.strip())


def extract_range(to_extract, converter):
    # Remove leading and trailing whitespace
    to_extract = to_extract.strip()
    # Collapse multiple central whitespaces to a single one
    to_extract = " ".join(to_extract.split())

    entries_string = to_extract.split()
    number_of_entries = len(entries_string)
    if number_of_entries != 2:
        raise RuntimeError("Expected a range defined by two numbers," " but instead received {0}".format(number_of_entries))

    return [converter(entries_string[0]), converter(entries_string[1])]


def extract_float_range(to_extract):
    return extract_range(to_extract, convert_string_to_float)


def extract_int_range(to_extract):
    return extract_range(to_extract, convert_string_to_integer)


def extract_list(to_extract, separator, converter):
    to_extract = to_extract.strip()
    to_extract = " ".join(to_extract.split())
    string_list = [element.replace(" ", "") for element in re.split(separator, to_extract)]
    string_list = [element for element in string_list if element != ""]
    return [converter(element) for element in string_list]


def extract_float_list(to_extract, separator=","):
    return extract_list(to_extract, separator, convert_string_to_float)


def extract_string_list(to_extract, separator=","):
    return extract_list(to_extract, separator, lambda x: x)


def extract_float_range_midpoint_and_steps(to_extract, separator):
    to_extract = " ".join(to_extract.split())

    entries_string = re.split(separator, to_extract)
    entries_string = [element for element in entries_string if element != ""]
    number_of_entries = len(entries_string)
    if number_of_entries != 5:
        raise RuntimeError("Expected a range defined by 5 numbers," " but instead received {0}".format(number_of_entries))

    return [
        convert_string_to_float(entries_string[0]),
        convert_string_to_float(entries_string[1]),
        convert_string_to_float(entries_string[2]),
        convert_string_to_float(entries_string[3]),
        convert_string_to_float(entries_string[4]),
    ]


def does_pattern_match(compiled_regex, line):
    return compiled_regex.match(line) is not None


def escape_special_characters_for_file_path(to_escape):
    escape = {"\a": "\\a", "\b": "\\b", r"\c": "\\c", "\f": "\\f", "\n": "\\n", "\r": "\\r", "\t": "\\t", "\v": "\\v"}
    keys = list(escape.keys())
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
rebin_string_no_comma = "(\\s*[-+]?\\d+(\\.\\d+)?)(\\s*\\s*[-+]?\\d+(\\.\\d+)?)*"


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
        Command | Qualifier    | Parameter
        BACK    / MON/TIMES     t1 t2
        BACK    / M m/TIMES      t1 t2
        BACK    / M m            t1 t2
        BACK    / M m/OFF
        BACK    / TRANS          t1 t2
    """

    Type = "BACK"

    def __init__(self):
        super(BackParser, self).__init__()

        # General
        self._times = "\\s*/\\s*TIMES"

        # All Monitors
        self._all_mons = "\\s*MON\\s*/\\s*TIMES\\s*"
        self._all_mons_pattern = re.compile(
            start_string + self._all_mons + space_string + float_number + space_string + float_number + end_string
        )

        # Single Monitor
        self._mon_id = "M"
        self._single_monitor = "\\s*" + self._mon_id + integer_number + "\\s*"
        self._single_monitor_pattern = re.compile(
            start_string
            + self._single_monitor
            + "(\\s*"
            + self._times
            + "\\s*)?"
            + space_string
            + float_number
            + space_string
            + float_number
            + end_string
        )

        # Off
        self._off_pattern = re.compile(start_string + self._single_monitor + "\\s*/\\s*OFF\\s*" + end_string)

        # Trans
        self._trans = "TRANS"
        self._trans_pattern = re.compile(start_string + self._trans + space_string + float_number + space_string + float_number)

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
        elif self._is_trans(setting):
            output = self._extract_trans(setting)
        else:
            raise RuntimeError("BackParser: Unknown command for BACK: {0}".format(line))
        return output

    def _is_all_mon(self, line):
        return does_pattern_match(self._all_mons_pattern, line)

    def _is_single_mon(self, line):
        return does_pattern_match(self._single_monitor_pattern, line)

    def _is_off(self, line):
        return does_pattern_match(self._off_pattern, line)

    def _is_trans(self, line):
        return does_pattern_match(self._trans_pattern, line)

    def _extract_all_mon(self, line):
        all_mons_string = re.sub(self._all_mons, "", line)
        time_range = extract_float_range(all_mons_string)
        return {BackId.ALL_MONITORS: range_entry(start=time_range[0], stop=time_range[1])}

    def _extract_single_mon(self, line):
        monitor_number = self._get_monitor_number(line)
        single_string = re.sub(self._times, "", line)
        all_mons_string = re.sub(self._single_monitor, "", single_string)
        time_range = extract_float_range(all_mons_string)
        return {BackId.SINGLE_MONITORS: back_single_monitor_entry(monitor=monitor_number, start=time_range[0], stop=time_range[1])}

    def _extract_off(self, line):
        monitor_number = self._get_monitor_number(line)
        return {BackId.MONITOR_OFF: monitor_number}

    def _extract_trans(self, line):
        trans_string = re.sub(self._trans, "", line)
        time_range = extract_float_range(trans_string)
        return {BackId.TRANS: range_entry(start=time_range[0], stop=time_range[1])}

    def _get_monitor_number(self, line):
        monitor_selection = re.search(self._single_monitor, line).group(0)
        monitor_selection = monitor_selection.strip()
        monitor_number_string = re.sub(self._mon_id, "", monitor_selection)
        return convert_string_to_integer(monitor_number_string)

    @staticmethod
    def get_type():
        return BackParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + BackParser.get_type() + "\\s*/\\s*"


class InstrParser(object):
    """
    InstrParser looks to find the instrument.
    Compared to other parsers, this is a naive implementation
    which expects a line in the user file to explicitly state the instrument,
    with no other data.
    Because of this, we are trying to match exact strings, and so do not use regex.
    """

    Type = "INSTR"
    _INSTRUMENTS = ["LOQ", "LARMOR", "SANS2D", "ZOOM", "NOINSTRUMENT"]

    INSTRUMENTS_DICT = {
        "LOQ": SANSInstrument.LOQ,
        "LARMOR": SANSInstrument.LARMOR,
        "SANS2D": SANSInstrument.SANS2D,
        "ZOOM": SANSInstrument.ZOOM,
    }

    @staticmethod
    def parse_line(line):
        try:
            ret_val = InstrParser.INSTRUMENTS_DICT[line]
        except KeyError:
            raise RuntimeError("InstrParser: Unknown command for INSTR: {0}".format(line))
        else:
            # If no exception raised
            return {DetectorId.INSTRUMENT: ret_val}

    @staticmethod
    def get_type():
        return InstrParser.Type

    @staticmethod
    def get_type_pattern(line):
        if line in InstrParser._INSTRUMENTS:
            return True
        else:
            return False


class DetParser(UserFileComponentParser):
    """
    The DetParser handles the following structure
        1) Corrections:
            DET/CORR/FRONT/qualifier [parameter]
            DET/CORR/REAR/qualifier [parameter]
              qualifiers are:
              X , Y, Z, ROT, RADIUS, SIDE, XTILT, YTILT

            Note that illegally the combination DET/CORR FRONT qualifier is accepted by the old ISIS SANS reduction
            code, therefore we need to support it here

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
            DET/OVERLAP [Q1 Q2]
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
        self._translation_pattern = re.compile(start_string + self._translation + space_string + float_number + end_string)
        self._x_tilt = "\\s*XTILT\\s*"
        self._x_tilt_pattern = re.compile(start_string + self._x_tilt + space_string + float_number + end_string)
        self._y_tilt = "\\s*YTILT\\s*"
        self._y_tilt_pattern = re.compile(start_string + self._y_tilt + space_string + float_number + end_string)

        self._radius = "\\s*RADIUS\\s*"
        self._radius_pattern = re.compile(start_string + self._radius + space_string + float_number + end_string)
        self._correction_lab = "\\s*CORR\\s*[/]?\\s*REAR\\s*[/]?\\s*"
        self._correction_hab = "\\s*CORR\\s*[/]?\\s*FRONT\\s*[/]?\\s*"
        self._correction_LAB_pattern = re.compile(start_string + self._correction_lab)
        self._correction_HAB_pattern = re.compile(start_string + self._correction_hab)

        # Merge options
        self._rescale = "\\s*RESCALE\\s*"
        self._rescale_pattern = re.compile(start_string + self._rescale + space_string + float_number + end_string)
        self._shift = "\\s*SHIFT\\s*"
        self._shift_pattern = re.compile(start_string + self._shift + space_string + float_number + end_string)
        self._rescale_fit = "\\s*RESCALE\\s*/\\s*FIT\\s*"
        self._q_range = "\\s*(" + float_number + space_string + float_number + ")?"
        self._rescale_fit_pattern = re.compile(start_string + self._rescale_fit + self._q_range + end_string)
        self._shift_fit = "\\s*SHIFT\\s*/\\s*FIT\\s*"
        self._shift_fit_pattern = re.compile(start_string + self._shift_fit + self._q_range + end_string)
        self._merge_range = "\\s*OVERLAP\\s*"
        self._merge_range_pattern = re.compile(start_string + self._merge_range + self._q_range + end_string)

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
        return does_pattern_match(self._correction_HAB_pattern, line) or does_pattern_match(self._correction_LAB_pattern, line)

    def _is_merge_option_setting(self, line):
        return (
            does_pattern_match(self._rescale_pattern, line)
            or does_pattern_match(self._shift_pattern, line)
            or does_pattern_match(self._rescale_fit_pattern, line)
            or does_pattern_match(self._shift_fit_pattern, line)
            or does_pattern_match(self._merge_range_pattern, line)
        )

    def _extract_reduction_mode(self, line):
        line_capital = line.upper()
        if line_capital in self._HAB:
            return {DetectorId.REDUCTION_MODE: ReductionMode.HAB}
        elif line_capital in self._LAB:
            return {DetectorId.REDUCTION_MODE: ReductionMode.LAB}
        elif line_capital in self._BOTH:
            return {DetectorId.REDUCTION_MODE: ReductionMode.ALL}
        elif line_capital in self._MERGE:
            return {DetectorId.REDUCTION_MODE: ReductionMode.MERGED}
        else:
            raise RuntimeError("DetParser:  Could not extract line: {0}".format(line))

    def _extract_correction(self, line):
        if self._correction_HAB_pattern.match(line) is not None:
            qualifier = re.sub(self._correction_hab, "", line)
            qualifier = qualifier.strip()
            return self._extract_detector_setting(qualifier, DetectorType.HAB)
        elif self._correction_LAB_pattern.match(line) is not None:
            qualifier = re.sub(self._correction_lab, "", line)
            qualifier = qualifier.strip()
            return self._extract_detector_setting(qualifier, DetectorType.LAB)
        else:
            raise RuntimeError("DetParser: Could not extract line: {0}".format(line))

    def _extract_detector_setting(self, qualifier, detector_type):
        if self._x_pattern.match(qualifier):
            value_string = re.sub(self._x, "", qualifier)
            key = DetectorId.CORRECTION_X
        elif self._y_pattern.match(qualifier):
            value_string = re.sub(self._y, "", qualifier)
            key = DetectorId.CORRECTION_Y
        elif self._z_pattern.match(qualifier):
            value_string = re.sub(self._z, "", qualifier)
            key = DetectorId.CORRECTION_Z
        elif self._rotation_pattern.match(qualifier):
            value_string = re.sub(self._rotation, "", qualifier)
            key = DetectorId.CORRECTION_ROTATION
        elif self._translation_pattern.match(qualifier):
            value_string = re.sub(self._translation, "", qualifier)
            key = DetectorId.CORRECTION_TRANSLATION
        elif self._radius_pattern.match(qualifier):
            value_string = re.sub(self._radius, "", qualifier)
            key = DetectorId.CORRECTION_RADIUS
        elif self._x_tilt_pattern.match(qualifier):
            value_string = re.sub(self._x_tilt, "", qualifier)
            key = DetectorId.CORRECTION_X_TILT
        elif self._y_tilt_pattern.match(qualifier):
            value_string = re.sub(self._y_tilt, "", qualifier)
            key = DetectorId.CORRECTION_Y_TILT
        else:
            raise RuntimeError("DetParser: Unknown qualifier encountered: {0}".format(qualifier))

        # Qualify the key with the selected detector
        value_string = value_string.strip()
        value = convert_string_to_float(value_string)
        return {key: single_entry_with_detector(entry=value, detector_type=detector_type)}

    def _extract_merge_option(self, line):
        if self._rescale_pattern.match(line) is not None:
            rescale_string = re.sub(self._rescale, "", line)
            rescale = convert_string_to_float(rescale_string)
            return {DetectorId.RESCALE: rescale}
        elif self._shift_pattern.match(line) is not None:
            shift_string = re.sub(self._shift, "", line)
            shift = convert_string_to_float(shift_string)
            return {DetectorId.SHIFT: shift}
        elif self._rescale_fit_pattern.match(line) is not None:
            rescale_fit_string = re.sub(self._rescale_fit, "", line)
            if rescale_fit_string:
                rescale_fit = extract_float_range(rescale_fit_string)
                value = det_fit_range(start=rescale_fit[0], stop=rescale_fit[1], use_fit=True)
            else:
                value = det_fit_range(start=None, stop=None, use_fit=True)
            return {DetectorId.RESCALE_FIT: value}
        elif self._shift_fit_pattern.match(line) is not None:
            shift_fit_string = re.sub(self._shift_fit, "", line)
            if shift_fit_string:
                shift_fit = extract_float_range(shift_fit_string)
                value = det_fit_range(start=shift_fit[0], stop=shift_fit[1], use_fit=True)
            else:
                value = det_fit_range(start=None, stop=None, use_fit=True)
            return {DetectorId.SHIFT_FIT: value}
        elif self._merge_range_pattern.match(line) is not None:
            merge_range_string = re.sub(self._merge_range, "", line)
            if merge_range_string:
                merge_range = extract_float_range(merge_range_string)
                value = det_fit_range(start=merge_range[0], stop=merge_range[1], use_fit=True)
            else:
                raise RuntimeError("DetParser: Could not extract line: {0}".format(line))
            return {DetectorId.MERGE_RANGE: value}
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
        L/Q q1 dq1 q2 dq2 q3 [/LIN]]  or  L/Q q1 dq1 q2 dq2 q3 [/LOG]]
        but apparently also L/Q q1 dq1 q2 dq2 q3 dq3 ... [/LOG | /LIN] is allowed

        L/Q/RCut c
        L/Q/WCut c

        L/QXY qxy1 qxy2 [dqxy[/LIN]]  or  L/QXY qxy1 qxy2 [dqxy[/LOG]]
        L/QXY qxy1 dqxy1 qxy3 dqxy2 qxy2 [/LIN]]  or  L/QXY qxy1 dqxy1 qxy3 dqxy2 qxy2 [/LOG]]

        L/R r1 r2  or undocumented L/R  r1 r2 step where step is actually ignored

        L/WAV l1 l2 [dl[/LIN]  or  L/WAV l1 l2 [dl[/LOG]
        L/WAV l1 dl1 l3 dl2 l2 [/LIN]  or  L/WAV l1 dl1 l3 dl2 l2 [/LOG]

        L/EVENTSTIME rebin_str

    Note that the docs state that all limit strings should be space-separated, however complex ranges
    used to be comma-separated ONLY. They remain as such so existing user files are not broken.
    We replace commas present in a string with spaces so that all inputs are effectively space-separated.
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

        self._comma = "\\s*,\\s*"
        self._complex_range = (
            "\\s*"
            + float_number
            + space_string
            + float_number
            + space_string
            + float_number
            + space_string
            + float_number
            + space_string
            + float_number
            + "(\\s*"
            + self._lin_or_log
            + ")?"
        )
        # The complex pattern is normally a rebin string, such as

        self._complex_range_2 = "\\s*" + float_number + "(" + space_string + float_number + ")*\\s*" + "(\\s*" + self._lin_or_log + ")?"

        # Angle limits
        self._phi_no_mirror = "\\s*/\\s*NOMIRROR\\s*"
        self._phi = "\\s*PHI\\s*(" + self._phi_no_mirror + ")?\\s*"
        self._phi_pattern = re.compile(start_string + self._phi + space_string + float_number + space_string + float_number + end_string)

        # Event time limits
        self._events_time = "\\s*EVENTSTIME\\s*"
        self._events_time_pattern = re.compile(start_string + self._events_time + space_string + rebin_string_no_comma + end_string)

        self._events_time_pattern_simple_pattern = re.compile(
            start_string + self._events_time + space_string + self._simple_range + end_string
        )

        # Q Limits
        self._q = "\\s*Q\\s*"
        self._q_simple_pattern = re.compile(start_string + self._q + space_string + self._simple_range + end_string)

        self._q_complex_pattern = re.compile(start_string + self._q + space_string + self._complex_range + end_string)
        self._q_complex_pattern_2 = re.compile(start_string + self._q + space_string + self._complex_range_2 + end_string)

        # Qxy limits
        self._qxy = "\\s*QXY\\s*"
        self._qxy_simple_pattern = re.compile(start_string + self._qxy + space_string + self._simple_range + end_string)
        self._qxy_complex_pattern = re.compile(start_string + self._qxy + space_string + self._complex_range + end_string)

        # Wavelength limits
        self._wavelength = "\\s*WAV\\s*"
        self._wavelength_simple_pattern = re.compile(start_string + self._wavelength + space_string + self._simple_range + end_string)
        self._wavelength_complex_pattern = re.compile(start_string + self._wavelength + space_string + self._complex_range + end_string)

        # Cut limits
        self._radius_cut = "\\s*Q\\s*/\\s*RCUT\\s*"
        self._radius_cut_pattern = re.compile(start_string + self._radius_cut + space_string + float_number + end_string)
        self._wavelength_cut = "\\s*Q\\s*/\\s*WCUT\\s*"
        self._wavelength_cut_pattern = re.compile(start_string + self._wavelength_cut + space_string + float_number + end_string)

        # Radius limits
        # Note that we have to account for an undocumented potential step size (which is ignored
        self._radius = "\\s*R\\s*"
        self._radius_string = (
            start_string
            + self._radius
            + space_string
            + float_number
            + space_string
            + float_number
            + "\\s*("
            + float_number
            + ")?\\s*"
            + end_string
        )
        self._radius_pattern = re.compile(self._radius_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, LimitParser.get_type_pattern())
        setting = setting.replace(",", " ")
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
        return does_pattern_match(self._events_time_pattern, line) or does_pattern_match(self._events_time_pattern_simple_pattern, line)

    def _is_cut_limit(self, line):
        return does_pattern_match(self._radius_cut_pattern, line) or does_pattern_match(self._wavelength_cut_pattern, line)

    def _is_radius_limit(self, line):
        return does_pattern_match(self._radius_pattern, line)

    def _is_q_limit(self, line):
        return (
            does_pattern_match(self._q_simple_pattern, line)
            or does_pattern_match(self._q_complex_pattern, line)
            or self._does_match_complex_pattern2(line)
        )

    def _is_qxy_limit(self, line):
        return does_pattern_match(self._qxy_simple_pattern, line) or does_pattern_match(self._qxy_complex_pattern, line)

    def _is_wavelength_limit(self, line):
        return does_pattern_match(self._wavelength_simple_pattern, line) or does_pattern_match(self._wavelength_complex_pattern, line)

    def _does_match_complex_pattern2(self, line):
        pattern_matches = does_pattern_match(self._q_complex_pattern_2, line)
        if pattern_matches:
            # We have to make sure that there is an odd number of elements
            range_with_steps_string = re.sub(self._q, "", line)
            range_with_steps_string = re.sub(self._lin_or_log, "", range_with_steps_string)
            range_with_steps = extract_float_list(range_with_steps_string, " ")
            pattern_matches = len(range_with_steps) > 5 and (len(range_with_steps) % 2 == 1)
        return pattern_matches

    def _extract_angle_limit(self, line):
        use_mirror = re.search(self._phi_no_mirror, line) is None
        angles_string = re.sub(self._phi, "", line)
        angles = extract_float_range(angles_string)
        return {LimitsId.ANGLE: mask_angle_entry(min=angles[0], max=angles[1], use_mirror=use_mirror)}

    def _extract_event_binning(self, line):
        event_binning = re.sub(self._events_time, "", line)
        if does_pattern_match(self._events_time_pattern, line):
            rebin_values = extract_float_list(event_binning, separator=" ")
            binning_string = ",".join([str(val) for val in rebin_values])
        else:
            simple_pattern = self._extract_simple_pattern(event_binning, LimitsId.EVENTS_BINNING)
            rebin_values = simple_pattern[LimitsId.EVENTS_BINNING]
            prefix = -1.0 if rebin_values.step_type is RangeStepType.LOG else 1.0
            binning_string = str(rebin_values.start) + "," + str(prefix * rebin_values.step) + "," + str(rebin_values.stop)

        output = {LimitsId.EVENTS_BINNING: binning_string}
        return output

    def _extract_cut_limit(self, line):
        if self._radius_cut_pattern.match(line) is not None:
            key = LimitsId.RADIUS_CUT
            limit_value = re.sub(self._radius_cut, "", line)
        else:
            key = LimitsId.WAVELENGTH_CUT
            limit_value = re.sub(self._wavelength_cut, "", line)
        return {key: convert_string_to_float(limit_value)}

    def _extract_radius_limit(self, line):
        radius_range_string = re.sub(self._radius, "", line)
        radius_range = extract_float_list(radius_range_string, separator=" ")
        return {LimitsId.RADIUS: range_entry(start=radius_range[0], stop=radius_range[1])}

    def _extract_q_limit(self, line):
        q_range = re.sub(self._q, "", line)
        if does_pattern_match(self._q_simple_pattern, line):
            simple_output = self._extract_simple_pattern(q_range, LimitsId.Q)
            simple_output = simple_output[LimitsId.Q]
            prefix = -1.0 if simple_output.step_type is RangeStepType.LOG else 1.0
            q_limit_output = [simple_output.start]
            if simple_output.step:
                q_limit_output.append(prefix * simple_output.step)
            q_limit_output.append(simple_output.stop)
        elif does_pattern_match(self._q_complex_pattern, line):
            complex_output = self._extract_complex_pattern(q_range, LimitsId.Q)
            complex_output = complex_output[LimitsId.Q]
            prefix1 = -1.0 if complex_output.step_type1 is RangeStepType.LOG else 1.0
            prefix2 = -1.0 if complex_output.step_type2 is RangeStepType.LOG else 1.0
            q_limit_output = [
                complex_output.start,
                prefix1 * complex_output.step1,
                complex_output.mid,
                prefix2 * complex_output.step2,
                complex_output.stop,
            ]
        else:
            q_limit_output = self._extract_complex_pattern2(q_range)

        # The output is a q_rebin_values object with q_min, q_max and the rebin string.
        rebinning_string = ",".join([str(element) for element in q_limit_output])
        q_rebin = q_rebin_values(min=q_limit_output[0], max=q_limit_output[-1], rebin_string=rebinning_string)
        output = {LimitsId.Q: q_rebin}
        return output

    def _extract_qxy_limit(self, line):
        qxy_range = re.sub(self._qxy, "", line)
        if does_pattern_match(self._qxy_simple_pattern, line):
            output = self._extract_simple_pattern(qxy_range, LimitsId.QXY)
        else:
            # v2 GUI cannot currently support complex QXY ranges
            # output = self._extract_complex_pattern(qxy_range, LimitsId.qxy)
            raise ValueError("QXY Limits: The expression {0} is currently not supported." " Use a simple pattern".format(line))
        return output

    def _extract_wavelength_limit(self, line):
        wavelength_range = re.sub(self._wavelength, "", line)
        if does_pattern_match(self._wavelength_simple_pattern, line):
            output = self._extract_simple_pattern(wavelength_range, LimitsId.WAVELENGTH)
        else:
            # This is not implemented in the old parser, hence disable here
            # output = self._extract_complex_pattern(wavelength_range, LimitsId.wavelength)
            raise ValueError("Wavelength Limits: The expression {0} is currently not supported." " Use a simple pattern.".format(line))
        return output

    def _extract_simple_pattern(self, simple_range_input, tag):
        if re.sub(self._range, "", simple_range_input, 1) == "":
            float_range = extract_float_range(simple_range_input)
            output = {tag: simple_range(start=float_range[0], stop=float_range[1], step=None, step_type=None)}
        else:
            # Extract the step information
            range_removed = re.sub(self._range, "", simple_range_input, 1)

            # Get the step type
            step_type = self._get_step_type(range_removed)

            # Get the step
            step_string = re.sub(self._lin_or_log, "", range_removed)
            step = convert_string_to_float(step_string)

            # Get the range
            pure_range = re.sub(range_removed, "", simple_range_input)
            float_range = extract_float_range(pure_range)
            output = {tag: simple_range(start=float_range[0], stop=float_range[1], step=step, step_type=step_type)}
        return output

    def _extract_complex_pattern(self, complex_range_input, tag):
        # Get the step type
        step_type = self._get_step_type(complex_range_input, default=None)

        # Remove the step type
        range_with_steps_string = re.sub(self._lin_or_log, "", complex_range_input)
        range_with_steps = extract_float_range_midpoint_and_steps(range_with_steps_string, " ")

        # Check if there is a sign on the individual steps, this shows if something had been marked as linear or log.
        # If there is an explicit LOG/LIN command, then this overwrites the sign
        step_type1 = RangeStepType.LOG if copysign(1, range_with_steps[1]) == -1 else RangeStepType.LIN
        step_type2 = RangeStepType.LOG if copysign(1, range_with_steps[3]) == -1 else RangeStepType.LIN
        if step_type is not None:
            step_type1 = step_type
            step_type2 = step_type

        return {
            tag: complex_range(
                start=range_with_steps[0],
                step1=abs(range_with_steps[1]),
                mid=range_with_steps[2],
                step2=abs(range_with_steps[3]),
                stop=range_with_steps[4],
                step_type1=step_type1,
                step_type2=step_type2,
            )
        }

    def _extract_complex_pattern2(self, complex_range_input):
        # Get the step type
        step_type = self._get_step_type(complex_range_input, default=None)

        # Remove the step type
        range_with_steps_string = re.sub(self._lin_or_log, "", complex_range_input)
        range_with_steps = extract_float_list(range_with_steps_string, " ")

        if step_type is not None:
            prefix = -1.0 if step_type is RangeStepType.LOG else 1.0
            for index in range(1, len(range_with_steps), 2):
                range_with_steps[index] *= prefix
        return range_with_steps

    def _get_step_type(self, range_string, default=RangeStepType.LIN):
        range_type = default
        if re.search(self._log, range_string):
            range_type = RangeStepType.LOG
        elif re.search(self._lin, range_string):
            range_type = RangeStepType.LIN
        return range_type

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
        MASK[/REAR/FRONT/HAB] Hn>Hm+Vn>Vm or Vn>Vm+Hn>Hm - to mask a rectangular 'box'
        MASK[/REAR/FRONT/HAB] Hn+Vm or Vm+Hn  - to mask the intersection of Hn and Vm

        MASK Ssp1[>Ssp2]

        MASK[/REAR/FRONT/HAB]/TIME t1 t2 or  MASK[/REAR/FRONT/HAB]/T t1 t2 - if no detector is specified, then mask
                                                                             is applied to both detectors.

        MASK/LINE width angle [x y]
    """

    Type = "MASK"

    def __init__(self):
        super(MaskParser, self).__init__()
        self._time = "\\s*/\\s*TIME\\s*"

        # ranges
        self._two_floats = "\\s*" + float_number + space_string + float_number + "\\s*"
        self._optional_two_floats = "(\\s+" + self._two_floats + "\\s*)?\\s*"
        self._range = "\\s*>\\s*"

        # Line Mask
        self._line = "\\s*LINE\\s*"
        self._line_pattern = re.compile(
            start_string + self._line + space_string + self._two_floats + self._optional_two_floats + end_string
        )

        # Clear Mask
        self._clear = "\\s*CLEAR\\s*"
        self._clear_pattern = re.compile(start_string + self._clear + "\\s*(" + self._time + ")?" + end_string)

        # Spectrum Mask
        self._spectrum = "\\s*S\\s*"
        self._additional_spectrum = "(\\s*>" + self._spectrum + integer_number + ")"
        self._spectrum_range_pattern = re.compile(start_string + self._spectrum + integer_number + self._additional_spectrum + end_string)
        self._spectrum_single_pattern = re.compile(start_string + self._spectrum + integer_number + end_string)

        # Strip Masks
        self._hab = "\\s*HAB|FRONT\\s*"
        self._lab = "\\s*LAB|REAR|MAIN\\s*"
        self._detector = "\\s*(" + self._hab + "|" + self._lab + ")?\\s*"

        # Vertical strip Mask
        self._v = "\\s*V\\s*"
        self._additional_v = "(\\s*>" + self._v + integer_number + ")"
        self._single_vertical_strip_pattern = re.compile(start_string + self._detector + self._v + integer_number + end_string)
        self._range_vertical_strip_pattern = re.compile(
            start_string + self._detector + self._v + integer_number + self._additional_v + end_string
        )

        # Horizontal strip Mask
        self._h = "\\s*H\\s*"
        self._additional_h = "(\\s*>" + self._h + integer_number + ")"
        self._single_horizontal_strip_pattern = re.compile(start_string + self._detector + self._h + integer_number + end_string)
        self._range_horizontal_strip_pattern = re.compile(
            start_string + self._detector + self._h + integer_number + self._additional_h + end_string
        )

        # Time Mask
        self._time_or_t = "\\s*(TIME|T)\\s*"
        self._detector_time = "\\s*((" + self._hab + "|" + self._lab + ")" + "\\s*)?\\s*"
        self._time_pattern = re.compile(
            start_string
            + self._detector_time
            + "/?"
            + self._time_or_t
            + space_string
            + self._two_floats
            + end_string
            + "|"
            + start_string
            + self._time_or_t
            + "/?"
            + self._detector_time
            + space_string
            + self._two_floats
            + end_string
        )

        # Block mask
        self._v_plus_h = "\\s*" + self._v + integer_number + "\\s*\\+\\s*" + self._h + integer_number
        self._h_plus_v = "\\s*" + self._h + integer_number + "\\s*\\+\\s*" + self._v + integer_number

        self._vv_plus_hh = self._v + integer_number + self._additional_v + "\\s*\\+\\s*" + self._h + integer_number + self._additional_h
        self._hh_plus_vv = self._h + integer_number + self._additional_h + "\\s*\\+\\s*" + self._v + integer_number + self._additional_v

        self._blocks = "\\s*(" + self._v_plus_h + "|" + self._h_plus_v + "|" + self._vv_plus_hh + "|" + self._hh_plus_vv + ")\\s*"
        self._block_pattern = re.compile(start_string + self._detector + self._blocks + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, MaskParser.get_type_pattern())
        # Determine the qualifier and extract the user setting
        if self._is_block_mask(setting):
            output = self._extract_block_mask(setting)
        elif self._is_line_mask(setting):
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

    def _is_block_mask(self, line):
        return does_pattern_match(self._block_pattern, line)

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

    def _extract_block_mask(self, line):
        # There are four cases that can exist:
        # 1. Va > Vb + Hc > Hd
        # 2. Ha > Hb + Vc > Vd
        # 3. Va + Hb
        # 4. Ha + Vb
        # Record and remove detector type
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        block_string = re.sub(self._detector, "", line)
        is_true_block = ">" in block_string
        two_blocks = block_string.split("+")
        horizontal_part = None
        vertical_part = None
        if is_true_block:
            for block in two_blocks:
                if self._is_vertical_range_strip_mask(block):
                    prelim_range = self._extract_vertical_range_strip_mask(block)
                    # Note we use the lab key word since the extraction defaults to lab
                    vertical_part = prelim_range[MaskId.VERTICAL_RANGE_STRIP_MASK]
                elif self._is_horizontal_range_strip_mask(block):
                    prelim_range = self._extract_horizontal_range_strip_mask(block)
                    # Note we use the lab key word since the extraction defaults to lab
                    horizontal_part = prelim_range[MaskId.HORIZONTAL_RANGE_STRIP_MASK]
                else:
                    raise RuntimeError("MaskParser: Cannot handle part of block mask: {0}".format(block))
            # Now that we have both parts we can assemble the output
            output = {
                MaskId.BLOCK: mask_block(
                    horizontal1=horizontal_part.start,
                    horizontal2=horizontal_part.stop,
                    vertical1=vertical_part.start,
                    vertical2=vertical_part.stop,
                    detector_type=detector_type,
                )
            }
        else:
            for block in two_blocks:
                if self._is_vertical_single_strip_mask(block):
                    prelim_single = self._extract_vertical_single_strip_mask(block)
                    # Note we use the lab key word since the extraction defaults to lab
                    vertical_part = prelim_single[MaskId.VERTICAL_SINGLE_STRIP_MASK]
                elif self._is_horizontal_single_strip_mask(block):
                    prelim_single = self._extract_horizontal_single_strip_mask(block)
                    # Note we use the lab key word since the extraction defaults to lab
                    horizontal_part = prelim_single[MaskId.HORIZONTAL_SINGLE_STRIP_MASK]
                else:
                    raise RuntimeError("MaskParser: Cannot handle part of block cross mask: {0}".format(block))
            output = {
                MaskId.BLOCK_CROSS: mask_block_cross(
                    horizontal=horizontal_part.entry, vertical=vertical_part.entry, detector_type=detector_type
                )
            }
        return output

    def _extract_line_mask(self, line):
        line_string = re.sub(self._line, "", line)
        line_values = extract_float_list(line_string, " ")
        length_values = len(line_values)
        if length_values == 2:
            output = {MaskId.LINE: mask_line(width=line_values[0], angle=line_values[1], x=None, y=None)}
        elif length_values == 4:
            output = {MaskId.LINE: mask_line(width=line_values[0], angle=line_values[1], x=line_values[2], y=line_values[3])}
        else:
            raise ValueError(
                "MaskParser: Line mask accepts wither 2 or 4 parameters," " but {0} parameters were passed in.".format(length_values)
            )
        return output

    def _extract_time_mask(self, line):
        # Check if one of the detectors is found
        has_hab = re.search(self._hab, line)
        has_lab = re.search(self._lab, line)
        if has_hab is not None or has_lab is not None:
            key = MaskId.TIME_DETECTOR
            detector_type = DetectorType.HAB if has_hab is not None else DetectorType.LAB
            regex_string = r"\s*(" + self._hab + r")\s*" if has_hab else r"\s*(" + self._lab + r")\s*"
            min_and_max_time_range = re.sub(regex_string, "", line)
        else:
            key = MaskId.TIME
            detector_type = None
            min_and_max_time_range = line
        min_and_max_time_range = re.sub(r"\s*/\s*", "", min_and_max_time_range)
        min_and_max_time_range = re.sub(self._time_or_t, "", min_and_max_time_range)
        min_and_max_time = extract_float_range(min_and_max_time_range)
        return {key: range_entry_with_detector(start=min_and_max_time[0], stop=min_and_max_time[1], detector_type=detector_type)}

    def _extract_clear_mask(self, line):
        clear_removed = re.sub(self._clear, "", line)
        return {MaskId.CLEAR_DETECTOR_MASK: True} if clear_removed == "" else {MaskId.CLEAR_TIME_MASK: True}

    def _extract_single_spectrum_mask(self, line):
        single_spectrum_string = re.sub(self._spectrum, "", line)
        single_spectrum = convert_string_to_integer(single_spectrum_string)
        return {MaskId.SINGLE_SPECTRUM_MASK: single_spectrum}

    def _extract_spectrum_range_mask(self, line):
        spectrum_range_string = re.sub(self._spectrum, "", line)
        spectrum_range_string = re.sub(self._range, " ", spectrum_range_string)
        spectrum_range = extract_int_range(spectrum_range_string)
        return {MaskId.SPECTRUM_RANGE_MASK: range_entry(start=spectrum_range[0], stop=spectrum_range[1])}

    def _extract_vertical_single_strip_mask(self, line):
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        single_vertical_strip_string = re.sub(self._detector, "", line)
        single_vertical_strip_string = re.sub(self._v, "", single_vertical_strip_string)
        single_vertical_strip = convert_string_to_integer(single_vertical_strip_string)
        return {MaskId.VERTICAL_SINGLE_STRIP_MASK: single_entry_with_detector(entry=single_vertical_strip, detector_type=detector_type)}

    def _extract_vertical_range_strip_mask(self, line):
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        range_vertical_strip_string = re.sub(self._detector, "", line)
        range_vertical_strip_string = re.sub(self._v, "", range_vertical_strip_string)
        range_vertical_strip_string = re.sub(self._range, " ", range_vertical_strip_string)
        range_vertical_strip = extract_int_range(range_vertical_strip_string)
        return {
            MaskId.VERTICAL_RANGE_STRIP_MASK: range_entry_with_detector(
                start=range_vertical_strip[0], stop=range_vertical_strip[1], detector_type=detector_type
            )
        }

    def _extract_horizontal_single_strip_mask(self, line):
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        single_horizontal_strip_string = re.sub(self._detector, "", line)
        single_horizontal_strip_string = re.sub(self._h, "", single_horizontal_strip_string)
        single_horizontal_strip = convert_string_to_integer(single_horizontal_strip_string)
        return {MaskId.HORIZONTAL_SINGLE_STRIP_MASK: single_entry_with_detector(entry=single_horizontal_strip, detector_type=detector_type)}

    def _extract_horizontal_range_strip_mask(self, line):
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        range_horizontal_strip_string = re.sub(self._detector, "", line)
        range_horizontal_strip_string = re.sub(self._h, "", range_horizontal_strip_string)
        range_horizontal_strip_string = re.sub(self._range, " ", range_horizontal_strip_string)
        range_horizontal_strip = extract_int_range(range_horizontal_strip_string)
        return {
            MaskId.HORIZONTAL_RANGE_STRIP_MASK: range_entry_with_detector(
                start=range_horizontal_strip[0], stop=range_horizontal_strip[1], detector_type=detector_type
            )
        }

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
        return {SampleId.PATH: value}

    def _extract_sample_offset(self, line):
        offset_string = re.sub(self._offset, "", line)
        offset = convert_string_to_float(offset_string)
        return {SampleId.OFFSET: offset}

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

        An undocumented feature is:
        SET CENTRE[/MAIN|/HAB] x y [d1 d2]
        where d1 and d2 are pixel sizes. This is not used in the old parser, but user files have it nonetheless.

        SET SCALES s a b c d
    """

    Type = "SET"

    def __init__(self):
        super(SetParser, self).__init__()

        # Scales
        self._scales = "\\s*SCALES\\s*"
        self._scales_pattern = re.compile(
            start_string
            + self._scales
            + space_string
            + float_number
            + space_string
            + float_number
            + space_string
            + float_number
            + space_string
            + float_number
            + space_string
            + float_number
            + end_string
        )

        # Centre
        self._centre = "\\s*CENTRE\\s*"
        self._hab = "\\s*(HAB|FRONT)\\s*"
        self._lab = "\\s*(LAB|REAR|MAIN)\\s*"
        self._hab_or_lab = "\\s*(/" + self._hab + "|/" + self._lab + ")\\s*"
        self._centre_pattern = re.compile(
            start_string
            + self._centre
            + "\\s*(/"
            + self._lab
            + space_string
            + ")?\\s*"
            + float_number
            + space_string
            + float_number
            + "\\s*("
            + space_string
            + float_number
            + space_string
            + float_number
            + ")?\\s*"
            + end_string
        )
        self._centre_pattern_HAB = re.compile(
            start_string
            + self._centre
            + "\\s*(/"
            + self._hab
            + space_string
            + ")?\\s*"
            + float_number
            + space_string
            + float_number
            + "\\s*("
            + space_string
            + float_number
            + space_string
            + float_number
            + ")?\\s*"
            + end_string
        )

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, SetParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_scales(setting):
            output = self._extract_scales(setting)
        elif self._is_centre(setting):
            output = self._extract_centre(setting)
        elif self._is_centre_HAB(setting):
            output = self._extract_centre_HAB(setting)
        else:
            raise RuntimeError("SetParser: Unknown command for SET: {0}".format(line))
        return output

    def _is_scales(self, line):
        return does_pattern_match(self._scales_pattern, line)

    def _is_centre(self, line):
        return does_pattern_match(self._centre_pattern, line)

    def _is_centre_HAB(self, line):
        return does_pattern_match(self._centre_pattern_HAB, line)

    def _extract_scales(self, line):
        scales_string = re.sub(self._scales, "", line)
        scales = extract_float_list(scales_string, separator=" ")
        if len(scales) != 5:
            raise ValueError("SetParser: Expected 5 entries for the SCALES setting, but got {0}.".format(len(scales)))
        return {SetId.SCALES: set_scales_entry(s=scales[0], a=scales[1], b=scales[2], c=scales[3], d=scales[4])}

    def _extract_centre(self, line):
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        centre_string = re.sub(self._centre, "", line)
        centre_string = re.sub("/" + self._lab, "", centre_string)
        centre_string = " ".join(centre_string.split())
        centre = extract_float_list(centre_string, separator=" ")
        return {SetId.CENTRE: position_entry(pos1=centre[0], pos2=centre[1], detector_type=detector_type)}

    def _extract_centre_HAB(self, line):
        detector_type = DetectorType.HAB if re.search(self._hab, line) is not None else DetectorType.LAB
        centre_string = re.sub(self._centre, "", line)
        centre_string = re.sub("/" + self._hab, "", centre_string)
        centre_string = " ".join(centre_string.split())
        centre = extract_float_list(centre_string, separator=" ")
        return {SetId.CENTRE_HAB: position_entry(pos1=centre[0], pos2=centre[1], detector_type=detector_type)}

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
        TRANS/TRANSPEC=4[/SHIFT=z]
        TRANS/RADIUS=r
        TRANS/ROI=roi_mask.xml
        TRANS/MASK=mask.xml
    """

    Type = "TRANS"

    mon_shift_pattern = "SHIFT"

    def __init__(self):
        super(TransParser, self).__init__()
        # General
        self._single_file = "[\\w]+(\\.XML)"
        self._multiple_files = self._single_file + "(,\\s*" + self._single_file + ")*\\s*"
        self._workspace = "[\\w]+"

        # Trans Spec
        self._trans_spec = "\\s*TRANSPEC\\s*=\\s*"
        self._trans_spec_pattern = re.compile(start_string + self._trans_spec + integer_number + end_string)

        # Trans Spec Shift
        self._shift = "\\s*/\\s*SHIFT\\s*=\\s*"
        self._trans_spec_shift_pattern = re.compile(
            start_string + self._trans_spec + integer_number + self._shift + float_number + end_string
        )

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
        self._can_workspace_pattern = re.compile(start_string + self._can_workspace + self._workspace + end_string)
        # SampleWS
        self._sample_workspace = "\\s*SAMPLEWS\\s*=\\s*"
        self._sample_workspace_pattern = re.compile(start_string + self._sample_workspace + self._workspace + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, TransParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_trans_spec(setting):
            output = self._extract_trans_spec(setting)
        elif self._is_trans_spec_shift(setting):
            output = self._extract_trans_spec_shift(setting)
        elif self._is_mon_shift(setting):
            output = self._extract_mon_shift(setting)
        elif self._is_radius(setting):
            output = self._extract_radius(setting)
        elif self._is_roi(setting):
            # Note that we need the original line in order to extract the case sensitive meaning
            output = self._extract_roi(setting, line)
        elif self._is_mask(setting):
            # Note that we need the original line in order to extract the case sensitive meaning
            output = self._extract_mask(setting, line)
        elif self._is_sample_workspace(setting):
            # Note that we need the original line in order to extract the case sensitive meaning
            output = self._extract_sample_workspace(setting, line)
        elif self._is_can_workspace(setting):
            # Note that we need the original line in order to extract the case sensitive meaning
            output = self._extract_can_workspace(setting, line)
        else:
            raise RuntimeError("TransParser: Unknown command for TRANS: {0}".format(line))
        return output

    def _is_trans_spec(self, line):
        return does_pattern_match(self._trans_spec_pattern, line)

    def _is_mon_shift(self, line):
        upper_line = line.upper().strip()
        if self.mon_shift_pattern not in upper_line:
            return False

        # Check there are no other chars such as transpec
        stripped_line = upper_line.replace(self.mon_shift_pattern, "")
        return not any(c.isalpha() for c in stripped_line)

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

    def _extract_mon_shift(self, line):
        converted = line.upper().strip().replace(self.mon_shift_pattern, "").replace("=", "")
        # We should have some form of 'x y'
        assert (c.isDigit() for c in converted.replace(" ", ""))
        split_vars = [i for i in converted.split(" ") if i.strip() != ""]

        if len(split_vars) != 2:
            raise RuntimeError("The line TRANS/{0} contains incorrect parameters", line)

        dist, monitor = int(split_vars[0]), int(split_vars[1])

        if monitor == 5:
            return {TransId.SPEC_5_SHIFT: dist}
        elif monitor >= 0:
            # Some instruments (i.e. LOQ) do not have monitor 4 on spectrum 4, as ZOOM
            # is currently the only one with monitor 5 at spectrum 5 we can make it an edge case
            # If a future instrument wants to use monitor 5 at a different spectrum number or
            # uses monitor 4 at spectrum 5 this should be updated
            return {TransId.SPEC_4_SHIFT: dist}
        else:
            raise RuntimeError("Monitor {0} cannot be shifted".format(monitor))

    def _extract_trans_spec(self, line):
        trans_spec_string = re.sub(self._trans_spec, "", line)
        trans_spec = convert_string_to_integer(trans_spec_string)
        return {TransId.SPEC: trans_spec}

    def _extract_trans_spec_shift(self, line):
        # Get the transpec
        trans_spec_string = re.sub(self._trans_spec, "", line)
        to_remove = re.compile(self._shift + float_number)
        trans_spec_string = re.sub(to_remove, "", trans_spec_string)
        trans_spec_string = re.sub(" ", "", trans_spec_string)
        trans_spec = int(trans_spec_string)

        # Get the shift
        to_remove = re.compile(self._trans_spec + integer_number)
        trans_spec_shift_string = re.sub(to_remove, "", line)
        trans_spec_shift_string = re.sub(self._shift, "", trans_spec_shift_string)
        trans_spec_shift_string = re.sub(" ", "", trans_spec_shift_string)
        trans_spec_shift = convert_string_to_float(trans_spec_shift_string)

        if trans_spec == 5:
            return {TransId.SPEC_5_SHIFT: trans_spec_shift, TransId.SPEC: trans_spec}
        elif trans_spec >= 0:
            # Some instruments (i.e. LOQ) do not have monitor 4 on spectrum 4, as ZOOM
            # is currently the only one with monitor 5 at spectrum 5 we can make it an edge case
            # If a future instrument wants to use monitor 5 at a different spectrum number or
            # uses monitor 4 at spectrum 5 this should be updated
            return {TransId.SPEC_4_SHIFT: trans_spec_shift, TransId.SPEC: trans_spec}
        else:
            raise RuntimeError("Monitor {0} cannot be shifted".format(trans_spec))

    def _extract_radius(self, line):
        radius_string = re.sub(self._radius, "", line)
        radius = convert_string_to_float(radius_string)
        return {TransId.RADIUS: radius}

    def _extract_roi(self, line, original_line):
        file_names = TransParser.extract_file_names(line, original_line, self._roi)
        return {TransId.ROI: file_names}

    def _extract_mask(self, line, original_line):
        file_names = TransParser.extract_file_names(line, original_line, self._mask)
        return {TransId.MASK: file_names}

    def _extract_sample_workspace(self, line, original_line):
        sample_workspace = TransParser.extract_workspace(line, original_line, self._sample_workspace)
        return {TransId.SAMPLE_WORKSPACE: sample_workspace}

    def _extract_can_workspace(self, line, original_line):
        can_workspace = TransParser.extract_workspace(line, original_line, self._can_workspace)
        return {TransId.CAN_WORKSPACE: can_workspace}

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
    The TubeCalibFileParser handles the following structure for
        TUBECALIBFILE=calib_file.nxs
    """

    Type = "TUBECALIBFILE"

    def __init__(self):
        super(TubeCalibFileParser, self).__init__()

        self._tube_calib_file = "\\s*[\\w-]+(\\.NXS)\\s*"
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
        return {TubeCalibrationFileId.FILE: file_name}

    @staticmethod
    def get_type():
        return TubeCalibFileParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + TubeCalibFileParser.get_type() + "\\s*=\\s*"


class QResolutionParser(UserFileComponentParser):
    """
    The QResolutionParser handles the following structure for
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

    Type = "QRESOL"

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
        self._collimation_length_pattern = re.compile(start_string + self._collimation_length + float_number + end_string)

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
        self._moderator = '\\s*MODERATOR\\s*=\\s*(")?'
        self._file = '[\\w]+(\\.TXT)(")?'
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
        return {QResolutionId.ON: value}

    def _extract_delta_r(self, line):
        return {QResolutionId.DELTA_R: QResolutionParser.extract_float(line, self._delta_r)}

    def _extract_collimation_length(self, line):
        return {QResolutionId.COLLIMATION_LENGTH: QResolutionParser.extract_float(line, self._collimation_length)}

    def _extract_a1(self, line):
        return {QResolutionId.A1: QResolutionParser.extract_float(line, self._a1)}

    def _extract_a2(self, line):
        return {QResolutionId.A2: QResolutionParser.extract_float(line, self._a2)}

    def _extract_h1(self, line):
        return {QResolutionId.H1: QResolutionParser.extract_float(line, self._h1)}

    def _extract_w1(self, line):
        return {QResolutionId.W1: QResolutionParser.extract_float(line, self._w1)}

    def _extract_h2(self, line):
        return {QResolutionId.H2: QResolutionParser.extract_float(line, self._h2)}

    def _extract_w2(self, line):
        return {QResolutionId.W2: QResolutionParser.extract_float(line, self._w2)}

    def _extract_moderator(self, line, original_line):
        moderator_capital = re.sub(self._moderator, "", line)
        moderator_capital = re.sub('"', "", moderator_capital)
        moderator = re.search(moderator_capital, original_line, re.IGNORECASE).group(0)
        # Remove quotation marks
        return {QResolutionId.MODERATOR: moderator}

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
        FIT/TRANS/[CAN/|SAMPLE/][LIN|LOG|POLYNOMIAL[2|3|4|5]] [w1 w2]
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

        # General fits
        self._sample = "\\s*SAMPLE\\s*/\\s*"
        self._can = "\\s*CAN\\s*/\\s*"
        self._can_or_sample = "\\s*(" + self._can + "|" + self._sample + ")"
        self._optional_can_or_sample = "\\s*(" + self._can_or_sample + ")?"

        self._lin = "\\s*(LINEAR|LIN|STRAIGHT)\\s*"
        self._log = "\\s*(YLOG|LOG)\\s*"
        self._polynomial = "\\s*POLYNOMIAL\\s*"
        self._polynomial_with_optional_order = self._polynomial + "(2|3|4|5)?\\s*"
        self._lin_or_log_or_poly = "\\s*(" + self._lin + "|" + self._log + "|" + self._polynomial_with_optional_order + ")\\s*"
        self._lin_or_log_or_poly_to_remove = "\\s*(" + self._lin + "|" + self._log + "|" + self._polynomial + ")\\s*"
        self._wavelength_optional = "\\s*(" + float_number + space_string + float_number + ")?\\s*"

        self._general_fit_pattern = re.compile(
            start_string
            + self._trans_prefix
            + self._optional_can_or_sample
            + self._lin_or_log_or_poly
            + self._wavelength_optional
            + end_string
        )

        # Monitor times
        self._monitor = "\\s*MONITOR\\s*"
        self._monitor_pattern = re.compile(
            start_string + self._monitor + space_string + float_number + space_string + float_number + end_string
        )

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, FitParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_clear(setting):
            output = FitParser.extract_clear()
        elif self._is_monitor(setting):
            output = self._extract_monitor(setting)
        elif self._is_general_fit(setting):
            output = self._extract_general_fit(setting)
        else:
            raise RuntimeError("FitParser: Unknown command for FIT: {0}".format(line))
        return output

    def _is_clear(self, line):
        return does_pattern_match(self._trans_clear_pattern, line)

    def _is_monitor(self, line):
        return does_pattern_match(self._monitor_pattern, line)

    def _is_general_fit(self, line):
        return does_pattern_match(self._general_fit_pattern, line)

    def _extract_monitor(self, line):
        values_string = re.sub(self._monitor, "", line)
        values = extract_float_range(values_string)
        return {FitId.MONITOR_TIMES: range_entry(start=values[0], stop=values[1])}

    def _extract_general_fit(self, line):
        fit_type = self._get_fit_type(line)
        ws_type = self._get_workspace_type(line)
        wavelength_min, wavelength_max = self._get_wavelength(line)
        polynomial_order = self._get_polynomial_order(fit_type, line)
        return {
            FitId.GENERAL: fit_general(
                start=wavelength_min, stop=wavelength_max, fit_type=fit_type, data_type=ws_type, polynomial_order=polynomial_order
            )
        }

    def _get_wavelength(self, line):
        _, wavelength_min, wavelength_max = self._get_wavelength_and_polynomial(line)
        return wavelength_min, wavelength_max

    def _get_polynomial_order(self, fit_type, line):
        if fit_type != FitType.POLYNOMIAL:
            poly_order = 0
        else:
            poly_order, _, _ = self._get_wavelength_and_polynomial(line)
        return 2 if poly_order is None else poly_order

    def _get_wavelength_and_polynomial(self, line):
        fit_string = re.sub(self._trans_prefix, "", line)
        fit_string = re.sub(self._can_or_sample, "", fit_string)
        fit_string = re.sub(self._lin_or_log_or_poly_to_remove, "", fit_string)

        # We should now have something like [poly_order] [w1 w2]
        # There are four possibilities
        # 1. There is no number
        # 2. There is one number -> it has to be the poly_order
        # 3. There are two numbers -> it has to be the w1 and w2
        # 4. There are three numbers -> it has to be poly_order w1 and w2
        fit_string = " ".join(fit_string.split())
        fit_string_array = fit_string.split()
        length_array = len(fit_string_array)
        if length_array == 0:
            polynomial_order = None
            wavelength_min = None
            wavelength_max = None
        elif length_array == 1:
            polynomial_order = convert_string_to_integer(fit_string_array[0])
            wavelength_min = None
            wavelength_max = None
        elif length_array == 2:
            polynomial_order = None
            wavelength_min = convert_string_to_float(fit_string_array[0])
            wavelength_max = convert_string_to_float(fit_string_array[1])
        elif length_array == 3:
            polynomial_order = convert_string_to_integer(fit_string_array[0])
            wavelength_min = convert_string_to_float(fit_string_array[1])
            wavelength_max = convert_string_to_float(fit_string_array[2])
        else:
            raise RuntimeError("FitParser: Incorrect number of fit entries: {0}".format(line))

        return polynomial_order, wavelength_min, wavelength_max

    def _get_fit_type(self, line):
        if re.search(self._log, line) is not None:
            fit_type = FitType.LOGARITHMIC
        elif re.search(self._lin, line) is not None:
            fit_type = FitType.LINEAR
        elif re.search(self._polynomial, line) is not None:
            fit_type = FitType.POLYNOMIAL
        else:
            raise RuntimeError("FitParser: Encountered unknown fit function: {0}".format(line))
        return fit_type

    def _get_workspace_type(self, line):
        if re.search(self._sample, line) is not None:
            ws_type = DataType.SAMPLE
        elif re.search(self._can, line) is not None:
            ws_type = DataType.CAN
        else:
            ws_type = None
        return ws_type

    @staticmethod
    def extract_clear():
        """
        With this we want to clear the fit type settings.
        """
        return {FitId.GENERAL: fit_general(start=None, stop=None, fit_type=FitType.NO_FIT, data_type=None, polynomial_order=None)}

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
        GRAVITY/LEXTRA=l1 or (non-standard) GRAVITY/LEXTRA l1
    """

    Type = "GRAVITY"

    def __init__(self):
        super(GravityParser, self).__init__()

        # On Off
        self._on = "ON"
        self._on_off = "\\s*(OFF|" + self._on + ")"
        self._on_off_pattern = re.compile(start_string + self._on_off + end_string)

        # Extra length
        self._extra_length = "\\s*LEXTRA\\s*(=|\\s)?\\s*"
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
        return {GravityId.ON_OFF: value}

    def _extract_extra_length(self, line):
        extra_length_string = re.sub(self._extra_length, "", line)
        extra_length = convert_string_to_float(extra_length_string)
        return {GravityId.EXTRA_LENGTH: extra_length}

    @staticmethod
    def get_type():
        return GravityParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + GravityParser.get_type() + "(\\s*/\\s*|\\s+)"


class CompatibilityParser(UserFileComponentParser):
    """
    The CompatibilityParser handles the following structure for
        Compatibility ON
        Compatibility OFF
    """

    Type = "COMPATIBILITY"

    def __init__(self):
        super(CompatibilityParser, self).__init__()

        # On Off
        self._on = "ON"
        self._on_off = "\\s*(OFF|" + self._on + ")"
        self._on_off_pattern = re.compile(start_string + self._on_off + end_string)

    def parse_line(self, line):
        # Get the settings, ie remove command
        setting = UserFileComponentParser.get_settings(line, CompatibilityParser.get_type_pattern())

        # Determine the qualifier and extract the user setting
        if self._is_on_off(setting):
            output = self._extract_on_off(setting)
        else:
            raise RuntimeError("CompatibilityParserParser: Unknown command for COMPATIBILITY: {0}".format(line))
        return output

    def _is_on_off(self, line):
        return does_pattern_match(self._on_off_pattern, line)

    def _extract_on_off(self, line):
        value = re.sub(self._on, "", line).strip() == ""
        return {OtherId.USE_COMPATIBILITY_MODE: value}

    @staticmethod
    def get_type():
        return CompatibilityParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + CompatibilityParser.get_type() + "(\\s*/\\s*|\\s+)"


class MaskFileParser(UserFileComponentParser):
    """
    The MaskFileParser handles the following structure for
        MASKFILE=mask1.xml,mask2.xml,...
    """

    Type = "MASKFILE"

    def __init__(self):
        super(MaskFileParser, self).__init__()

        # MaskFile
        self._single_file = "[\\w-]+(\\.XML)"
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
        return {MaskId.FILE: elements}

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
        self._length_pattern = re.compile(
            start_string + self._length + float_number + space_string + integer_number + "(\\s*" + self._interpolate + "\\s*)?" + end_string
        )

        # Direct
        self._direct = "\\s*DIRECT\\s*"
        self._direct_pattern = re.compile(
            start_string + self._direct + self._optional_detector + self._equal + self._file_path + end_string
        )

        # Flat
        self._flat = "\\s*FLAT\\s*"
        self._flat_pattern = re.compile(start_string + self._flat + self._optional_detector + self._equal + self._file_path + end_string)

        # Flat
        self._hab_file = "\\s*HAB\\s*"
        self._hab_pattern = re.compile(start_string + self._hab_file + self._optional_detector + self._equal + self._file_path + end_string)

        # Spectrum
        self._spectrum = "\\s*SPECTRUM\\s*"
        self._trans = "\\s*TRANS\\s*"
        self._spectrum_pattern = re.compile(
            start_string
            + "(\\s*"
            + self._trans
            + "\\s*/\\s*)?"
            + self._spectrum
            + self._equal
            + integer_number
            + "(\\s*"
            + self._interpolate
            + "\\s*)?"
            + end_string
        )

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
        if re.search(self._interpolate, line) is not None:
            interpolate = True
            line = re.sub(self._interpolate, "", line)
        else:
            interpolate = False
        length_string = re.sub(self._length, "", line)
        length_entries = extract_float_list(length_string, separator=" ")
        if len(length_entries) != 2:
            raise RuntimeError("MonParser: Length setting needs 2 numeric parameters, " "but received {0}.".format(len(length_entries)))
        return {MonId.LENGTH: monitor_length(length=length_entries[0], spectrum=length_entries[1], interpolate=interpolate)}

    def _extract_direct(self, line, original_line):
        # If we have a HAB specified then select HAB
        # If we have LAB specified then select LAB
        # If nothing is specified then select BOTH
        is_hab = re.search(self._hab, line, re.IGNORECASE)
        is_lab = re.search(self._lab, line, re.IGNORECASE)

        if not is_hab and not is_lab:
            is_hab = True
            is_lab = True
        file_path = self._extract_file_path(line, original_line, self._direct)
        output = []
        if is_hab:
            output.append(monitor_file(file_path=file_path, detector_type=DetectorType.HAB))
        if is_lab:
            output.append(monitor_file(file_path=file_path, detector_type=DetectorType.LAB))
        return {MonId.DIRECT: output}

    def _extract_flat(self, line, original_line):
        # If we have a HAB specified then select HAB
        # If we have LAB specified then select LAB
        # If nothing is specified then select LAB
        detector_type = DetectorType.HAB if re.search(self._hab, line, re.IGNORECASE) else DetectorType.LAB
        file_path = self._extract_file_path(line, original_line, self._flat)
        return {MonId.FLAT: monitor_file(file_path=file_path, detector_type=detector_type)}

    def _extract_hab(self, line, original_line):
        # This is the same as direct/front
        file_path = self._extract_file_path(line, original_line, self._hab_file)
        return {MonId.DIRECT: [monitor_file(file_path=file_path, detector_type=DetectorType.HAB)]}

    def _extract_file_path(self, line, original_line, to_remove):
        direct = re.sub(self._detector, "", line)
        # Remove only the first occurrence
        direct = re.sub(to_remove, "", direct, count=1)
        direct = re.sub(self._equal, "", direct)
        direct = direct.strip()
        # We need to escape special characters
        direct = direct.replace("$", r"\$")
        direct = direct.replace(".", r"\.")
        direct = direct.replace("[", r"\[")
        direct = direct.replace("]", r"\]")
        direct = direct.replace(":", r"\:")

        # for VMS compatibility ignore anything in "[]", those are normally VMS drive specifications
        file_path = re.search(direct, original_line, re.IGNORECASE).group(0)
        if "[" in file_path:
            index = file_path.rfind("]")
            file_path = file_path[index + 1 :]
        return file_path

    def _extract_spectrum(self, line):
        if re.search(self._interpolate, line) is not None:
            interpolate = True
            line = re.sub(self._interpolate, "", line)
        else:
            interpolate = False

        if re.search(self._trans, line) is not None:
            is_trans = True
            line = re.sub(self._trans, "", line)
            line = re.sub("/", "", line)
        else:
            is_trans = False

        line = re.sub(self._spectrum, "", line)
        line = re.sub(self._equal, "", line)
        spectrum = convert_string_to_integer(line)
        return {MonId.SPECTRUM: monitor_spectrum(spectrum=spectrum, is_trans=is_trans, interpolate=interpolate)}

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

    def parse_line(self, line):
        # Get the settings, ie remove command

        setting = line.strip()

        if setting.upper().startswith(PrintParser.Type):
            setting = setting[len(PrintParser.Type) :]
            setting = setting.strip()
        else:
            raise RuntimeError("PrintParser: Failed to extract line {} it does not start with {}".format(line, PrintParser.Type))

        return {PrintId.PRINT_LINE: setting}

    @staticmethod
    def get_type():
        return PrintParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + PrintParser.get_type() + "(\\s+)"


class SANS2DParser(UserFileComponentParser):
    """
    The SANS2D is a hollow parser to ensure backwards compatibility
    """

    Type = "SANS2D"

    def __init__(self):
        super(SANS2DParser, self).__init__()

    def parse_line(self, line):
        return {}

    @staticmethod
    def get_type():
        return SANS2DParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + SANS2DParser.get_type() + "(\\s*)"


class LOQParser(UserFileComponentParser):
    """
    The LOQParser is a hollow parser to ensure backwards compatibility
    """

    Type = "LOQ"

    def __init__(self):
        super(LOQParser, self).__init__()

    def parse_line(self, line):
        return {}

    @staticmethod
    def get_type():
        return LOQParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + LOQParser.get_type() + "(\\s*)"


class LARMORParser(UserFileComponentParser):
    """
    The LARMORParser is a hollow parser to ensure backwards compatibility
    """

    Type = "LARMOR"

    def __init__(self):
        super(LARMORParser, self).__init__()

    def parse_line(self, line):
        return {}

    @staticmethod
    def get_type():
        return LARMORParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + LARMORParser.get_type() + "(\\s*)"


class ZOOMParser(UserFileComponentParser):
    """
    The ZOOMParser is a hollow parser to ensure backwards compatibility
    """

    Type = "ZOOM"

    def __init__(self):
        super(ZOOMParser, self).__init__()

    def parse_line(self, line):
        return {}

    @staticmethod
    def get_type():
        return ZOOMParser.Type

    @staticmethod
    @abc.abstractmethod
    def get_type_pattern():
        return "\\s*" + ZOOMParser.get_type() + "(\\s*)"


class IgnoredParser(object):
    """
    The IgnoredParser deals with known commands which are not relevant any longer, but might appear in legacy files.
    This is of particular importance for Collete commands.
    """

    def __init__(self):
        # SPY ON
        self._on_off = "\\s*(ON|OFF)\\s*"
        self._spy_on_off = "\\s*SPY\\s*" + self._on_off
        self._spy_on_off_pattern = re.compile(self._spy_on_off)

        # READ
        self._read_pattern = re.compile("\\s*READ\\s*")

        # Centre
        self._centre = "\\s*FIT\\s*/\\s*CENTRE\\s*" + float_number + space_string + float_number
        self._centre_pattern = re.compile(self._centre)

        # MID
        self._mid = "\\s*FIT\\s*/\\s*MID\\s*/\\s*FILE\\s*=\\s*"
        self._mid_pattern = re.compile(self._mid)
        self._mid_hab = "\\s*FIT\\s*/\\s*MID\\s*/\\s*HAB\\s*/\\s*FILE\\s*=\\s*"
        self._mid_hab_pattern = re.compile(self._mid_hab)

        # SP
        self._sp = "\\s*L\\s*/\\s*SP\\s*"
        self._sp_pattern = re.compile(self._sp)

        # Set notab
        self._notab = "\\s*SET\\s*/\\s*NOTAB"
        self._notab_pattern = re.compile(self._notab)

        # Set yc
        self._yc = "\\s*SET\\s*/\\s*YC\\s*"
        self._yc_pattern = re.compile(self._yc)

        # Box mask
        self._mask_pattern = re.compile(
            start_string
            + "\\s*MASK\\s*"
            + integer_number
            + space_string
            + integer_number
            + space_string
            + integer_number
            + space_string
            + integer_number
            + end_string
        )

        # Habeff
        self._habeff_pattern = re.compile("\\s*MON\\s*/\\s*HABEFF\\s*")

        # Habpath
        self._habpath_pattern = re.compile("\\s*MON\\s*/\\s*HABPATH\\s*")

        # Bad monitor description
        self._back_mon_pattern = re.compile("\\s*BACK\\s*/\\s*M\\s*" + integer_number + r"\." + integer_number + "\\s*/\\s*TIMES\\s*")

    def is_ignored(self, line):
        ignore = False

        line = line.upper()
        if (
            does_pattern_match(self._spy_on_off_pattern, line)
            or does_pattern_match(self._read_pattern, line)
            or does_pattern_match(self._centre_pattern, line)
            or does_pattern_match(self._mid_pattern, line)
            or does_pattern_match(self._mid_hab_pattern, line)
            or does_pattern_match(self._sp_pattern, line)
            or does_pattern_match(self._notab_pattern, line)
            or does_pattern_match(self._yc_pattern, line)
            or does_pattern_match(self._mask_pattern, line)
            or does_pattern_match(self._habeff_pattern, line)
            or does_pattern_match(self._habpath_pattern, line)
            or does_pattern_match(self._back_mon_pattern, line)
        ):
            ignore = True
        return ignore


class UserFileParser(object):
    def __init__(self):
        super(UserFileParser, self).__init__()
        self._parsers = {
            BackParser.get_type(): BackParser(),
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
            LOQParser.get_type(): LOQParser(),
            LARMORParser.get_type(): LARMORParser(),
            ZOOMParser.get_type(): ZOOMParser(),
            CompatibilityParser.get_type(): CompatibilityParser(),
        }
        self._ignored_parser = IgnoredParser()

    def _get_correct_parser(self, line):
        line = line.strip()
        line = line.upper()

        if InstrParser.get_type_pattern(line):
            return InstrParser()

        for key in self._parsers:
            parser = self._parsers[key]
            if re.match(parser.get_type_pattern(), line, re.IGNORECASE) is not None:
                return parser
            else:
                continue

        # We have encountered an unknown file specifier.
        raise ValueError("UserFileParser: Unknown user " "file command: {0}".format(line))

    def parse_line(self, line):
        # Clean the line of trailing white space
        line = line.strip()

        # If the line is empty, then ignore it
        if not line:
            return {}

        # If the entry is a comment, then ignore it
        if line.startswith("!"):
            return {}

        # Check if we are dealing with an ignored line (from collete for example)
        if self._ignored_parser.is_ignored(line):
            return {}

        # Get the appropriate parser
        parser = self._get_correct_parser(line)

        # Parse the line and return the result
        return parser.parse_line(line)
