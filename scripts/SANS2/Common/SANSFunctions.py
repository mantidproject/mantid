# pylint: disable=invalid-name

from math import (acos, sqrt, degrees)
from mantid.api import AlgorithmManager
from mantid.kernel import (DateAndTime)


# -------------------------------------------
# Free functions
# -------------------------------------------
def get_log_value(run, log_name, log_type):
    # Find the logs.  There are two options here. Either the log is a scalar or a vector.
    # In the case of a scalar there is not much left to do.
    # In the case of a vector we select the first element whose time_stamp is after the start time of the run
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
    alg = AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    alg.setChild(True)
    for key, value in kwargs.items():
        alg.setProperty(key, value)
    return alg


def quaternion_to_angle_and_axis(quaternion):
    """
    Converts a quaterion to an angle + an axis

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
