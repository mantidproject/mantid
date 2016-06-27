import json
from SANSStateBase import (SANSStateBase, ClassTypeParameter, sans_parameters)
from State.SANSStateSerializer import(convert_state_to_dict, set_state_from_property_manager)


# ------------------------------------------------
# Free functions and enum types
# ------------------------------------------------
class ReductionType(object):
    pass


class SANSReductionType(object):
    class Front(ReductionType):
        pass

    class Back(ReductionType):
        pass

    class Merged(ReductionType):
        pass

    class Both(ReductionType):
        pass


class ReductionDimensionality(object):
    class OneDim(object):
        pass

    class TwoDim(object):
        pass


# ------------------------------------------------
# SANSStateReduction
# ------------------------------------------------
class SANSStateReduction(object):
    pass


# -----------------------------------------------
#  SANSStateReduction for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateReductionISIS(SANSStateReduction, SANSStateBase):
    reduction_type = ClassTypeParameter(ReductionType)
    dimensionality = ClassTypeParameter(ReductionDimensionality)

    def __init__(self):
        super(SANSStateReductionISIS, self).__init__()

    @property
    def property_manager(self):
        return convert_state_to_dict(self)

    @property_manager.setter
    def property_manager(self, value):
        set_state_from_property_manager(self, value)

    def validate(self):
        pass


# -----------------------------------------------
# SANSStateReduction setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateReduction and SANSStateBase and fulfill its contract.
# -----------------------------------------------
