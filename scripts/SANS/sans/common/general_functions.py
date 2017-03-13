""" The elements of this module contain various general-purpose functions for the SANS reduction framework."""

# pylint: disable=invalid-name

from __future__ import (absolute_import, division, print_function)
from math import (acos, sqrt, degrees)
import re
from mantid.api import AlgorithmManager, AnalysisDataService
from mantid.kernel import (DateAndTime)
from sans.common.constants import SANS_FILE_TAG
from sans.common.log_tagger import (get_tag, has_tag, set_tag)
from sans.common.enums import (DetectorType, RangeStepType)


# -------------------------------------------
# Free functions
# -------------------------------------------
def get_log_value(run, log_name, log_type):
    """
    Find a log value.

    There are two options here. Either the log is a scalar or a vector. In the case of a scalar there is not much
    left to do. In the case of a vector we select the first element whose time_stamp is after the start time of the run
    @param run: a Run object.
    @param log_name: the name of the log entry
    @param log_type: the expected type fo the log entry
    @return: the log entry
    """
    try:
        # Scalar case
        output = log_type(run.getLogData(log_name).value)
    except TypeError:
        # We must be dealing with a vectorized case, ie a time series
        log_property = run.getLogData(log_name)
        number_of_entries = len(log_property.value)

        # If we have only one item, then there is nothing left to do
        output = None
        if number_of_entries == 1:
            output = log_type(run.getLogData(log_name).value[0])
        else:
            # Get the first entry which is past the start time log
            start_time = DateAndTime(run.getLogData('run_start').value)
            times = log_property.times
            values = log_property.value

            has_found_value = False
            for index in range(0, number_of_entries):
                if times[index] > start_time:
                    # Not fully clear why we take index - 1, but this follows the old implementation rules
                    value_index = index if index == 0 else index - 1
                    has_found_value = True
                    output = log_type(values[value_index])
                    break

            # Not fully clear why we take index - 1, but this follows the old implementation rules
            if not has_found_value:
                output = float(values[number_of_entries - 1])
    return output


def get_single_valued_logs_from_workspace(workspace, log_names, log_types, convert_from_millimeter_to_meter=False):
    """
    Gets non-array valued entries from the sample logs.

    :param workspace: the workspace with the sample log.
    :param log_names: the log names which are to be extracted.
    :param log_types: the types of log entries, ie strings or numeric
    :param convert_from_millimeter_to_meter:
    :return: the log results
    """
    assert len(log_names) == len(log_types)
    # Find the desired log names.
    run = workspace.getRun()
    log_results = {}
    for log_name, log_type in zip(log_names, log_types):
        log_value = get_log_value(run, log_name, log_type)
        log_results.update({log_name: log_value})
    if convert_from_millimeter_to_meter:
        for key in list(log_results.keys()):
            log_results[key] /= 1000.
    return log_results


def create_unmanaged_algorithm(name, **kwargs):
    """
    Creates an unmanaged child algorithm and initializes it.

    :param name: the name of the algorithm
    :param kwargs: settings for the algorithm
    :return: an initialized algorithm instance.
    """
    alg = AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    alg.setChild(True)
    for key, value in list(kwargs.items()):
        alg.setProperty(key, value)
    return alg


def quaternion_to_angle_and_axis(quaternion):
    """
    Converts a quaternion to an angle + an axis

    The conversion from a quaternion to an angle + axis is explained here:
    http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToAngle/
    """
    angle = 2*acos(quaternion[0])
    s_parameter = sqrt(1 - quaternion[0]*quaternion[0])

    axis = []
    # If the the angle is zero, then it does not make sense to have an axis
    if s_parameter < 1e-8:
        axis.append(quaternion[1])
        axis.append(quaternion[2])
        axis.append(quaternion[3])
    else:
        axis.append(quaternion[1]/s_parameter)
        axis.append(quaternion[2]/s_parameter)
        axis.append(quaternion[3]/s_parameter)
    return degrees(angle), axis


def get_charge_and_time(workspace):
    """
    Gets the total charge and time from a workspace

    :param workspace: the workspace from which we extract the charge and time.
    :return: the charge, the time
    """
    run = workspace.getRun()
    charges = run.getLogData('proton_charge')
    total_charge = sum(charges.value)
    time_passed = (charges.times[-1] - charges.times[0]).total_microseconds()
    time_passed /= 1e6
    return total_charge, time_passed


def add_to_sample_log(workspace, log_name, log_value, log_type):
    """
    Adds a sample log to the workspace

    :param workspace: the workspace to whcih the sample log is added
    :param log_name: the name of the log
    :param log_value: the value of the log in string format
    :param log_type: the log value type which can be String, Number, Number Series
    """
    if log_type not in ["String", "Number", "Number Series"]:
        raise ValueError("Tryint go add {0} to the sample logs but it was passed "
                         "as an unknown type of {1}".format(log_value, log_type))
    if not isinstance(log_value, str):
        raise TypeError("The value which is added to the sample logs needs to be passed as a string,"
                        " but it is passed as {0}".format(type(log_value)))

    add_log_name = "AddSampleLog"
    add_log_options = {"Workspace": workspace,
                       "LogName": log_name,
                       "LogText": log_value,
                       "LogType": log_type}
    add_log_alg = create_unmanaged_algorithm(add_log_name, **add_log_options)
    add_log_alg.execute()


def append_to_sans_file_tag(workspace, to_append):
    """
    Appends a string to the existing sans file tag.

    :param workspace: the workspace which contains the sample logs with the sans file tag.
    :param to_append: the additional tag
    """
    if has_tag(SANS_FILE_TAG, workspace):
        value = get_tag(SANS_FILE_TAG, workspace)
        value += to_append
        set_tag(SANS_FILE_TAG, value, workspace)


def get_ads_workspace_references():
    """
    Gets a list of handles of available workspaces on the ADS

    @return: the workspaces on the ADS.
    """
    for workspace_name in AnalysisDataService.getObjectNames():
        yield AnalysisDataService.retrieve(workspace_name)


def convert_bank_name_to_detector_type_isis(detector_name):
    """
    Converts a detector name of an isis detector to a detector type.

    The current translation is
    SANS2D: rear-detector -> LAB
            front-detector -> HAB
            but also allowed rear, front
    LOQ:    main-detector-bank -> LAB
            HAB                -> HAB
            but also allowed main
    LARMOR: DetectorBench      -> LAB

    @param detector_name: a string with a valid detector name
    @return: a detector type depending on the input string, or a runtime exception.
    """
    detector_name = detector_name.upper()
    detector_name = detector_name.strip()
    if detector_name == "REAR-DETECTOR" or detector_name == "MAIN-DETECTOR-BANK" or detector_name == "DETECTORBENCH" \
            or detector_name == "REAR" or detector_name == "MAIN":
        detector_type = DetectorType.LAB
    elif detector_name == "FRONT-DETECTOR" or detector_name == "HAB" or detector_name == "FRONT":
        detector_type = DetectorType.HAB
    else:
        raise RuntimeError("There is not detector type conversion for a detector with the "
                           "name {0}".format(detector_name))
    return detector_type


def parse_event_slice_setting(string_to_parse):
    """
    Create a list of boundaries from a string defining the slices.
    Valid syntax is:
      * From 8 to 9 > '8-9' --> return [[8,9]]
      * From 8 to 9 and from 10 to 12 > '8-9, 10-12' --> return [[8,9],[10,12]]
      * From 5 to 10 in steps of 1 > '5:1:10' --> return [[5,6],[6,7],[7,8],[8,9],[9,10]]
      * From 5 > '>5' --> return [[5, -1]]
      * Till 5 > '<5' --> return [[-1,5]]

    Any combination of these syntax separated by comma is valid.
    A special mark is used to signal no limit: -1,
    As, so, for an empty string, it will return: None.

    It does not accept negative values.
    """

    def _does_match(compiled_regex, line):
        return compiled_regex.match(line) is not None

    def _extract_simple_slice(line):
        start, stop = line.split("-")
        start = float(start)
        stop = float(stop)
        if start > stop:
            raise ValueError("Parsing event slices. It appears that the start value {0} is larger than the stop "
                             "value {1}. Make sure that this is not the case.")
        return [start, stop]

    def float_range(start, stop, step):
        while start < stop:
            yield start
            start += step

    def _extract_slice_range(line):
        split_line = line.split(":")
        start = float(split_line[0])
        step = float(split_line[1])
        stop = float(split_line[2])
        if start > stop:
            raise ValueError("Parsing event slices. It appears that the start value {0} is larger than the stop "
                             "value {1}. Make sure that this is not the case.")

        elements = list(float_range(start, stop, step))
        # We are missing the last element
        elements.append(stop)

        # We generate ranges with [[element[0], element[1]], [element[1], element[2]], ...]
        ranges = list(zip(elements[:-1], elements[1:]))
        return [[e1, e2] for e1, e2 in ranges]

    def _extract_full_range(line, range_marker_pattern):
        is_lower_bound = ">" in line
        line = re.sub(range_marker_pattern, "", line)
        value = float(line)
        if is_lower_bound:
            return [value, -1]
        else:
            return [-1, value]

    # Check if the input actually exists.
    if not string_to_parse:
        return None

    number = r'(\d+(?:\.\d+)?(?:[eE][+-]\d+)?)'  # float without sign
    simple_slice_pattern = re.compile("\\s*" + number + "\\s*" r'-' + "\\s*" + number + "\\s*")
    slice_range_pattern = re.compile("\\s*" + number + "\\s*" + r':' + "\\s*" + number + "\\s*"
                                     + r':' + "\\s*" + number)
    full_range_pattern = re.compile("\\s*" + "(<|>)" + "\\s*" + number + "\\s*")

    range_marker = re.compile("[><]")

    slice_settings = string_to_parse.split(',')
    all_ranges = []
    for slice_setting in slice_settings:
        slice_setting = slice_setting.replace(' ', '')
        # We can have three scenarios
        # 1. Simple Slice:     X-Y
        # 2. Slice range :     X:Y:Z
        # 3. Slice full range: >X or <X
        if _does_match(simple_slice_pattern, slice_setting):
            all_ranges.append(_extract_simple_slice(slice_setting))
        elif _does_match(slice_range_pattern, slice_setting):
            all_ranges.extend(_extract_slice_range(slice_setting))
        elif _does_match(full_range_pattern, slice_setting):
            all_ranges.append(_extract_full_range(slice_setting, range_marker))
        else:
            raise ValueError("The provided event slice configuration {0} cannot be parsed because "
                             "of {1}".format(slice_settings, slice_setting))
    return all_ranges


def get_ranges_from_event_slice_setting(string_to_parse):
    parsed_elements = parse_event_slice_setting(string_to_parse)
    if not parsed_elements:
        return
    # We have the elements in the form [[a, b], [c, d], ...] but want [a, c, ...] and [b, d, ...]
    lower = [element[0] for element in parsed_elements]
    upper = [element[1] for element in parsed_elements]
    return lower, upper


def get_bins_for_rebin_setting(min_value, max_value, step_value, step_type):
    """
    Creates a list of bins for the rebin setting.

    @param min_value: the minimum value
    @param max_value: the maximum value
    @param step_value: the step value
    @param step_type: the step type, ie if linear or logarithmic
    @return: a list of bin values
    """
    lower_bound = min_value
    bins = []
    while lower_bound < max_value:

        bins.append(lower_bound)
        # We can either have linear or logarithmic steps. The logarithmic step depends on the lower bound.
        if step_type is RangeStepType.Lin:
            step = step_value
        else:
            step = lower_bound*step_value

        # Check if the step will bring us out of bounds. If so, then set the new upper value to the max_value
        upper_bound = lower_bound + step
        upper_bound = upper_bound if upper_bound < max_value else max_value

        # Now we advance the lower bound
        lower_bound = upper_bound
    # Add the last lower_bound
    bins.append(lower_bound)
    return bins


def get_range_lists_from_bin_list(bin_list):
    return bin_list[:-1], bin_list[1:]


def get_ranges_for_rebin_setting(min_value, max_value, step_value, step_type):
    """
    Creates two lists of lower and upper bounds for the

    @param min_value: the minimum value
    @param max_value: the maximum value
    @param step_value: the step value
    @param step_type: the step type, ie if linear or logarithmic
    @return: two ranges lists, one for the lower and one for the upper bounds.
    """
    bins = get_bins_for_rebin_setting(min_value, max_value, step_value, step_type)
    return get_range_lists_from_bin_list(bins)


def get_ranges_for_rebin_array(rebin_array):
    """
    Converts a rebin string into min, step (+ step_type), max

    @param rebin_array: a simple rebin array, ie min, step, max
    @return: two ranges lists, one for the lower and one for the upper bounds.
    """
    min_value = rebin_array[0]
    step_value = rebin_array[1]
    max_value = rebin_array[2]
    step_type = RangeStepType.Lin if step_value >= 0. else RangeStepType.Log
    step_value = abs(step_value)
    return get_ranges_for_rebin_setting(min_value, max_value, step_value, step_type)
