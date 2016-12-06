# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateAdjustment import (SANSStateAdjustmentISIS)
from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateAdjustmentBuilderISIS(object):
    @automatic_setters(SANSStateAdjustmentISIS)
    def __init__(self):
        super(SANSStateAdjustmentBuilderISIS, self).__init__()
        self.state = SANSStateAdjustmentISIS()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateAdjustmentBuilder
# ------------------------------------------
def get_adjustment_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateAdjustmentBuilderISIS()
    else:
        raise NotImplementedError("SANSStateAdjustmentBuilder: Could not find any valid adjustment builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
