""" The elements of this module contain various general-purpose functions for the SANS reduction framework."""

# pylint: disable=invalid-name

from math import (acos, sqrt, degrees)
from mantid.api import AlgorithmManager, AnalysisDataService
from mantid.kernel import (DateAndTime)
from sans.common.constants import SANS_FILE_TAG
from sans.common.log_tagger import (get_tag, has_tag, set_tag)


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
        for key in log_results.keys():
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
    for key, value in kwargs.items():
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
