# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the background which should be subtracted after the main reduction."""

import json
import copy

from sans.state.JsonSerializable import JsonSerializable
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import is_pure_none_or_not_none, validation_message
from sans.common.enums import SANSFacility


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class StateBackgroundSubtraction(metaclass=JsonSerializable):
    def __init__(self):
        super(StateBackgroundSubtraction, self).__init__()

        self.workspace = None  # : Str()
        self.scale_factor = None  # : Float()

    def validate(self):
        is_invalid = dict()

        if not is_pure_none_or_not_none([self.workspace, self.scale_factor]):
            entry = validation_message(
                "Missing background subtraction parameters.",
                "Make sure that either both or none are set.",
                {"BackGroundWorkspace": self.workspace, "ScaleFactor": self.scale_factor},
            )
            is_invalid.update(entry)

        if is_invalid:
            raise ValueError("StateBackgroundSubtraction: The provided inputs are illegal. Please see: {}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateBackgroundSubtractionBuilder(object):
    @automatic_setters(StateBackgroundSubtraction)
    def __init__(self):
        super(StateBackgroundSubtractionBuilder, self).__init__()
        self.state = StateBackgroundSubtraction()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_background_subtraction_builder(data_info):
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateBackgroundSubtractionBuilder()
    else:
        raise NotImplementedError(
            f"StateBackgroundSubtraction: Could not find any valid background subtraction builder for the specified "
            f"StateData object {data_info}"
        )
