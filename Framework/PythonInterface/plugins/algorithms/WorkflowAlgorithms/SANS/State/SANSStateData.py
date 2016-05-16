from SANSStateBase import (SANSStateBase, TypedParameter, is_not_none, is_positive, PropertyManagerConverter)


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateData(object):
    pass


# -----------------------------------------------
#  SANSStateData Setup for ISIS
# -----------------------------------------------
class SANSStateDataISIS(SANSStateBase, SANSStateData):
    sample_scatter = TypedParameter("sample_scatter", str, is_not_none)
    sample_scatter_period = TypedParameter("sample_scatter_period", int, is_positive, 1)
    sample_transmission = TypedParameter("sample_transmission", str, is_not_none)
    sample_transmission_period = TypedParameter("sample_transmission_period", int, is_positive, 1)
    sample_direct = TypedParameter("sample_direct", str, is_not_none)
    sample_direct_period = TypedParameter("sample_direct_period", int, is_positive, 1)

    can_scatter = TypedParameter("can_scatter", str, is_not_none)
    can_scatter_period = TypedParameter("can_scatter_period", int, is_positive, 1)
    can_transmission = TypedParameter("can_transmission", str, is_not_none)
    can_transmission_period = TypedParameter("can_transmission_period", int, is_positive, 1)
    can_direct = TypedParameter("can_direct", str, is_not_none)
    can_direct_period = TypedParameter("can_direct_period", int, is_positive, 1)

    def __init__(self):
        super(SANSStateDataISIS, self).__init__()
        self.converter = PropertyManagerConverter()

    @property
    def property_manager(self):
        return self.converter.convert_state_to_property_manager(self)

    @property_manager.setter
    def property_manager(self, value):
        self.converter.set_state_from_property_manager(self, value)

    def validate(self):
        is_valid = dict()

        # A sample scatter must be specified
        if self.sample_scatter is None:
            is_valid.update({"sample_scatter": "The sample scatter workspace must be set."})

        # If the sample transmission/direct was specified, then a sample direct/transmission is required
        if (self.sample_transmission and not self.sample_direct) or \
                (not self.sample_transmission and self.sample_direct):
            is_valid.update({"sample_transmission": "The sample transmission/direct was specified, then the "
                                                    "direct/transmission must be specified as well."})

        # If a can transmission/direct was specified, then the other can entries need to be specified as well.
        # TODO

# -----------------------------------------------
#  SANSStateData Setup for ...
# -----------------------------------------------
