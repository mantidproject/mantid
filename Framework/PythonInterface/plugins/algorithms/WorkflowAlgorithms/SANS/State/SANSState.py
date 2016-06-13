import json
import pickle
import inspect
from mantid.kernel import PropertyManager
from State.SANSStateBase import (SANSStateBase, TypedParameter, convert_state_to_dict,
                                 set_state_from_property_manager)
from SANSStateData import SANSStateData


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
class SANSStateISIS(SANSStateBase, SANSState):
    data = TypedParameter("data", SANSStateData, validator_sub_state)

    def __init__(self):
        super(SANSStateISIS, self).__init__()

    def validate(self):
        is_invalid = dict()
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


# ----------------------------------------------------
# SANSStateFactory method
# ----------------------------------------------------
def create_correct_sans_state_for_dictionary_input(input):
    pass