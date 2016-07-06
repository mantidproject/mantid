# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateMove import (SANSStateMoveLOQ, SANSStateMoveSANS2D, SANSStateMoveLARMOR)
from SANS2.State.StateBuilder.StateBuilderFunctions import automatic_setters
from SANS2.Common.SANSFileInformation import (SANSFileInformationFactory, get_instrument_paths_for_sans_file)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSEnumerations import SANSInstrument
from SANS2.Common.XMLParsing import (get_named_elements_from_ipf_file, get_monitor_names_from_idf_file)


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

    found_detector_names = get_named_elements_from_ipf_file(ipf_path, names_to_search, str)

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


def set_monitor_names(move_info, idf_path):
    monitor_names = get_monitor_names_from_idf_file(idf_path)
    move_info.monitor_names = monitor_names


def setup_idf_and_ipf_content(move_info, data_info):
    # Get the IDF and IPF path since they contain most of the import information
    file_name = data_info.sample_scatter
    idf_path, ipf_path = get_instrument_paths_for_sans_file(file_name)
    # Set the detector names
    set_detector_names(move_info, ipf_path)
    # Set the monitor names
    set_monitor_names(move_info, idf_path)


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateMoveLOQBuilder(object):
    @automatic_setters(SANSStateMoveLOQ, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(SANSStateMoveLOQBuilder, self).__init__()
        self.state = SANSStateMoveLOQ()
        setup_idf_and_ipf_content(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class SANSStateMoveSANS2DBuilder(object):
    @automatic_setters(SANSStateMoveSANS2D, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(SANSStateMoveSANS2DBuilder, self).__init__()
        self.state = SANSStateMoveSANS2D()
        setup_idf_and_ipf_content(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class SANSStateMoveLARMORBuilder(object):
    @automatic_setters(SANSStateMoveLARMOR, exclusions=["detector_name",
                                                        "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(SANSStateMoveLARMORBuilder, self).__init__()
        self.state = SANSStateMoveLARMOR()
        setup_idf_and_ipf_content(self.state, data_info)

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
        return SANSStateMoveLOQBuilder(data_info)
    elif instrument is SANSInstrument.SANS2D:
        return SANSStateMoveSANS2DBuilder(data_info)
    elif instrument is SANSInstrument.LARMOR:
        return SANSStateMoveLARMORBuilder(data_info)
    else:
        raise NotImplementedError("SANSStateMoveBuilder: Could not find any valid move builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
