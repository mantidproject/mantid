# pylint: disable=too-few-public-methods

import copy

from SANS2.Common.SANSEnumerations import SANSFacility
from SANS2.State.SANSStateData import SANSStateDataISIS
from SANS2.State.StateBuilder.StateBuilderFunctions import automatic_setters


# ---------------------------------------
# State builders
# ---------------------------------------
class SANStateDataISISBuilder(object):
    @automatic_setters(SANSStateDataISIS)
    def __init__(self):
        super(SANStateDataISISBuilder, self).__init__()
        self.state = SANSStateDataISIS()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_state_data_builder(facility):
    if facility is SANSFacility.ISIS:
        return SANSStateDataISIS()
    else:
        raise NotImplementedError("SANSStateDataBuilder: The selected facility {0} does not seem"
                                  " to exist".format(str(facility)))
