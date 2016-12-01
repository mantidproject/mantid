# pylint: disable=too-few-public-methods

import copy
from SANS2.State.SANSStateReduction import (SANSStateReductionISIS)
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)
from SANS2.Common.SANSFileInformation import (get_instrument_paths_for_sans_file)
from SANS2.Common.XMLParsing import get_named_elements_from_ipf_file
from SANS2.Common.SANSType import (SANSInstrument)
from SANS2.Common.SANSConstants import SANSConstants


def setup_detectors_from_ipf(reduction_info, data_info):
    file_name = data_info.sample_scatter
    _, ipf_path = get_instrument_paths_for_sans_file(file_name)

    detector_names = {SANSConstants.low_angle_bank: "low-angle-detector-name",
                      SANSConstants.high_angle_bank: "high-angle-detector-name"}

    names_to_search = []
    names_to_search.extend(detector_names.values())

    found_detector_names = get_named_elements_from_ipf_file(ipf_path, names_to_search, str)

    for detector_type in reduction_info.detector_names.keys():
        try:
            detector_name_tag = detector_names[detector_type]
            detector_name = found_detector_names[detector_name_tag]
        except KeyError:
            continue
        reduction_info.detector_names[detector_type] = detector_name


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateReductionISISBuilder(object):
    @automatic_setters(SANSStateReductionISIS, exclusions=["detector_names"])
    def __init__(self, data_info):
        super(SANSStateReductionISISBuilder, self).__init__()
        self.state = SANSStateReductionISIS()
        setup_detectors_from_ipf(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateReductionBuilder
# ------------------------------------------
def get_reduction_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateReductionISISBuilder(data_info)
    else:
        raise NotImplementedError("SANSStateReductionBuilder: Could not find any valid reduction builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
