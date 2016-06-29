""" Defines the state of the reduction."""

from SANS2.State.SANSStateBase import (SANSStateBase, ClassTypeParameter, sans_parameters)
from SANS2.State.SANSStateSerializer import(convert_state_to_dict, set_state_from_property_manager)
from SANS2.Common.SANSEnumerations import (ReductionType, SANSReductionType, ReductionDimensionality)


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
        self.reduction_type = ReductionType.Lab
        self.dimensionality = ReductionDimensionality.OneDim

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
