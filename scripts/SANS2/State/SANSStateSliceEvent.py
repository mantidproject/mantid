""" Defines the state of the event slices which should be reduced."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, sans_parameters, FloatListParameter)
from SANS2.State.SANSStateFunctions import (is_pure_none_or_not_none, validation_message)


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

        if not is_pure_none_or_not_none([self.start_time, self.end_time]):
            entry = validation_message("Missing slice times",
                                       "Makes sure that either both or none are set.",
                                       {"start_time": self.start_time,
                                        "end_time": self.end_time})
            is_invalid.update(entry)

        if self.start_time and self.end_time:
            # The length of start_time and end_time needs to be identical
            if len(self.start_time) != len(self.end_time):
                entry = validation_message("Bad relation of start and end",
                                           "Makes sure that the start time is smaller than the end time.",
                                           {"start_time": self.start_time,
                                            "end_time": self.end_time})
                is_invalid.update(entry)

            # Each entry in start_time and end_time must be a float
            if len(self.start_time) == len(self.end_time) and len(self.start_time) > 0:
                for item in range(0, len(self.start_time)):
                    for element1, element2 in zip(self.start_time, self.end_time):
                        if not isinstance(element1, float) or not isinstance(element2, float):
                            entry = validation_message("Bad relation of start and end time entries",
                                                       "The elements need to be floats.",
                                                       {"start_time": self.start_time,
                                                        "end_time": self.end_time})
                            is_invalid.update(entry)

            # Check that the entries are monotonically increasing. We don't want 12, 24, 22
            if len(self.start_time) > 1 and not monotonically_increasing(self.start_time):
                entry = validation_message("Not monotonically increasing start time list",
                                           "Make sure that the start times increase monotonically.",
                                           {"start_time": self.start_time})
                is_invalid.update(entry)

            if len(self.end_time) > 1 and not monotonically_increasing(self.end_time):
                entry = validation_message("Not monotonically increasing end time list",
                                           "Make sure that the end times increase monotonically.",
                                           {"end_time": self.end_time})
                is_invalid.update(entry)

            # Check that end_time is not smaller than start_time
            if not is_smaller(self.start_time, self.end_time):
                entry = validation_message("Start time larger than end time.",
                                           "Make sure that the start time is not smaller than the end time.",
                                           {"start_time": self.start_time,
                                            "end_time": self.end_time})
                is_invalid.update(entry)

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
