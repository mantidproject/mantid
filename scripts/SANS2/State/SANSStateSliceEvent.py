""" Defines the state of the event slices which should be reduced."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, sans_parameters, FloatListParameter)


# ------------------------------------------------
# SANSStateReduction
# ------------------------------------------------
class SANSStateSliceEvent(object):
    pass


# -----------------------------------------------
#  SANSStateReduction for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateSliceEventISIS(SANSStateSliceEvent, SANSStateBase):
    start_time = FloatListParameter()
    end_time = FloatListParameter()

    def __init__(self):
        super(SANSStateSliceEventISIS, self).__init__()

    def validate(self):
        is_invalid = dict()
        if self.start_time and not self.end_time or not self.start_time and self.end_time:
            is_invalid.update(
                {"start_time or end_time": "Either only the start_time {0} or the end_time {1} "
                                           "was set.".format(len(self.start_time), len(self.end_time))})
        if self.start_time and self.end_time:
            # The length of start_time and end_time needs to be identical
            if len(self.start_time) != len(self.end_time):
                is_invalid.update({"start_time": "The length of start_time is {} and the length of end_time "
                                                 "is {}, but they have to be identical".format(len(self.start_time),
                                                                                               len(self.end_time))})

            # Each entry in start_time and end_time must be a float
            if len(self.start_time) == len(self.end_time) and len(self.start_time) > 0:
                for item in range(0, len(self.start_time)):
                    for element1, element2 in zip(self.start_time, self.end_time):
                        if not isinstance(element1, float) or not isinstance(element2, float):
                            is_invalid.update(
                                {"start_time or end_time": "An input for the time of flight for event slicing is not a"
                                                           " floating point value. Start_time is {0} and end_time is "
                                                           "{1}".format(element1, element2)})  # noqa

            # Check that the entries are monotonically increasing. We don't want 12, 24, 22
            if len(self.start_time) > 1 and not monotonically_increasing(self.start_time):
                is_invalid.update(
                    {"start_time": "The values in start_time don't seem to be monotonically increasing."})
            if len(self.end_time) > 1 and not monotonically_increasing(self.end_time):
                is_invalid.update(
                    {"end_time": "The values in end_time don't seem to be monotonically increasing."})

            # Check that end_time is not smaller than start_time
            if not is_smaller(self.start_time, self.end_time):
                is_invalid.update(
                    {"start_time and end_time": "The end_time values seem to be smaller than the start_time values."})

        if is_invalid:
            raise ValueError("SANSStateSliceEvent: The provided inputs are illegal. "
                             "Please see: {}".format(json.dumps(is_invalid)))


def monotonically_increasing(to_check):
    return all(x <= y for x, y in zip(to_check, to_check[1:]))


def is_smaller(smaller, larger):
    return all(x <= y for x, y in zip(smaller, larger))

# -----------------------------------------------
# SANSStateSliceEvent setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateSliceEvent and SANSStateBase and fulfill its contract.
# -----------------------------------------------
