# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateMask import (SANSStateMaskISIS)
from SANS2.State.StateBuilder.StateBuilderFunctions import (set_detector_names)
from SANS2.Common.SANSEnumerations import SANSInstrument
from SANS2.Common.SANSFileInformation import (get_instrument_paths_for_sans_file)
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# -------------------------------------
# Free functions
# -------------------------------------
def setup_idf_and_ipf_content(move_info, data_info):
    # Get the IDF and IPF path since they contain most of the import information
    file_name = data_info.sample_scatter
    idf_path, ipf_path = get_instrument_paths_for_sans_file(file_name)
    # Set the detector names
    set_detector_names(move_info, ipf_path)
    # Set the idf path
    move_info.idf_path = idf_path


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateMaskBuilderISIS(object):
    @automatic_setters(SANSStateMaskISIS)
    def __init__(self, data_info):
        super(SANSStateMaskBuilderISIS, self).__init__()
        self._data = data_info
        self.state = SANSStateMaskISIS()
        setup_idf_and_ipf_content(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateMaskBuilder
# ------------------------------------------
def get_mask_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return SANSStateMaskBuilderISIS(data_info)
    else:
        raise NotImplementedError("SANSStateMaskBuilder: Could not find any valid mask builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
