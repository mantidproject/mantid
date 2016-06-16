import copy
import xml.etree import ElementTree

from State.SANSStateMoveWorkspace import SANSStateMoveWorkspaceLOQ
from Common.SANSFileInformation import (SANSFileInformationFactory, get_instrument_paths_for_sans_file)
from Common.SANSConstants import (SANSConstants, SANSInstrument)
from Common.XMLParsing import get_named_elements_from_ipf_file


# -------------------------------------
# Free functions
# -------------------------------------
def set_detector_names(move_info, ipf_path):
    detector_names = {SANSConstants.low_angle_bank: "low-angle-detector-name",
                     SANSConstants.high_angle_bank: "high-angle-detector-name"}
    detector_names_short = {SANSConstants.low_angle_bank: "low-angle-detector-short-name",
                           SANSConstants.high_angle_bank: "high-angle-detector-short-name"}

    names_to_search = []
    names_to_search.extend(detector_names.values())
    names_to_search.extend(detector_names_short.values())

    found_detector_names = get_named_elements_from_ipf_file(ipf_path, names_to_search)

    for detector_type in move_info.detectors:
        try:
            detector_name_tag = detector_names[detector_type]
            detector_name_short_tag = detector_names_short[detector_type]
            detector_name = found_detector_names[detector_name_tag]
            detector_name_short = found_detector_names[detector_name_short_tag]
        except KeyError:
            continue
        move_info.detectors[detector_type].detector_name = detector_name
        move_info.detectors[detector_type].detector_name_short = detector_name_short
    return move_info


# ---------------------------------------
# State builders
# ---------------------------------------
class SANStateMoveLOQBuilder(object):
    def __init__(self, data_info):
        super(SANStateMoveLOQBuilder, self).__init__()
        self.state = SANSStateMoveWorkspaceLOQ()
        self._extract_move_information(data_info)

    def _extract_move_information(self, data_info):
        # Get the IDF and IPF path since they contain most of the import information
        file_name = data_info.sample_scatter
        idf_path, ipf_path = get_instrument_paths_for_sans_file(file_name)

        # Set the detector names
        self.state = set_detector_names(self.state, ipf_path)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateDataBuilder
# ------------------------------------------
def get_state_move_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    data_info.validate()
    sample_scatter_file = data_info.sample_scatter

    file_info_factory = SANSFileInformationFactory()
    file_info = file_info_factory.create_sans_file_information(sample_scatter_file)
    instrument = file_info.get_instrument()

    if instrument is SANSInstrument.LOQ:
        return SANStateMoveLOQBuilder(data_info)
    else:
        raise NotImplementedError("SANSStateMoveBuilder: Could not find any valid move builder for the "
                                  "specified SANSStateData object {}".format(str(data_info)))
