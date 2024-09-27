# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the main State object."""

import copy
import json

from sans_core.common.enums import SANSFacility
from sans_core.state.JsonSerializable import JsonSerializable
from sans_core.state.StateObjects.StateAdjustment import StateAdjustment
from sans_core.state.StateObjects.StateCompatibility import get_compatibility_builder, StateCompatibility

# Note that the compatibility state is not part of the new reduction chain, but allows us to accurately compare
# results obtained via the old and new reduction chain
from sans_core.state.StateObjects.StateConvertToQ import StateConvertToQ
from sans_core.state.StateObjects.StateData import StateData
from sans_core.state.StateObjects.StateMaskDetectors import StateMask
from sans_core.state.StateObjects.StateMoveDetectors import StateMove
from sans_core.state.StateObjects.StateReductionMode import StateReductionMode
from sans_core.state.StateObjects.StateSave import StateSave
from sans_core.state.StateObjects.StateScale import StateScale
from sans_core.state.StateObjects.StateSliceEvent import StateSliceEvent
from sans_core.state.StateObjects.StateWavelength import StateWavelength
from sans_core.state.StateObjects.state_instrument_info import StateInstrumentInfo
from sans_core.state.StateObjects.StateBackgroundSubtraction import StateBackgroundSubtraction
from sans_core.state.automatic_setters import automatic_setters


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class AllStates(metaclass=JsonSerializable):
    def __init__(self):
        super(AllStates, self).__init__()
        self.data: StateData = StateData()
        self.move: StateMove = StateMove()
        self.instrument_info: StateInstrumentInfo = StateInstrumentInfo()
        self.reduction: StateReductionMode = StateReductionMode()
        self.slice: StateSliceEvent = StateSliceEvent()
        self.mask: StateMask = StateMask()
        self.wavelength: StateWavelength = StateWavelength()
        self.save: StateSave = StateSave()
        self.scale: StateScale = StateScale()
        self.adjustment: StateAdjustment = StateAdjustment()
        self.convert_to_q: StateConvertToQ = StateConvertToQ()
        self.compatibility: StateCompatibility = StateCompatibility()
        self.background_subtraction: StateBackgroundSubtraction = StateBackgroundSubtraction()

    def validate(self):
        is_invalid = dict()

        # Make sure that the substates are contained
        if not self.data:
            is_invalid.update("State: The state object needs to include a StateData object.")
        if not self.move:
            is_invalid.update("State: The state object needs to include a StateMove object.")
        if not self.reduction:
            is_invalid.update("State: The state object needs to include a StateReduction object.")
        if not self.slice:
            is_invalid.update("State: The state object needs to include a StateSliceEvent object.")
        if not self.mask:
            is_invalid.update("State: The state object needs to include a StateMask object.")
        if not self.wavelength:
            is_invalid.update("State: The state object needs to include a StateWavelength object.")
        if not self.save:
            is_invalid.update("State: The state object needs to include a StateSave object.")
        if not self.scale:
            is_invalid.update("State: The state object needs to include a StateScale object.")
        if not self.adjustment:
            is_invalid.update("State: The state object needs to include a StateAdjustment object.")
        if not self.convert_to_q:
            is_invalid.update("State: The state object needs to include a StateConvertToQ object.")
        if not self.background_subtraction:
            is_invalid.update("State: The state object needs to include a StateBackgroundSubtraction object.")

        # We don't enforce a compatibility mode, we just create one if it does not exist
        if not self.compatibility:
            if self.data:
                self.compatibility = get_compatibility_builder(self.data).build()

        if is_invalid:
            raise ValueError("State: There is an issue with your input. See: {0}".format(json.dumps(is_invalid)))


class AllStatesBuilder(object):
    @automatic_setters(AllStates)
    def __init__(self):
        super(AllStatesBuilder, self).__init__()
        self.state = AllStates()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_all_states_builder(data_info):
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return AllStatesBuilder()
    else:
        raise NotImplementedError(
            "SANSStateBuilder: Could not find any valid state builder for the " "specified SANSStateData object {0}".format(str(data_info))
        )
