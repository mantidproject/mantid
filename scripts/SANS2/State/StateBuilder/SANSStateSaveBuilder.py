# pylint: disable=too-few-public-methods

import copy
from SANS2.Common.SANSEnumerations import (SANSInstrument)
from SANS2.State.SANSStateSave import (SANSStateSaveISIS)
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# Save builders
# ---------------------------------------
class SANSStateSaveISISBuilder(object):
    @automatic_setters(SANSStateSaveISIS)
    def __init__(self):
        super(SANSStateSaveISISBuilder, self).__init__()
        self.state = SANSStateSaveISIS()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateSaveBuilder
# ------------------------------------------
def get_save_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateSaveISISBuilder()
    else:
        raise NotImplementedError("SANSStateSaveBuilder: Could not find any valid save builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
