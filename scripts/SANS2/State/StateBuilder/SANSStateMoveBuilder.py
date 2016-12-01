# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateMove import (SANSStateMoveLOQ, SANSStateMoveSANS2D, SANSStateMoveLARMOR)
from SANS2.State.StateBuilder.StateBuilderFunctions import (set_detector_names, set_monitor_names)
from SANS2.Common.SANSFileInformation import (get_instrument_paths_for_sans_file)
from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.StateBuilder.AutomaticSetters import automatic_setters


# -------------------------------------
# Free functions
# -------------------------------------
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

    def convert_pos1(self, value):
        return value / 1000.

    def convert_pos2(self, value):
        return value / 1000.


class SANSStateMoveSANS2DBuilder(object):
    @automatic_setters(SANSStateMoveSANS2D, exclusions=["detector_name", "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(SANSStateMoveSANS2DBuilder, self).__init__()
        self.state = SANSStateMoveSANS2D()
        setup_idf_and_ipf_content(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / 1000.

    def convert_pos2(self, value):
        return value / 1000.


class SANSStateMoveLARMORBuilder(object):
    @automatic_setters(SANSStateMoveLARMOR, exclusions=["detector_name",
                                                        "detector_name_short", "monitor_names"])
    def __init__(self, data_info):
        super(SANSStateMoveLARMORBuilder, self).__init__()
        self.state = SANSStateMoveLARMOR()
        setup_idf_and_ipf_content(self.state, data_info)
        self.conversion_value = 1000.
        self._set_conversion_value(data_info)

    def _set_conversion_value(self, data_info):
        run_number = data_info.sample_scatter_run_number
        self.conversion_value = 1000. if run_number >= 2217 else 1.

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def convert_pos1(self, value):
        return value / self.conversion_value

    def convert_pos2(self, value):
        return value / 1000.


# ------------------------------------------
# Factory method for SANStateMoveBuilder
# ------------------------------------------
def get_move_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LOQ:
        return SANSStateMoveLOQBuilder(data_info)
    elif instrument is SANSInstrument.SANS2D:
        return SANSStateMoveSANS2DBuilder(data_info)
    elif instrument is SANSInstrument.LARMOR:
        return SANSStateMoveLARMORBuilder(data_info)
    else:
        raise NotImplementedError("SANSStateMoveBuilder: Could not find any valid move builder for the "
                                  "specified SANSStateData object {0}".format(str(data_info)))
