""" Defines the main State object."""

# pylint: disable=too-few-public-methods

from __future__ import (absolute_import, division, print_function)
import json
import pickle
import inspect
import copy
from sans.common.enums import SANSInstrument
from sans.state.state_base import (StateBase, TypedParameter,
                                   rename_descriptor_names, validator_sub_state)
from sans.state.data import StateData
from sans.state.move import StateMove
from sans.state.reduction_mode import StateReductionMode
from sans.state.slice_event import StateSliceEvent
from sans.state.mask import StateMask
from sans.state.wavelength import StateWavelength
from sans.state.save import StateSave
from sans.state.adjustment import StateAdjustment
from sans.state.scale import StateScale
from sans.state.convert_to_q import StateConvertToQ
from sans.state.automatic_setters import (automatic_setters)

# Note that the compatibiliy state is not part of the new reduction chain, but allows us to accurately compare
# results obtained via the old and new reduction chain
from sans.state.compatibility import (StateCompatibility, get_compatibility_builder)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class State(StateBase):
    data = TypedParameter(StateData, validator_sub_state)
    move = TypedParameter(StateMove, validator_sub_state)
    reduction = TypedParameter(StateReductionMode, validator_sub_state)
    slice = TypedParameter(StateSliceEvent, validator_sub_state)
    mask = TypedParameter(StateMask, validator_sub_state)
    wavelength = TypedParameter(StateWavelength, validator_sub_state)
    save = TypedParameter(StateSave, validator_sub_state)
    scale = TypedParameter(StateScale, validator_sub_state)
    adjustment = TypedParameter(StateAdjustment, validator_sub_state)
    convert_to_q = TypedParameter(StateConvertToQ, validator_sub_state)
    compatibility = TypedParameter(StateCompatibility, validator_sub_state)

    def __init__(self):
        super(State, self).__init__()

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

        # We don't enforce a compatibility mode, we just create one if it does not exist
        if not self.compatibility:
            if self.data:
                self.compatibility = get_compatibility_builder(self.data).build()

        if is_invalid:
            raise ValueError("State: There is an issue with your in put. See: {0}".format(json.dumps(is_invalid)))

        # Check the attributes themselves
        is_invalid = {}
        for descriptor_name, descriptor_object in inspect.getmembers(type(self)):
            if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
                try:
                    attr = getattr(self, descriptor_name)
                    attr.validate()
                except ValueError as err:
                    is_invalid.update({descriptor_name: pickle.dumps(str(err))})

        if is_invalid:
            raise ValueError("State: There is an issue with your in put. See: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateBuilder(object):
    @automatic_setters(State)
    def __init__(self):
        super(StateBuilder, self).__init__()
        self.state = State()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_state_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateBuilder()
    else:
        raise NotImplementedError("SANSStateBuilder: Could not find any valid state builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
