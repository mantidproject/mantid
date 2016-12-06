""" Defines the main SANSState object."""

# pylint: disable=too-few-public-methods

import json
import pickle
import inspect
from SANS2.State.SANSStateBase import (SANSStateBase, TypedParameter, sans_parameters, validator_sub_state)
from SANS2.State.SANSStateData import SANSStateData
from SANS2.State.SANSStateMove import SANSStateMove
from SANS2.State.SANSStateReduction import SANSStateReduction
from SANS2.State.SANSStateSliceEvent import SANSStateSliceEvent
from SANS2.State.SANSStateMask import SANSStateMask
from SANS2.State.SANSStateWavelength import SANSStateWavelength
from SANS2.State.SANSStateSave import SANSStateSave
from SANS2.State.SANSStateAdjustment import SANSStateAdjustment
from SANS2.State.SANSStateScale import SANSStateScale
from SANS2.State.SANSStateConvertToQ import SANSStateConvertToQ


# ------------------------------------------------
# SANSState
# ------------------------------------------------
class SANSState(object):
    pass


# -----------------------------------------------
# SANSState class
# -----------------------------------------------
@sans_parameters
class SANSStateISIS(SANSStateBase, SANSState):
    data = TypedParameter(SANSStateData, validator_sub_state)
    move = TypedParameter(SANSStateMove, validator_sub_state)
    reduction = TypedParameter(SANSStateReduction, validator_sub_state)
    slice = TypedParameter(SANSStateSliceEvent, validator_sub_state)
    mask = TypedParameter(SANSStateMask, validator_sub_state)
    wavelength = TypedParameter(SANSStateWavelength, validator_sub_state)
    save = TypedParameter(SANSStateSave, validator_sub_state)
    scale = TypedParameter(SANSStateScale, validator_sub_state)
    adjustment = TypedParameter(SANSStateAdjustment, validator_sub_state)
    convert_to_q = TypedParameter(SANSStateConvertToQ, validator_sub_state)

    def __init__(self):
        super(SANSStateISIS, self).__init__()

    def validate(self):
        is_invalid = dict()

        # Make sure that the substates are contained
        if not self.data:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateData object.")
        if not self.move:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateMove object.")
        if not self.reduction:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateReduction object.")
        if not self.slice:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateSliceEvent object.")
        if not self.mask:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateMask object.")
        if not self.wavelength:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateWavelength object.")
        if not self.save:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateSave object.")
        if not self.scale:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateScale object.")
        if not self.adjustment:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateAdjustment object.")
        if not self.convert_to_q:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateConvertToQ object.")

        if is_invalid:
            raise ValueError("SANSState: There is an issue with your in put. See: {0}".format(json.dumps(is_invalid)))

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
            raise ValueError("SANSState: There is an issue with your in put. See: {0}".format(json.dumps(is_invalid)))
