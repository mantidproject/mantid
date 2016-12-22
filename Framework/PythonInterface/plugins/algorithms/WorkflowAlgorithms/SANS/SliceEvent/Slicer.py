""" Performs slicing on an event workspace"""

from abc import (ABCMeta, abstractmethod)
from mantid.dataobjects import Workspace2D

from SANS2.Common.SANSFunctions import (get_charge_and_time, create_unmanaged_algorithm)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSEnumerations import (SANSInstrument)


def slice_by_time(workspace, start_time=None, stop_time=None):
    """
    Performs a time slice

    :param workspace: the workspace to slice.
    :param start_time: the start time of the slice.
    :param stop_time: the stop time of the slice.
    :return: the sliced workspace.
    """
    filter_name = "FilterByTime"
    filter_options = {SANSConstants.input_workspace: workspace,
                      SANSConstants.output_workspace: SANSConstants.dummy}
    if start_time:
        filter_options.update({'StartTime': start_time})
    if stop_time:
        filter_options.update({'StopTime': stop_time})

    filter_alg = create_unmanaged_algorithm(filter_name, **filter_options)
    filter_alg.execute()
    return filter_alg.getProperty(SANSConstants.output_workspace).value


def get_scaled_workspace(workspace, factor):
    """
    Scales a workspace by a specified factor.

    :param workspace: the workspace to scale.
    :param factor: the scale factor.
    :return: the scaled workspace.
    """
    scale_name = "Scale"
    scale_options = {SANSConstants.input_workspace: workspace,
                     SANSConstants.output_workspace: SANSConstants.dummy,
                     "Factor": factor}
    scale_alg = create_unmanaged_algorithm(scale_name, **scale_options)
    scale_alg.execute()
    return scale_alg.getProperty(SANSConstants.output_workspace).value


class Slicer(object):
    __metaclass__ = ABCMeta

    def __init__(self):
        super(Slicer, self).__init__()

    @abstractmethod
    def create_slice(self, workspace, slice_info):
        pass


class NullSlicer(Slicer):
    __metaclass__ = ABCMeta

    def __init__(self):
        super(NullSlicer, self).__init__()

    def create_slice(self, workspace, slice_info):
        slice_factor = 1.0
        return workspace, slice_factor


class ISISSlicer(Slicer):
    __metaclass__ = ABCMeta

    def __init__(self):
        super(ISISSlicer, self).__init__()

    def create_slice(self, workspace, slice_info):
        #Get the slice limits
        start_time = slice_info.start_time
        end_time = slice_info.end_time

        # If there are no slice limits specified, then
        if start_time is None or end_time is None or len(start_time) == 0 or len(end_time) == 0:
            return workspace, 1.0

        if len(start_time) > 1 or len(end_time) > 1:
            raise("Slicer: There seem to be too many start or end values for slicing present. Can have only 1 "
                  "but found {0} and {1} for the start and end time, respectively.".format(start_time, end_time))

        # Slice the workspace
        total_charge, total_time = get_charge_and_time(workspace)
        sliced_workspace = slice_by_time(workspace, start_time[0], end_time[0])
        partial_charge, partial_time = get_charge_and_time(sliced_workspace)

        # The slice factor
        slice_factor = partial_charge / total_charge
        return sliced_workspace, slice_factor


class SliceEventFactory(object):
    def __init__(self):
        super(SliceEventFactory, self).__init__()

    @staticmethod
    def create_slicer(state, workspace):
        """
        Provides the appropriate slicer.

        :param state: a SANSState object
        :param workspace: the workspace to slice
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
             instrument is SANSInstrument.SANS2D: # noqa
            slicer = ISISSlicer()
        else:
            slicer = NullSlicer()
            RuntimeError("SliceEventFactory: Other instruments are not implemented yet.")
        return slicer
