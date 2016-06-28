import json
import pickle
import inspect
from SANS2.State.SANSStateBase import (SANSStateBase, TypedParameter, sans_parameters)
from SANS2.State.SANSStateSerializer import(convert_state_to_dict, set_state_from_property_manager)
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
            raise ValueError("SANSState: There is an issue with your in put. See: {}".format(json.dumps(is_invalid)))

        # Check the attributes themselves
        is_invalid = {}
        for descriptor_name, descriptor_object in inspect.getmembers(type(self)):
            if inspect.isdatadescriptor(descriptor_object) and isinstance(descriptor_object, TypedParameter):
                try:
                    attr = getattr(self, descriptor_name)
                    attr.validate()
                except ValueError as e:
                    is_invalid.update(descriptor_name, pickle.dumps(e.message))

        if is_invalid:
            raise ValueError("SANSState: There is an issue with your in put. See: {}".format(json.dumps(is_invalid)))

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

