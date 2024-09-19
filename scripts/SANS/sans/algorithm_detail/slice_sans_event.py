# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from SANSUtility import _clean_logs
from mantid.dataobjects import Workspace2D
from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.general_functions import append_to_sans_file_tag, get_charge_and_time, create_unmanaged_algorithm
from SANS.sans.common.enums import DataType


def slice_sans_event(state_slice, input_ws, input_ws_monitor, data_type_str="Sample"):
    """
    Takes an event slice from an event workspace
    :param state_slice: The state.slice object
    :param input_ws: The input workspace. If it is an event workspace, then the slice is taken.
                     In case of a Workspace2D the original workspace is returned
    :param input_ws_monitor: The monitor workspace associated with the main input workspace.
    :param data_type_str: The component of the instrument which is to be reduced. Allowed values: ['Sample', 'Can']
    :return: A dict with the following:
             'SliceEventFactor': The factor of the event slicing. This corresponds to the proportion of the
                                 the total proton charge, which the slice corresponds to.
             'OutputWorkspace' : The slice workspace
             'OutputWorkspaceMonitor' : The output monitor workspace which has the correct slice factor applied to it.
    """

    data_type = DataType(data_type_str)

    # This should be removed in the future when cycle 19/1 data is unlikely to be processed by users
    # This prevents time slicing falling over, since we wrap around and get -0
    _clean_logs(ws=input_ws, estimate_logs=True)

    if isinstance(input_ws, Workspace2D):
        sliced_workspace = input_ws
        slice_factor = 1.0
    else:
        sliced_workspace, slice_factor = _create_slice(workspace=input_ws, slice_info=state_slice, data_type=data_type)

    # Scale the monitor accordingly
    slice_monitor = _scale_monitors(slice_factor=slice_factor, input_monitor_ws=input_ws_monitor)

    # Set the outputs
    append_to_sans_file_tag(sliced_workspace, "_sliced")

    to_return = {"OutputWorkspace": sliced_workspace, "SliceEventFactor": slice_factor, "OutputWorkspaceMonitor": slice_monitor}

    return to_return


def _create_slice(workspace, slice_info, data_type):
    # Get the slice limits
    start_time = slice_info.start_time
    end_time = slice_info.end_time

    # If there are no slice limits specified, then
    if start_time is None or end_time is None or len(start_time) == 0 or len(end_time) == 0:
        return workspace, 1.0

    if len(start_time) > 1 or len(end_time) > 1:
        raise RuntimeError(
            "Slicer: There seem to be too many start or end values for slicing present. "
            "Can have only 1 but found {0} and {1} for the start and end time,"
            " respectively.".format(len(start_time), len(end_time))
        )

    start_time = start_time[0]
    end_time = end_time[0]

    # Slice the workspace
    total_charge, total_time = get_charge_and_time(workspace)

    # If we are dealing with a Can reduction then the slice times are -1
    if data_type is DataType.CAN:
        start_time = -1.0
        end_time = -1.0

    # If the start_time is -1 or the end_time is -1 then use the limit of that slice
    if start_time == -1.0:
        start_time = 0.0
    if end_time == -1.0:
        end_time = total_time + 0.001

    sliced_workspace = _slice_by_time(workspace, start_time, end_time)
    partial_charge, partial_time = get_charge_and_time(sliced_workspace)

    # The slice factor
    slice_factor = partial_charge / total_charge
    return sliced_workspace, slice_factor


def _slice_by_time(workspace, start_time=None, stop_time=None):
    """
    Performs a time slice

    :param workspace: the workspace to slice.
    :param start_time: the start time of the slice.
    :param stop_time: the stop time of the slice.
    :return: the sliced workspace.
    """
    filter_name = "FilterByTime"
    filter_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME}
    if start_time:
        filter_options.update({"StartTime": start_time})
    if stop_time:
        filter_options.update({"StopTime": stop_time})

    filter_alg = create_unmanaged_algorithm(filter_name, **filter_options)
    filter_alg.execute()
    return filter_alg.getProperty("OutputWorkspace").value


def _get_scaled_workspace(workspace, factor):
    """
    Scales a workspace by a specified factor.

    :param workspace: the workspace to scale.
    :param factor: the scale factor.
    :return: the scaled workspace.
    """
    single_valued_name = "CreateSingleValuedWorkspace"
    single_valued_options = {"OutputWorkspace": EMPTY_NAME, "DataValue": factor}
    single_valued_alg = create_unmanaged_algorithm(single_valued_name, **single_valued_options)
    single_valued_alg.execute()
    single_valued_workspace = single_valued_alg.getProperty("OutputWorkspace").value

    multiply_name = "Multiply"
    multiply_options = {"LHSWorkspace": workspace, "RHSWorkspace": single_valued_workspace, "OutputWorkspace": EMPTY_NAME}
    multiply_alg = create_unmanaged_algorithm(multiply_name, **multiply_options)
    multiply_alg.execute()
    return multiply_alg.getProperty("OutputWorkspace").value


def _scale_monitors(slice_factor, input_monitor_ws):
    if slice_factor < 1.0:
        input_monitor_ws = _get_scaled_workspace(input_monitor_ws, slice_factor)
    append_to_sans_file_tag(input_monitor_ws, "_sliced")
    return input_monitor_ws
