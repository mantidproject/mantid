from SANS2.State.SANSStateBase import (SANSStateBase, StringParameter, PositiveIntegerParameter, sans_parameters)
from SANS2.State.SANSStateSerializer import(convert_state_to_dict, set_state_from_property_manager)


# -----------------------------------------------
#  SANSStateData Setup for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStatePrototype(SANSStateBase):
    parameter1 = StringParameter()
    parameter2 = PositiveIntegerParameter()

    def __init__(self):
        super(SANSStatePrototype, self).__init__()

    @property
    def property_manager(self):
        # Once algorithms accept Property Managers we can use the version below
        # return self.converter.convert_state_to_property_manager(self)
        # otherwise we use this version
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        is_invalid = dict()
        if self.parameter1 is None:
            is_invalid.update({"parameter1": "Parameter may not be none"})
        if is_invalid:
            raise ValueError("SANSStatePropertyType: Parameters are not valid")
