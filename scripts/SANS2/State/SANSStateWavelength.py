""" Defines the state of the event slices which should be reduced."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, FloatParameter, ClassTypeParameter, sans_parameters)
from SANS2.Common.SANSEnumerations import (RebinType, RangeStepType)


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
    wavelength_low = FloatParameter()
    wavelength_high = FloatParameter()
    wavelength_step = FloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    def __init__(self):
        super(SANSStateWavelengthISIS, self).__init__()
        self.rebin_type = RebinType.Rebin

    def validate(self):
        is_invalid = dict()

        if self.wavelength_low is not None and self.wavelength_high is None or \
           self.wavelength_low is None and self.wavelength_high is not None:
            is_invalid.update({"wave_length_low and wave_length_high": "Either both are specified or none."})

        if self.wavelength_low is not None and self.wavelength_high is not None and \
           self.wavelength_low > self.wavelength_high:
            is_invalid.update({"wave_length_low and wave_length_high": "wave_length_low {0} appears to be larger than "
                                                                       "wavelength_high {1}".format(self.wavelength_low,
                                                                                                    self.wavelength_high)})  # noqa

        if is_invalid:
            raise ValueError("SANSStateWavelength: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid, indent=4)))

# -----------------------------------------------
# SANSStateWavelength setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateWavelength and SANSStateBase and fulfill its contract.
# -----------------------------------------------
