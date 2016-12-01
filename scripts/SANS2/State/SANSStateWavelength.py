""" Defines the state of the event slices which should be reduced."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, PositiveFloatParameter, ClassTypeParameter, sans_parameters)
from SANS2.Common.SANSType import (RebinType, RangeStepType)
from SANS2.State.SANSStateFunctions import (is_not_none_and_first_larger_than_second, one_is_none, validation_message)


# ------------------------------------------------
# SANSStateReduction
# ------------------------------------------------
class SANSStateWavelength(object):
    pass


# -----------------------------------------------
#  SANSStateWavelength for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateWavelengthISIS(SANSStateWavelength, SANSStateBase):
    rebin_type = ClassTypeParameter(RebinType)
    wavelength_low = PositiveFloatParameter()
    wavelength_high = PositiveFloatParameter()
    wavelength_step = PositiveFloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    def __init__(self):
        super(SANSStateWavelengthISIS, self).__init__()
        self.rebin_type = RebinType.Rebin

    def validate(self):
        is_invalid = dict()
        if one_is_none([self.wavelength_low, self.wavelength_high, self.wavelength_step]):
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries for the wavelength are set.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high,
                                        "wavelength_step": self.wavelength_step})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Incorrect wavelength bounds.",
                                       "Make sure that lower wavelength bound is smaller then upper bound.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
            is_invalid.update(entry)

        if is_invalid:
            raise ValueError("SANSStateWavelength: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid, indent=4)))

# -----------------------------------------------
# SANSStateWavelength setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateWavelength and SANSStateBase and fulfill its contract.
# -----------------------------------------------
