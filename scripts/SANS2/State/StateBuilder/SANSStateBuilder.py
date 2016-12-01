# pylint: disable=too-few-public-methods

import copy

from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.SANSState import SANSStateISIS
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# State builders
# ---------------------------------------
class SANStateISISBuilder(object):
    @automatic_setters(SANSStateISIS)
    def __init__(self):
        super(SANStateISISBuilder, self).__init__()
        self.state = SANSStateISIS()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_state_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANStateISISBuilder()
    else:
        raise NotImplementedError("SANSStateBuilder: Could not find any valid state builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
