# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The elements of this module contain various general-purpose functions for the SANS reduction framework."""

# pylint: disable=invalid-name

from __future__ import (absolute_import, division, print_function)
from math import (acos, sqrt, degrees)
import re
from copy import deepcopy
import json
import numpy as np
from mantid.api import (AlgorithmManager, AnalysisDataService, isSameWorkspaceObject)
from sans.common.constant_containers import (SANSInstrument_enum_list, SANSInstrument_string_list,
                                             SANSInstrument_string_as_key_NoInstrument)
from sans.common.constants import (SANS_FILE_TAG, ALL_PERIODS, SANS2D, EMPTY_NAME,
                                   REDUCED_CAN_TAG)
from sans.common.log_tagger import (get_tag, has_tag, set_tag, has_hash, get_hash_value, set_hash)
from sans.common.enums import (DetectorType, RangeStepType, ReductionDimensionality, OutputParts, ISISReductionMode,
                               SANSInstrument, SANSFacility, DataType, TransmissionType)
# -------------------------------------------
# Constants
# -------------------------------------------
ALTERNATIVE_SANS2D_NAME = "SAN"


# -------------------------------------------
# Free functions
# -------------------------------------------
def get_log_value(run, log_name, log_type):
    """
    Find a log value.

    There are two options here. Either the log is a scalar or a vector. In the case of a scalar there is not much
    left to do. In the case of a vector we select the first element whose time_stamp is after the start time of the run
    :param run: a Run object.
    :param log_name: the name of the log entry
    :param log_type: the expected type fo the log entry
    :return: the log entry
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
            start_time = run.startTime().to_datetime64()
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
        for key in log_results:
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
    alg.setRethrows(True)
    for key, value in kwargs.items():
        alg.setProperty(key, value)
    return alg


def create_managed_non_child_algorithm(name, **kwargs):
    """
    Creates a managed child algorithm and initializes it.

    :param name: the name of the algorithm
    :param kwargs: settings for the algorithm
    :return: an initialized algorithm instance.
    """
    alg = AlgorithmManager.create(name)
    alg.initialize()
    alg.setChild(False)
    alg.setRethrows(True)
    for key, value in kwargs.items():
        alg.setProperty(key, value)
    return alg


def create_child_algorithm(parent_alg, name, **kwargs):
    """
    Creates a child algorithm from a parent algorithm

    :param parent_alg: a handle to the parent algorithm
    :param name: the name of the child algorithm
    :param kwargs: a argument dict
    :return: the child algorithm
    """
    if parent_alg:
        alg = parent_alg.createChildAlgorithm(name)
        alg.setRethrows(True)
        for key, value in kwargs.items():
            alg.setProperty(key, value)
    else:
        alg = create_unmanaged_algorithm(name, **kwargs)
    return alg


def get_input_workspace_as_copy_if_not_same_as_output_workspace(alg):
    """
    This function checks if the input workspace is the same as the output workspace, if so then it returns the
    workspace else it creates a copy of the input in order for it to be consumed.

    :param alg: a handle to the algorithm which has a InputWorkspace property and a OutputWorkspace property
    :return: a workspace
    """
    def _clone_input(_ws):
        clone_name = "CloneWorkspace"
        clone_options = {"InputWorkspace": _ws,
                         "OutputWorkspace": EMPTY_NAME}
        clone_alg = create_unmanaged_algorithm(clone_name, **clone_options)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    if "InputWorkspace" not in alg or "OutputWorkspace" not in alg:
        raise RuntimeError("The algorithm {} does not seem to have an InputWorkspace and"
                           " an OutputWorkspace property.".format(alg.name()))

    ws_in = alg.getProperty("InputWorkspace").value
    if ws_in is None:
        raise RuntimeError("The input for the algorithm {} seems to be None. We need an input.".format(alg.name()))

    ws_out = alg.getProperty("OutputWorkspace").value

    # There are three scenarios.
    # 1. ws_out is None
    # 2. ws_in and ws_out are the same workspace
    # 3. ws_in and ws_out are not the same workspace
    if ws_out is None:
        return _clone_input(ws_in)
    elif isSameWorkspaceObject(ws_in, ws_out):
        return ws_in
    else:
        return _clone_input(ws_in)


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
    time_passed = int((charges.times[-1] - charges.times[0]) / np.timedelta64(1,'us')) # microseconds
    time_passed = float(time_passed) / 1e6
    return total_charge, time_passed


def add_to_sample_log(workspace, log_name, log_value, log_type):
    """
    Adds a sample log to the workspace

    :param workspace: the workspace to which the sample log is added
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

    :return: the workspaces on the ADS.
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
    ZOOM:   rear-detector -> LAB

    :param detector_name: a string with a valid detector name
    :return: a detector type depending on the input string, or a runtime exception.
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


def convert_instrument_and_detector_type_to_bank_name(instrument, detector_type):
    if instrument is SANSInstrument.SANS2D:
        bank_name = "front-detector" if detector_type is DetectorType.HAB else "rear-detector"
    elif instrument is SANSInstrument.LOQ:
        bank_name = "HAB" if detector_type is DetectorType.HAB else "main-detector-bank"
    elif instrument is SANSInstrument.LARMOR:
        bank_name = "DetectorBench"
    elif instrument is SANSInstrument.ZOOM:
        bank_name = "rear-detector"
    else:
        raise RuntimeError("SANSCrop: The instrument {0} is currently not supported.".format(instrument))
    return bank_name


def is_part_of_reduced_output_workspace_group(state):
    """
    Check if this state will generate a WorkspaceGroup as the output.

    This can be the case if:
    1. The scatter sample data is a multi-period workspace
    2. A event slice reduction is being performed, ie that there is more than one entry for the slice length

    Note: that this is a hacky solution to for the return value of WavRangeReduction in ISISCommandInterface.
          Improve this!!! (Maybe by getting rid of the return value of WavRangeReduction)
    :param state: a state object.
    :return: True if the reduced output is a workspace group else false
    """
    # 1. Multi-period input
    data_info = state.data
    is_multi_period = data_info.sample_scatter_is_multi_period

    # 2. Slice event input
    slice_info = state.slice
    if slice_info.start_time is None:
        is_sliced_reduction = False
    else:
        is_sliced_reduction = len(slice_info.start_time) > 1

    return is_multi_period or is_sliced_reduction


def parse_diagnostic_settings(string_to_parse):
    """
  This class parses a given string into  vector of vectors of numbers.
  For example : 60,61+62,63-66,67:70,71-75:2  This gives a vector containing 8
  vectors
  as Vec[8] and Vec[0] is a vector containing 1 element 60
  Vec[1] is a vector containing elements 61,62 Vec[2] is a vector containing
  elements 63,64,65,66,
  Vec[3] is a vector containing element 67,Vec[4] is a vector containing element
  68,
  Vec[5] is a vector containing element 69,Vec[6] is a vector containing element
  70 ,
  vec[7] is a vector containing element 71,73,75
  """

    def _does_match(compiled_regex, line):
        return compiled_regex.match(line) is not None

    def _extract_simple_range(line):
        start, stop = line.split("-")
        start = int(start)
        stop = int(stop)
        if start > stop:
            raise ValueError("Parsing event slices. It appears that the start value {0} is larger than the stop "
                             "value {1}. Make sure that this is not the case.").format(start, stop)
        return [start, stop]

    def _extract_simple_range_with_step_pattern(line):
        start, stop = line.split("-")
        start = int(start)
        stop, step = stop.split(":")
        stop = int(stop)
        step = int(step)
        if start > stop:
            raise ValueError("Parsing event slices. It appears that the start value {0} is larger than the stop "
                             "value {1}. Make sure that this is not the case.").format(start, stop)
        return range(start, stop+1, step)

    def _extract_multiple_entry(line):
        split_line = line.split(":")
        start = int(split_line[0])
        stop = int(split_line[1])
        if start > stop:
            raise ValueError("Parsing event slices. It appears that the start value {0} is larger than the stop "
                             "value {1}. Make sure that this is not the case.").format(start, stop)
        result = []
        for entry in range(start, stop + 1):
            result.append([entry, entry])
        return result

    def _extract_number(line):
        return [int(line), int(line)]

    # Check if the input actually exists.
    if not string_to_parse:
        return None

    number = r'(\d+(?:\.\d+)?(?:[eE][+-]\d+)?)'  # float without sign
    single_number_pattern = re.compile("\\s*" + number + "\\s*")
    simple_range_pattern = re.compile("\\s*" + number + "\\s*" r'-' + "\\s*" + number + "\\s*")

    multiple_entry_pattern = re.compile("\\s*" + number + "\\s*" + r':' + "\\s*" + number + "\\s*")

    simple_range_with_step_pattern = re.compile("\\s*" + number + "\\s*" r'-' + "\\s*" + number + "\\s*" + r':' + "\\s*")

    slice_settings = string_to_parse.split(',')

    all_ranges = []
    for slice_setting in slice_settings:
        slice_setting = slice_setting.replace(' ', '')
        if _does_match(multiple_entry_pattern, slice_setting):
            all_ranges += _extract_multiple_entry(slice_setting)
        else:
            integral = []
            # We can have three scenarios
            # 1. Simple Slice:     X-Y
            # 2. Slice range :     X:Y:Z
            # 3. Slice full range: >X or <X
            if _does_match(simple_range_with_step_pattern, slice_setting):
                integral += _extract_simple_range_with_step_pattern(slice_setting)
            elif _does_match(simple_range_pattern, slice_setting):
                integral += _extract_simple_range(slice_setting)
            elif _does_match(single_number_pattern, slice_setting):
                integral += _extract_number(slice_setting)
            else:
                raise ValueError("The provided event slice configuration {0} cannot be parsed because "
                                 "of {1}".format(slice_settings, slice_setting))
            all_ranges.append(integral)
    return all_ranges


# ----------------------------------------------------------------------------------------------------------------------
#  Functions for bins, ranges and slices
# ----------------------------------------------------------------------------------------------------------------------
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
            return [value, -1.]
        else:
            return [-1., value]

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

    :param min_value: the minimum value
    :param max_value: the maximum value
    :param step_value: the step value
    :param step_type: the step type, ie if linear or logarithmic
    :return: a list of bin values
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

    :param min_value: the minimum value
    :param max_value: the maximum value
    :param step_value: the step value
    :param step_type: the step type, ie if linear or logarithmic
    :return: two ranges lists, one for the lower and one for the upper bounds.
    """
    bins = get_bins_for_rebin_setting(min_value, max_value, step_value, step_type)
    return get_range_lists_from_bin_list(bins)


def get_ranges_for_rebin_array(rebin_array):
    """
    Converts a rebin string into min, step (+ step_type), max

    :param rebin_array: a simple rebin array, ie min, step, max
    :return: two ranges lists, one for the lower and one for the upper bounds.
    """
    min_value = rebin_array[0]
    step_value = rebin_array[1]
    max_value = rebin_array[2]
    step_type = RangeStepType.Lin if step_value >= 0. else RangeStepType.Log
    step_value = abs(step_value)
    return get_ranges_for_rebin_setting(min_value, max_value, step_value, step_type)


# ----------------------------------------------------------------------------------------------------------------------
# Functions related to workspace names
# ----------------------------------------------------------------------------------------------------------------------
def get_standard_output_workspace_name(state, reduction_data_type, data_type = DataType.to_string(DataType.Sample)):
    """
    Creates the name of the output workspace from a state object.

    The name of the output workspace is:
    1. The short run number
    2. If specific period is being reduced: 'p' + number
    3. Short detector name of the current reduction or "merged"
    4. The reduction dimensionality: "_" + dimensionality
    5. A wavelength range: wavelength_low + "_" + wavelength_high
    6. In case of a 1D reduction, then add phi limits
    7. If we are dealing with an actual slice limit, then specify it: "_tXX_TYY" Note that the time set to
       two decimals
    :param state: a SANSState object
    :param reduction_data_type: which reduced data type is being looked at, ie HAB, LAB or Merged
    :return: the name of the reduced workspace, and the base name fo the reduced workspace
    """
    # 1. Short run number
    data = state.data
    short_run_number = data.sample_scatter_run_number
    short_run_number_as_string = str(short_run_number)

    # 2. Multiperiod
    if state.data.sample_scatter_period != ALL_PERIODS:
        period = data.sample_scatter_period
        period_as_string = "p"+str(period)
    else:
        period_as_string = ""

    # 3. Detector name
    move = state.move
    detectors = move.detectors
    if reduction_data_type is ISISReductionMode.Merged:
        detector_name_short = "merged"
    elif reduction_data_type is ISISReductionMode.HAB:
        det_name = detectors[DetectorType.to_string(DetectorType.HAB)].detector_name_short
        detector_name_short = det_name if det_name is not None else "hab"
    elif reduction_data_type is ISISReductionMode.LAB:
        det_name = detectors[DetectorType.to_string(DetectorType.LAB)].detector_name_short
        detector_name_short = det_name if det_name is not None else "lab"
    else:
        raise RuntimeError("SANSStateFunctions: Unknown reduction data type {0} cannot be used to "
                           "create an output name".format(reduction_data_type))

    # 4. Dimensionality
    reduction = state.reduction
    if reduction.reduction_dimensionality is ReductionDimensionality.OneDim:
        dimensionality_as_string = "_1D"
    else:
        dimensionality_as_string = "_2D"

    # 5. Wavelength range
    wavelength = state.wavelength
    wavelength_range_string = "_" + str(wavelength.wavelength_low[0]) + "_" + str(wavelength.wavelength_high[0])

    # 6. Phi Limits
    mask = state.mask
    if reduction.reduction_dimensionality is ReductionDimensionality.OneDim:
        if mask.phi_min and mask.phi_max and (abs(mask.phi_max - mask.phi_min) != 180.0):
            phi_limits_as_string = 'Phi' + str(mask.phi_min) + '_' + str(mask.phi_max)
        else:
            phi_limits_as_string = ""
    else:
        phi_limits_as_string = ""

    # 7. Slice limits
    slice_state = state.slice
    start_time = slice_state.start_time
    end_time = slice_state.end_time
    if start_time and end_time:
        start_time_as_string = '_t%.2f' % start_time[0]
        end_time_as_string = '_T%.2f' % end_time[0]
    else:
        start_time_as_string = ""
        end_time_as_string = ""

    # Piece it all together
    output_workspace_name = (short_run_number_as_string + period_as_string + detector_name_short +
                             dimensionality_as_string + wavelength_range_string + phi_limits_as_string +
                             start_time_as_string + end_time_as_string)
    output_workspace_base_name = (short_run_number_as_string + detector_name_short + dimensionality_as_string
                                  + phi_limits_as_string)

    return output_workspace_name, output_workspace_base_name


def get_transmission_output_name(state, data_type=DataType.Sample, multi_reduction_type=None, fitted=True):
    user_specified_output_name = state.save.user_specified_output_name

    data = state.data
    short_run_number = data.sample_scatter_run_number
    short_run_number_as_string = str(short_run_number)

    calculated_transmission_state = state.adjustment.calculate_transmission
    fit = calculated_transmission_state.fit[DataType.to_string(DataType.Sample)]
    wavelength_range_string = "_" + str(fit.wavelength_low) + "_" + str(fit.wavelength_high)

    trans_suffix = "_trans_Sample" if data_type == DataType.Sample else "_trans_Can"
    trans_suffix = trans_suffix + '_unfitted' if not fitted else trans_suffix

    if user_specified_output_name:
        output_name = user_specified_output_name + trans_suffix
        output_base_name = user_specified_output_name + '_trans'
    else:
        output_name = short_run_number_as_string + trans_suffix + wavelength_range_string
        output_base_name = short_run_number_as_string + '_trans' + wavelength_range_string

    if multi_reduction_type and fitted:
        if multi_reduction_type["wavelength_range"]:
            wavelength = state.wavelength
            wavelength_range_string = "_" + str(wavelength.wavelength_low[0]) + "_" + str(
                wavelength.wavelength_high[0])
            output_name += wavelength_range_string

    return output_name, output_base_name


def get_output_name(state, reduction_mode, is_group, suffix="", multi_reduction_type=None):
    # Get the external settings from the save state
    save_info = state.save
    user_specified_output_name = save_info.user_specified_output_name
    user_specified_output_name_group = user_specified_output_name
    user_specified_output_name_suffix = save_info.user_specified_output_name_suffix
    use_reduction_mode_as_suffix = save_info.use_reduction_mode_as_suffix
    # This adds a reduction mode suffix in merged or all reductions so the workspaces do not overwrite each other.
    if (
            state.reduction.reduction_mode == ISISReductionMode.Merged or state.reduction.reduction_mode == ISISReductionMode.All) \
            and user_specified_output_name:
        use_reduction_mode_as_suffix = True

    # Get the standard workspace name
    workspace_name, workspace_base_name = get_standard_output_workspace_name(state, reduction_mode)

    # If user specified output name is not none then we use it for the base name
    if user_specified_output_name and not is_group:
        # Deal with single period data which has a user-specified name
        output_name = user_specified_output_name
        output_base_name = user_specified_output_name
    elif user_specified_output_name and is_group:
        # Deal with data which requires special attention and which has a user-specified name
        output_name = user_specified_output_name
        output_base_name = user_specified_output_name_group
    elif not user_specified_output_name and is_group:
        output_name = workspace_name
        output_base_name = workspace_base_name
    else:
        output_name = workspace_name
        output_base_name = workspace_name

    # Add a reduction mode suffix if it is required
    if use_reduction_mode_as_suffix:
        if reduction_mode is ISISReductionMode.LAB:
            output_name += "_rear"
            output_base_name += "_rear"
        elif reduction_mode is ISISReductionMode.HAB:
            output_name += "_front"
            output_base_name += "_front"
        elif reduction_mode is ISISReductionMode.Merged:
            output_name += "_merged"
            output_base_name += "_merged"

    if multi_reduction_type and user_specified_output_name:
        if multi_reduction_type["period"]:
            period = state.data.sample_scatter_period
            period_as_string = "_p" + str(period)
            output_name += period_as_string

        if multi_reduction_type["event_slice"]:
            slice_state = state.slice
            start_time = slice_state.start_time
            end_time = slice_state.end_time
            if start_time and end_time:
                start_time_as_string = '_t%.2f' % start_time[0]
                end_time_as_string = '_T%.2f' % end_time[0]
            else:
                start_time_as_string = ""
                end_time_as_string = ""
            output_name += start_time_as_string + end_time_as_string

        if multi_reduction_type["wavelength_range"]:
            wavelength = state.wavelength
            wavelength_range_string = "_" + str(wavelength.wavelength_low[0]) + "_" + str(
                wavelength.wavelength_high[0])
            output_name += wavelength_range_string

    # Add a suffix if the user has specified one
    if user_specified_output_name_suffix:
        output_name += user_specified_output_name_suffix
        output_base_name += user_specified_output_name_suffix

    # Additional suffix. This will be a suffix for data like the partial output workspace
    if suffix:
        output_name += suffix
        output_base_name += suffix

    return output_name, output_base_name


def get_base_name_from_multi_period_name(workspace_name):
    """
    Gets a base name from a multiperiod name. The multiperiod name is NAME_xxx and the base name is NAME

    :param workspace_name: a workspace name string
    :return: the base name
    """
    multi_period_workspace_form = "_[0-9]+$"
    if re.search(multi_period_workspace_form, workspace_name) is not None:
        return re.sub(multi_period_workspace_form, "", workspace_name)
    else:
        raise RuntimeError("The workspace name {0} seems to not be part of a "
                           "multi-period workspace.".format(workspace_name))


def sanitise_instrument_name(instrument_name):
    """
    Sanitises instrument names which have been obtained from an instrument on a workspace.

    Unfortunately the instrument names are sometimes truncated or extended. This is possible since they are strings
    and not types.
    :param instrument_name: a instrument name string
    :return: a sanitises instrument name string
    """
    instrument_name_upper = instrument_name.upper()
    for instrument in SANSInstrument_string_list:
        if re.search(instrument, instrument_name_upper):
            return instrument
    return instrument_name


def get_facility(instrument):
    if instrument in SANSInstrument_enum_list:
        return SANSFacility.ISIS
    else:
        return SANSFacility.NoFacility


# ----------------------------------------------------------------------------------------------------------------------
# Other
# ----------------------------------------------------------------------------------------------------------------------
def instrument_name_correction(instrument_name):
    return SANS2D if instrument_name == ALTERNATIVE_SANS2D_NAME else instrument_name


def get_instrument(instrument_name):
    instrument_name = instrument_name.upper()
    try:
        return SANSInstrument_string_as_key_NoInstrument[instrument_name]
    except KeyError:
        pass


# ----------------------------------------------------------------------------------------------------------------------
# Hashing + ADS
# ----------------------------------------------------------------------------------------------------------------------
def get_state_hash_for_can_reduction(state, reduction_mode, partial_type=None):
    """
    Creates a hash for a (modified) state object.

    Note that we need to modify the state object to exclude elements which are not relevant for the can reduction.
    This is primarily the setting of the sample workspaces. This is the only place where we directly alter the value
    of a state object in the entire reduction workflow. Note that we are not changing the
    :param state: a SANSState object.
    :param reduction_mode: the reduction mode, here it can be LAB or HAb
    :param partial_type: if it is a partial type, then it needs to be specified here.
    :return: the hash of the state
    """
    def remove_sample_related_information(full_state):
        state_to_hash = deepcopy(full_state)

        # Data
        state_to_hash.data.sample_scatter = EMPTY_NAME
        state_to_hash.data.sample_scatter_period = ALL_PERIODS
        state_to_hash.data.sample_transmission = EMPTY_NAME
        state_to_hash.data.sample_transmission_period = ALL_PERIODS
        state_to_hash.data.sample_direct = EMPTY_NAME
        state_to_hash.data.sample_direct_period = ALL_PERIODS
        state_to_hash.data.sample_scatter_run_number = 1

        # Save
        state_to_hash.save.user_specified_output_name = ""

        return state_to_hash
    new_state = remove_sample_related_information(state)
    new_state_serialized = new_state.property_manager
    new_state_serialized = json.dumps(new_state_serialized, sort_keys=True, indent=4)

    # Add a tag for the reduction mode
    state_string = str(new_state_serialized)
    if reduction_mode is ISISReductionMode.LAB:
        state_string += "LAB"
    elif reduction_mode is ISISReductionMode.HAB:
        state_string += "HAB"
    else:
        raise RuntimeError("Only LAB and HAB reduction modes are allowed at this point."
                           " {} was provided".format(reduction_mode))

    # If we are dealing with a partial output workspace, then mark it as such
    if partial_type is OutputParts.Count:
        state_string += "counts"
    elif partial_type is OutputParts.Norm:
        state_string += "norm"
    elif partial_type is TransmissionType.Calculated:
        state_string += "calculated_transmission"
    elif partial_type is TransmissionType.Unfitted:
        state_string += "unfitted_transmission"
    return str(get_hash_value(state_string))


def get_workspace_from_ads_based_on_hash(hash_value):
    for workspace in get_ads_workspace_references():
        if has_hash(REDUCED_CAN_TAG, hash_value, workspace):
            return workspace


def get_reduced_can_workspace_from_ads(state, output_parts, reduction_mode):
    """
    Get the reduced can workspace from the ADS if it exists else nothing

    :param state: a SANSState object.
    :param output_parts: if true then search also for the partial workspaces
    :param reduction_mode: the reduction mode which at this point is either HAB or LAB
    :return: a reduced can object or None.
    """
    # Get the standard reduced can workspace)
    hashed_state = get_state_hash_for_can_reduction(state, reduction_mode)
    reduced_can = get_workspace_from_ads_based_on_hash(hashed_state)
    reduced_can_count = None
    reduced_can_norm = None
    if output_parts:
        hashed_state_count = get_state_hash_for_can_reduction(state, reduction_mode, OutputParts.Count)
        reduced_can_count = get_workspace_from_ads_based_on_hash(hashed_state_count)
        hashed_state_norm = get_state_hash_for_can_reduction(state, reduction_mode, OutputParts.Norm)
        reduced_can_norm = get_workspace_from_ads_based_on_hash(hashed_state_norm)
    return reduced_can, reduced_can_count, reduced_can_norm


def get_transmission_workspaces_from_ads(state, reduction_mode):
    """
        Get the reduced can transmission workspace from the ADS if it exists else nothing

        :param state: a SANSState object.
        :param reduction_mode: the reduction mode which at this point is either HAB or LAB
        :return: a reduced transmission can object or None.
        """
    hashed_state = get_state_hash_for_can_reduction(state, reduction_mode, TransmissionType.Calculated)
    calculated_transmission = get_workspace_from_ads_based_on_hash(hashed_state)
    hashed_state = get_state_hash_for_can_reduction(state, reduction_mode, TransmissionType.Unfitted)
    unfitted_transmission = get_workspace_from_ads_based_on_hash(hashed_state)
    return calculated_transmission, unfitted_transmission


def write_hash_into_reduced_can_workspace(state, workspace, reduction_mode, partial_type=None):
    """
    Writes the state hash into a reduced can workspace.

    :param state: a SANSState object.
    :param workspace: a reduced can workspace
    :param reduction_mode: the reduction mode
    :param partial_type: if it is a partial type, then it needs to be specified here.
    """
    hashed_state = get_state_hash_for_can_reduction(state, reduction_mode, partial_type=partial_type)
    set_hash(REDUCED_CAN_TAG, hashed_state, workspace)


def does_can_workspace_exist_on_ads(can_workspace):
    """
    Checks if a can workspace already exists on the ADS, based on the stored hash

    :param can_workspace: a handle to the can workspace
    :return: True if the workspace exists on the ADS else False
    """
    if not has_tag(REDUCED_CAN_TAG, can_workspace):
        return False

    hash_value_to_compare = get_tag(REDUCED_CAN_TAG, can_workspace)

    for workspace in get_ads_workspace_references():
        if has_hash(REDUCED_CAN_TAG, hash_value_to_compare, workspace):
            return True
    return False


def get_bank_for_spectrum_number(spectrum_number, instrument):
    """
    This is not very nice since we have to hard-code some instrument information here. But at the moment there is no
    other (efficient and easy) way to check on which detector the spectrum is living.

    The solution here is ugly and should be improved in the future. It is uses the same approach as in the old
    framework.
    :param spectrum_number: The spectrum number to check
    :param instrument: the SANS instrument
    :returns: either LAB or HAB
    """
    detector = DetectorType.LAB
    if instrument is SANSInstrument.LOQ:
        if 16387 <= spectrum_number <= 17784:
            detector = DetectorType.HAB
    elif instrument is SANSInstrument.LOQ:
        if 36873 <= spectrum_number <= 73736:
            detector = DetectorType.HAB
    return detector


# ----------------------------------------------------------------------------------------------------------------------
# Utility functions - functions to perform non-SANS specific operations
# ----------------------------------------------------------------------------------------------------------------------
def check_for_bytes(value, encoding="utf-8"):
    """
    A function to decode a bytes class into a string. In Python3, behaviour of str and bytes classes diverged.
    There are cases when data intended to be a string can be passed around as bytes, due to legacy Python2 code.
    While we will try to correctly encode/decode data as we begin to handle it, there may be missed cases.
    This function should be used to confirm that we have a string when we expect a string.
    :param value: the variable to convert to a string. This may be *str* or *bytes*. Raises TypeError if other type.
    :param encoding: optional str. Denotes the encoding of the data.
    :return: value, now of type *str*
    """
    if isinstance(value, str):
        return value
    elif isinstance(value, bytes):
        return value.decode(encoding)
    else:
        raise TypeError("{} is type {}, not str or bytes.".format(value, type(value)))
