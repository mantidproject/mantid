from SANSStateBase import (SANSStateBase, TypedParameter, is_not_none, is_positive, PropertyManagerConverter)


# -----------------------------------------------
#  SANSStateData Setup for ISIS
# -----------------------------------------------
class SANSStatePrototype(SANSStateBase):
    parameter1 = TypedParameter("parameter1", str, is_not_none)
    parameter2 = TypedParameter("parameter2", int, is_positive, 1)

    def __init__(self):
        super(SANSStatePrototype, self).__init__()
        self.converter = PropertyManagerConverter()

    @property
    def property_manager(self):
        # Once algorithms accept Property Managers we can use the version below
        # return self.converter.convert_state_to_property_manager(self)
        # otherwise we use this version
        return self.converter.convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        self.converter.set_state_from_property_manager(self, value)

    def validate(self):
        is_valid = dict()
        if self.parameter1 is None:
            is_valid.update({"parameter1": "Parameter may not be none"})
        if not is_valid:
            raise ValueError("SANSStatePropertyType: Parameters are not valid")
