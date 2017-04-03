""" Performs slicing on an event workspace"""

from __future__ import (absolute_import, division, print_function)
from abc import (ABCMeta, abstractmethod)
from six import with_metaclass
from mantid.dataobjects import Workspace2D

from sans.common.general_functions import (get_charge_and_time, create_unmanaged_algorithm)
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (SANSInstrument, DataType)


def slice_by_time(workspace, start_time=None, stop_time=None):
    """
    Performs a time slice

    :param workspace: the workspace to slice.
    :param start_time: the start time of the slice.
    :param stop_time: the stop time of the slice.
    :return: the sliced workspace.
    """
    filter_name = "FilterByTime"
    filter_options = {"InputWorkspace": workspace,
                      "OutputWorkspace": EMPTY_NAME}
    if start_time:
        filter_options.update({'StartTime': start_time})
    if stop_time:
        filter_options.update({'StopTime': stop_time})

    filter_alg = create_unmanaged_algorithm(filter_name, **filter_options)
    filter_alg.execute()
    return filter_alg.getProperty("OutputWorkspace").value


def get_scaled_workspace(workspace, factor):
    """
    Scales a workspace by a specified factor.

    :param workspace: the workspace to scale.
    :param factor: the scale factor.
    :return: the scaled workspace.
    """
    single_valued_name = "CreateSingleValuedWorkspace"
    single_valued_options = {"OutputWorkspace": EMPTY_NAME,
                             "DataValue": factor}
    single_valued_alg = create_unmanaged_algorithm(single_valued_name, **single_valued_options)
    single_valued_alg.execute()
    single_valued_workspace = single_valued_alg.getProperty("OutputWorkspace").value

    multiply_name = "Multiply"
    multiply_options = {"LHSWorkspace": workspace,
                        "RHSWorkspace": single_valued_workspace,
                        "OutputWorkspace": EMPTY_NAME}
    multiply_alg = create_unmanaged_algorithm(multiply_name, **multiply_options)
    multiply_alg.execute()
    return multiply_alg.getProperty("OutputWorkspace").value


class Slicer(with_metaclass(ABCMeta, object)):
    def __init__(self):
        super(Slicer, self).__init__()

    @abstractmethod
    def create_slice(self, workspace, slice_info):
        pass


class NullSlicer(Slicer):
    def __init__(self):
        super(NullSlicer, self).__init__()

    def create_slice(self, workspace, slice_info):
        slice_factor = 1.0
        return workspace, slice_factor


class ISISSlicer(Slicer):
    def __init__(self, data_type):
        super(ISISSlicer, self).__init__()
        self._data_type = data_type

    def create_slice(self, workspace, slice_info):
        # Get the slice limits
        start_time = slice_info.start_time
        end_time = slice_info.end_time

        # If there are no slice limits specified, then
        if start_time is None or end_time is None or len(start_time) == 0 or len(end_time) == 0:
            return workspace, 1.0

        if len(start_time) > 1 or len(end_time) > 1:
            raise RuntimeError("Slicer: There seem to be too many start or end values for slicing present. "
                               "Can have only 1 but found {0} and {1} for the start and end time,"
                               " respectively.".format(len(start_time), len(end_time)))

        start_time = start_time[0]
        end_time = end_time[0]

        # Slice the workspace
        total_charge, total_time = get_charge_and_time(workspace)

        # If we are dealing with a Can reduction then the slice times are -1
        if self._data_type is DataType.Can:
            start_time = -1.
            end_time = -1.

        # If the start_time is -1 or the end_time is -1 then use the limit of that slice
        if start_time == -1.:
            start_time = 0.0
        if end_time == -1.:
            end_time = total_time + 0.001

        sliced_workspace = slice_by_time(workspace, start_time, end_time)
        partial_charge, partial_time = get_charge_and_time(sliced_workspace)

        # The slice factor
        slice_factor = partial_charge / total_charge
        return sliced_workspace, slice_factor


class SliceEventFactory(object):
    def __init__(self):
        super(SliceEventFactory, self).__init__()

    @staticmethod
    def create_slicer(state, workspace, data_type):
        """
        Provides the appropriate slicer.

        :param state: a SANSState object
        :param workspace: the workspace to slice
        :param data_type: the data type, ie if the Sample or Can
        :return: the corresponding slicer
        """
        data_info = state.data
        instrument = data_info.instrument

        # The factory is currently set up to
        # 1. Use NullSlicer when we have a histogram
        # 3. Use ISISSlicer if we have an ISIS instrument and event data
        # 3. else raise error
        if isinstance(workspace, Workspace2D):
            slicer = NullSlicer()
        elif instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or \
             instrument is SANSInstrument.SANS2D:  # noqa
            slicer = ISISSlicer(data_type)
        else:
            raise RuntimeError("SliceEventFactory: Other instruments are not implemented yet.")
        return slicer
