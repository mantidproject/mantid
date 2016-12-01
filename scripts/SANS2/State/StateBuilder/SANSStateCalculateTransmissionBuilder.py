# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateCalculateTransmission import (SANSStateCalculateTransmissionLOQ,
                                                        SANSStateCalculateTransmissionSANS2D,
                                                        SANSStateCalculateTransmissionLARMOR)
from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)
from SANS2.Common.XMLParsing import get_named_elements_from_ipf_file
from SANS2.Common.SANSFileInformation import (get_instrument_paths_for_sans_file)


# -------------------------------------
# Free functions
# -------------------------------------
def set_default_monitors(calculate_transmission_info, data_info):
    """
    The default incident monitor is stored on the IPF.
    :param calculate_transmission_info: a SANSStateCalculateTransmission object on which we set the default value
    :param data_info: a SANSStateData object
    """
    file_name = data_info.sample_scatter
    _, ipf_path = get_instrument_paths_for_sans_file(file_name)
    incident_tag = "default-incident-monitor-spectrum"
    transmission_tag = "default-transmission-monitor-spectrum"
    monitors_to_search = [incident_tag, transmission_tag]
    found_monitor_spectrum = get_named_elements_from_ipf_file(ipf_path, monitors_to_search, int)
    if incident_tag in found_monitor_spectrum:
        calculate_transmission_info.default_incident_monitor = found_monitor_spectrum[incident_tag]
    if transmission_tag in found_monitor_spectrum:
        calculate_transmission_info.default_transmission_monitor = found_monitor_spectrum[transmission_tag]


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateCalculateTransmissionBuilderLOQ(object):
    @automatic_setters(SANSStateCalculateTransmissionLOQ)
    def __init__(self, data_info):
        super(SANSStateCalculateTransmissionBuilderLOQ, self).__init__()
        self._data = data_info
        self.state = SANSStateCalculateTransmissionLOQ()
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class SANSStateCalculateTransmissionBuilderSANS2D(object):
    @automatic_setters(SANSStateCalculateTransmissionSANS2D)
    def __init__(self, data_info):
        super(SANSStateCalculateTransmissionBuilderSANS2D, self).__init__()
        self._data = data_info
        self.state = SANSStateCalculateTransmissionSANS2D()
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class SANSStateCalculateTransmissionBuilderLARMOR(object):
    @automatic_setters(SANSStateCalculateTransmissionLARMOR)
    def __init__(self, data_info):
        super(SANSStateCalculateTransmissionBuilderLARMOR, self).__init__()
        self._data = data_info
        self.state = SANSStateCalculateTransmissionLARMOR()
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANSStateCalculateTransmissionBuilder
# ------------------------------------------
def get_calculate_transmission_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR:
        return SANSStateCalculateTransmissionBuilderLARMOR(data_info)
    elif instrument is SANSInstrument.SANS2D:
        return SANSStateCalculateTransmissionBuilderSANS2D(data_info)
    elif instrument is SANSInstrument.LOQ:
        return SANSStateCalculateTransmissionBuilderLOQ(data_info)
    else:
        raise NotImplementedError("SANSStateCalculateTransmissionBuilder: Could not find any valid transmission "
                                  "builder for the specified SANSStateData object {0}".format(str(data_info)))
