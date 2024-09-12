# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the event slices which should be reduced."""

import json
import copy

from sans.state.JsonSerializable import JsonSerializable
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import is_pure_none_or_not_none, validation_message
from sans.common.enums import SANSFacility


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class StateSliceEvent(metaclass=JsonSerializable):
    def __init__(self):
        super(StateSliceEvent, self).__init__()

        self.start_time = None  # : List[Float]
        self.end_time = None  # : List[Float]
        self.event_slice_str: str = ""

    def validate(self):
        is_invalid = dict()

        if not is_pure_none_or_not_none([self.start_time, self.end_time]):
            entry = validation_message(
                "Missing slice times",
                "Makes sure that either both or none are set.",
                {"start_time": self.start_time, "end_time": self.end_time},
            )
            is_invalid.update(entry)

        if self.start_time and self.end_time:
            # The length of start_time and end_time needs to be identical
            if len(self.start_time) != len(self.end_time):
                entry = validation_message(
                    "Bad relation of start and end",
                    "Makes sure that the start time is smaller than the end time.",
                    {"start_time": self.start_time, "end_time": self.end_time},
                )
                is_invalid.update(entry)

            # Each entry in start_time and end_time must be a float
            if len(self.start_time) == len(self.end_time) and len(self.start_time) > 0:
                for item in range(0, len(self.start_time)):
                    for element1, element2 in zip(self.start_time, self.end_time):
                        if not isinstance(element1, float) or not isinstance(element2, float):
                            entry = validation_message(
                                "Bad relation of start and end time entries",
                                "The elements need to be floats.",
                                {"start_time": self.start_time, "end_time": self.end_time},
                            )
                            is_invalid.update(entry)

            # Check that end_time is not smaller than start_time
            if not is_smaller(self.start_time, self.end_time):
                entry = validation_message(
                    "Start time larger than end time.",
                    "Make sure that the start time is not smaller than the end time.",
                    {"start_time": self.start_time, "end_time": self.end_time},
                )
                is_invalid.update(entry)

        if is_invalid:
            raise ValueError("StateSliceEvent: The provided inputs are illegal. " "Please see: {}".format(json.dumps(is_invalid)))


def monotonically_increasing(to_check):
    return all(x <= y for x, y in zip(to_check, to_check[1:]))


def is_smaller(smaller, larger):
    return all(x <= y for x, y in zip(smaller, larger))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateSliceEventBuilder(object):
    @automatic_setters(StateSliceEvent)
    def __init__(self):
        super(StateSliceEventBuilder, self).__init__()
        self.state = StateSliceEvent()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_slice_event_builder(data_info):
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateSliceEventBuilder()
    else:
        raise NotImplementedError(
            "StateSliceEventBuilder: Could not find any valid slice builder for the " "specified StateData object {0}".format(
                str(data_info)
            )
        )
