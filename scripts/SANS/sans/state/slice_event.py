""" Defines the state of the event slices which should be reduced."""

from __future__ import (absolute_import, division, print_function)
import json
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, FloatListParameter)
from sans.state.state_functions import (is_pure_none_or_not_none, validation_message)
from sans.common.enums import SANSInstrument
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateSliceEvent(StateBase):
    start_time = FloatListParameter()
    end_time = FloatListParameter()

    def __init__(self):
        super(StateSliceEvent, self).__init__()

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

            # Check that end_time is not smaller than start_time
            if not is_smaller(self.start_time, self.end_time):
                entry = validation_message("Start time larger than end time.",
                                           "Make sure that the start time is not smaller than the end time.",
                                           {"start_time": self.start_time,
                                            "end_time": self.end_time})
                is_invalid.update(entry)

        if is_invalid:
            raise ValueError("StateSliceEvent: The provided inputs are illegal. "
                             "Please see: {}".format(json.dumps(is_invalid)))


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
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateSliceEventBuilder()
    else:
        raise NotImplementedError("StateSliceEventBuilder: Could not find any valid slice builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
