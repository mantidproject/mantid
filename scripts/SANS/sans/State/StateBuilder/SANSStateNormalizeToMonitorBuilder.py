# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateNormalizeToMonitor import (SANSStateNormalizeToMonitorISIS, SANSStateNormalizeToMonitorLOQ)
from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)
from SANS2.Common.XMLParsing import get_named_elements_from_ipf_file
from SANS2.Common.SANSFileInformation import (get_instrument_paths_for_sans_file)


# -------------------------------------
# Free functions
# -------------------------------------
def set_default_incident_monitor(normalize_monitor_info, data_info):
    """
    The default incident monitor is stored on the IPF.
    :param normalize_monitor_info: a SANSStateNormalizeMonitor object on which we set the default value
    :param data_info: a SANSStateData object
    """
    file_name = data_info.sample_scatter
    _, ipf_path = get_instrument_paths_for_sans_file(file_name)
    named_element = "default-incident-monitor-spectrum"
    monitor_spectrum_tag_to_search = [named_element]
    found_monitor_spectrum = get_named_elements_from_ipf_file(ipf_path, monitor_spectrum_tag_to_search, int)
    if named_element in found_monitor_spectrum:
        normalize_monitor_info.incident_monitor = found_monitor_spectrum[named_element]


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateNormalizeToMonitorBuilderISIS(object):
    @automatic_setters(SANSStateNormalizeToMonitorISIS, exclusions=["default_incident_monitor"])
    def __init__(self, data_info):
        super(SANSStateNormalizeToMonitorBuilderISIS, self).__init__()
        self._data = data_info
        self.state = SANSStateNormalizeToMonitorISIS()
        set_default_incident_monitor(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class SANSStateNormalizeToMonitorBuilderLOQ(object):
    @automatic_setters(SANSStateNormalizeToMonitorLOQ)
    def __init__(self, data_info):
        super(SANSStateNormalizeToMonitorBuilderLOQ, self).__init__()
        self._data = data_info
        self.state = SANSStateNormalizeToMonitorLOQ()
        set_default_incident_monitor(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANSStateNormalizeToMonitorBuilder
# ------------------------------------------
def get_normalize_to_monitor_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.SANS2D:
        return SANSStateNormalizeToMonitorBuilderISIS(data_info)
    elif instrument is SANSInstrument.LOQ:
        return SANSStateNormalizeToMonitorBuilderLOQ(data_info)
    else:
        raise NotImplementedError("SANSStateNormalizeToMonitorBuilder: Could not find any valid normalize to monitor "
                                  "builder for the specified SANSStateData object {0}".format(str(data_info)))
