# pylint: disable=too-few-public-methods

import copy

from SANS2.Common.SANSType import SANSFacility
from SANS2.State.StateBuilder.AutomaticSetters import automatic_setters
from SANS2.State.SANSStateData import SANSStateDataISIS
from SANS2.Common.SANSFileInformation import SANSFileInformationFactory


def set_information_from_file(data_info):
    file_name = data_info.sample_scatter
    file_information_factory = SANSFileInformationFactory()
    file_information = file_information_factory.create_sans_file_information(file_name)
    instrument = file_information.get_instrument()
    run_number = file_information.get_run_number()
    data_info.instrument = instrument
    data_info.sample_scatter_run_number = run_number


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

        # There are some elements which need to be read from the file. This is currently:
        # 1. instrument
        # 2. sample_scatter_run_number
        set_information_from_file(self.state)

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
