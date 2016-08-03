# pylint: disable=too-few-public-methods

import copy

from SANS2.Common.SANSEnumerations import SANSFacility
from SANS2.State.StateBuilder.AutomaticSetters import automatic_setters
from SANS2.State.SANSStateData import SANSStateDataISIS


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateDataISISBuilder(object):
    @automatic_setters(SANSStateDataISIS)
    def __init__(self):
        super(SANSStateDataISISBuilder, self).__init__()
        self.state = SANSStateDataISIS()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_data_builder(facility):
    if facility is SANSFacility.ISIS:
        return SANSStateDataISISBuilder()
    else:
        raise NotImplementedError("SANSStateDataBuilder: The selected facility {0} does not seem"
                                  " to exist".format(str(facility)))
