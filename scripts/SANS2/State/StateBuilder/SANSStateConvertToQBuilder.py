# pylint: disable=too-few-public-methods

import copy
from SANS2.Common.SANSType import (SANSInstrument)
from SANS2.State.SANSStateConvertToQ import (SANSStateConvertToQISIS)
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# ConvertToQ builders
# ---------------------------------------
class SANSStateConvertToQISISBuilder(object):
    @automatic_setters(SANSStateConvertToQISIS)
    def __init__(self):
        super(SANSStateConvertToQISISBuilder, self).__init__()
        self.state = SANSStateConvertToQISIS()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANSStateConvertToQBuilder
# ------------------------------------------
def get_convert_to_q_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateConvertToQISISBuilder()
    else:
        raise NotImplementedError("SANSStateConvertToQBuilder: Could not find any valid save builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
