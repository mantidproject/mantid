""" Defines the state of the event slices which should be reduced."""

from __future__ import (absolute_import, division, print_function)
import json
import copy

from sans.state.state_base import (StateBase, PositiveFloatParameter, ClassTypeParameter, rename_descriptor_names)
from sans.common.enums import (RebinType, RangeStepType, SANSInstrument)
from sans.state.state_functions import (is_not_none_and_first_larger_than_second, one_is_none, validation_message)
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateWavelength(StateBase):
    rebin_type = ClassTypeParameter(RebinType)
    wavelength_low = PositiveFloatParameter()
    wavelength_high = PositiveFloatParameter()
    wavelength_step = PositiveFloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    def __init__(self):
        super(StateWavelength, self).__init__()
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
            raise ValueError("StateWavelength: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid, indent=4)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthBuilder(object):
    @automatic_setters(StateWavelength)
    def __init__(self):
        super(StateWavelengthBuilder, self).__init__()
        self.state = StateWavelength()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


def get_wavelength_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateWavelengthBuilder()
    else:
        raise NotImplementedError("StateWavelengthBuilder: Could not find any valid wavelength builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
