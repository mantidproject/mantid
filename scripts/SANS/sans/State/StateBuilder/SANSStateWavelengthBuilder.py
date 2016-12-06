# pylint: disable=too-few-public-methods

import copy

from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.SANSStateWavelength import SANSStateWavelengthISIS
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateWavelengthISISBuilder(object):
    @automatic_setters(SANSStateWavelengthISIS)
    def __init__(self):
        super(SANSStateWavelengthISISBuilder, self).__init__()
        self.state = SANSStateWavelengthISIS()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateWavelengthBuilder
# ------------------------------------------
def get_wavelength_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateWavelengthISISBuilder()
    else:
        raise NotImplementedError("SANSStateWavelengthBuilder: Could not find any valid wavelength builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
