# pylint: disable=too-few-public-methods

import json
import pickle
import inspect
from SANS2.State.SANSStateBase import (SANSStateBase, TypedParameter, sans_parameters)
from SANS2.State.SANSStateData import SANSStateData
from SANS2.State.SANSStateMove import SANSStateMove


# -----------------------------------------------
# Validator for sub states
# -----------------------------------------------
def validator_sub_state(sub_state):
    is_valid = True
    try:
        sub_state.validate()
    except ValueError:
        is_valid = False
    return is_valid


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

    def __init__(self):
        super(SANSStateISIS, self).__init__()

    def validate(self):
        is_invalid = dict()

        # Make sure that the substates are contained
        if not self.data:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateData object.")
        if not self.move:
            is_invalid.update("SANSStateISIS: The state object needs to include a SANSStateMove object.")

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
                    is_invalid.update({descriptor_name: pickle.dumps(err.message)})

        if is_invalid:
            raise ValueError("SANSState: There is an issue with your in put. See: {0}".format(json.dumps(is_invalid)))
