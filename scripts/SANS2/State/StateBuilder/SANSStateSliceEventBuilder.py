# pylint: disable=too-few-public-methods

import copy

from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.SANSStateSliceEvent import SANSStateSliceEventISIS
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateSliceEventISISBuilder(object):
    @automatic_setters(SANSStateSliceEventISIS)
    def __init__(self):
        super(SANSStateSliceEventISISBuilder, self).__init__()
        self.state = SANSStateSliceEventISIS()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_slice_event_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateSliceEventISISBuilder()
    else:
        raise NotImplementedError("SANSStateSliceEventBuilder: Could not find any valid slice builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
